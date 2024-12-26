//===- sqlite3.cpp -------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the interface of the SQLite3 library.
//
//===------------------------------------------------------------------===//

#include "common/util/sqlite3.hpp"
#include <stdexcept>

#include "common/util/log.hpp"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "sqlite3"

namespace knight::sqlite {

/// \brief Return the error message for the given code
///
/// Taken from the SQLite online documentation.
const char* error_code_message(int code) noexcept {
    switch (code) {
        case SQLITE_ERROR:
            return "SQL error or missing database";
        case SQLITE_INTERNAL:
            return "Internal logic error in SQLite";
        case SQLITE_PERM:
            return "Access permission denied";
        case SQLITE_ABORT:
            return "Callback routine requested an abort";
        case SQLITE_BUSY:
            return "The database file is locked";
        case SQLITE_LOCKED:
            return "A table in the database is locked";
        case SQLITE_NOMEM:
            return "A malloc() failed";
        case SQLITE_READONLY:
            return "Attempt to write a readonly database";
        case SQLITE_INTERRUPT:
            return "Operation terminated by sqlite3_interrupt()";
        case SQLITE_IOERR:
            return "Some kind of disk I/O error occurred";
        case SQLITE_CORRUPT:
            return "The database disk image is malformed";
        case SQLITE_NOTFOUND:
            return "Unknown opcode in sqlite3_file_control()";
        case SQLITE_FULL:
            return "Insertion failed because database is full";
        case SQLITE_CANTOPEN:
            return "Unable to open the database file";
        case SQLITE_PROTOCOL:
            return "Database lock protocol error";
        case SQLITE_EMPTY:
            return "Database is empty";
        case SQLITE_SCHEMA:
            return "The database schema changed";
        case SQLITE_TOOBIG:
            return "String or BLOB exceeds size limit";
        case SQLITE_CONSTRAINT:
            return "Abort due to constraint violation";
        case SQLITE_MISMATCH:
            return "Data type mismatch";
        case SQLITE_MISUSE:
            return "Library used incorrectly";
        case SQLITE_NOLFS:
            return "Uses OS features not supported on host";
        case SQLITE_AUTH:
            return "Authorization denied";
        case SQLITE_FORMAT:
            return "Auxiliary database format error";
        case SQLITE_RANGE:
            return "2nd parameter to sqlite3_bind out of range";
        case SQLITE_NOTADB:
            return "File opened that is not a database file";
        case SQLITE_ROW:
            return "sqlite3_step() has another row ready";
        case SQLITE_DONE:
            return "sqlite3_step() has finished executing";
        default:
            return "Unknown error code";
    }
}

const char* column_type_str(knight::sqlite::ResColumn::ColumnKind type) {
    switch (type) {
        using enum knight::sqlite::ResColumn::ColumnKind;
        case INTEGER:
            return "INTEGER";
        case FLOAT:
            return "FLOAT";
        case TEXT:
            return "TEXT";
        case BLOB:
            return "BLOB";
        case NULL_:
            return "NULL";
        default:
            knight_unreachable("invalid column type");
    }
}

const char* get_lib_version() noexcept {
    return sqlite3_libversion();
}

int get_lib_version_number() noexcept {
    return sqlite3_libversion_number();
}

ResColumn::ColumnKind ResColumn::get_value_type() const noexcept {
    return static_cast< ResColumn::ColumnKind >(
        sqlite3_column_type(m_stmt.get(), static_cast< int >(m_idx)));
}

unsigned ResColumn::get_num_bytes() const noexcept {
    return sqlite3_column_bytes(m_stmt.get(), static_cast< int >(m_idx));
}

const void* ResColumn::get_as_blob() const noexcept {
    return sqlite3_column_blob(m_stmt.get(), static_cast< int >(m_idx));
}

std::string ResColumn::get_as_string() const noexcept {
    return {get_as_text(), get_num_bytes()};
}

const char* ResColumn::get_as_text() const noexcept {
    const auto* p = reinterpret_cast< const char* >(
        sqlite3_column_text(m_stmt.get(), static_cast< int >(m_idx)));
    return p != nullptr ? p : "";
}

double ResColumn::get_as_double() const noexcept {
    return sqlite3_column_double(m_stmt.get(), static_cast< int >(m_idx));
}

int64_t ResColumn::get_as_int64() const noexcept {
    return sqlite3_column_int64(m_stmt.get(), static_cast< int >(m_idx));
}

int32_t ResColumn::get_as_int() const noexcept {
    return sqlite3_column_int(m_stmt.get(), static_cast< int >(m_idx));
}

std::string ResColumn::get_name() const noexcept {
    return sqlite3_column_name(m_stmt.get(), static_cast< int >(m_idx));
}

KNIGHT_API void Database::HandleCloser::operator()(sqlite3* handle) {
    const int ret = sqlite3_close(handle);
    (void)ret;

    // statements are not finalized
    knight_assert_msg(ret == SqliteOK, "SQLITE_BUSY: database is locked");
}

Database::Database(const std::string& file,
                   const int flags,
                   const int busy_timeout_milliseconds,
                   const std::string& vfs) noexcept(false)
    : m_file(file) {
    sqlite3* handle = nullptr;

    knight_log_nl(llvm::outs() << "Opening sqlite3 database: " << file);

    const int ret = sqlite3_open_v2(file.c_str(),
                                    &handle,
                                    flags,
                                    vfs.empty() ? nullptr : vfs.c_str());
    m_sqlite3_handle.reset(handle);
    validate_return(ret);
    if (busy_timeout_milliseconds > 0) {
        set_busy_timeout(busy_timeout_milliseconds);
    }
}

void Database::set_busy_timeout(const int milliseconds) const noexcept(false) {
    validate_return(sqlite3_busy_timeout(get_handle(), milliseconds));
}

void Database::validate_return(const int ret) const noexcept(false) {
    if (ret != SqliteOK) {
        throw std::runtime_error(
            "Sqlite db error code = " + std::to_string(ret) +
            ", error message = " + std::string(sqlite3_errmsg(get_handle())));
    }
}

sqlite3* Database::get_handle() const noexcept {
    return m_sqlite3_handle.get();
}

int Database::execute(const std::string& sqls) const noexcept(false) {
    const int ret = try_execute(sqls);
    validate_return(ret);

    return sqlite3_changes(get_handle());
}

int Database::try_execute(const std::string& sqls,
                          bool& invalid) const noexcept {
    const int ret = try_execute(sqls);
    if (ret != SqliteOK) {
        invalid = true;
    }
    return ret;
}

int Database::try_execute(const std::string& sqls) const noexcept {
    knight_log_nl(llvm::outs() << "Executing SQL: " << sqls);
    return sqlite3_exec(get_handle(), sqls.c_str(), nullptr, nullptr, nullptr);
}

PreparedStmt::PreparedStmt(const Database& db, std::string sql)
    : m_sql(std::move(sql)),
      m_handle(db.get_handle()),
      m_prepared_intern_stmt(get_prepare_intern_statement_ref()),
      m_res_col_cnt(sqlite3_column_count(m_prepared_intern_stmt.get())) {
    knight_log_nl(llvm::outs() << "Preparing SQL: " << m_sql);
}

PreparedStmt::PreparedStmt(PreparedStmt&& aStatement) noexcept
    : m_sql(std::move(aStatement.m_sql)),
      m_handle(aStatement.m_handle),
      m_prepared_intern_stmt(std::move(aStatement.m_prepared_intern_stmt)),
      m_res_col_cnt(aStatement.m_res_col_cnt),
      m_row_to_fetched(aStatement.m_row_to_fetched),
      m_rows_done(aStatement.m_rows_done),
      m_col_name_idx_cache(std::move(aStatement.m_col_name_idx_cache)) {
    aStatement.m_handle = nullptr;
    aStatement.m_res_col_cnt = 0;
    aStatement.m_row_to_fetched = false;
    aStatement.m_rows_done = false;
}

void PreparedStmt::validate_return(const int ret) const noexcept(false) {
    if (ret != SqliteOK) {
        throw_db_error(ret);
    }
}

void PreparedStmt::throw_db_error(int ret, const std::string& extra_msg) const
    noexcept(false) {
    throw std::runtime_error(
        "Sqlite stmt error code = " + std::to_string(ret) +
        ", error message = " + std::string(sqlite3_errmsg(m_handle)) + " " +
        extra_msg);
}

int PreparedStmt::try_reset() noexcept {
    m_row_to_fetched = false;
    m_rows_done = false;
    return sqlite3_reset(m_prepared_intern_stmt.get());
}

void PreparedStmt::reset() noexcept(false) {
    validate_return(try_reset());
}

PreparedInternStmt* PreparedStmt::get_prepare_intern_statement() const {
    if (auto* s = m_prepared_intern_stmt.get()) {
        return s;
    }
    throw std::runtime_error("Statement was not prepared.");
}

PreparedInternStmtRef PreparedStmt::get_prepare_intern_statement_ref() {
    sqlite3_stmt* statement = nullptr;
    validate_return(sqlite3_prepare_v2(m_handle,
                                       m_sql.c_str(),
                                       static_cast< int >(m_sql.size()),
                                       &statement,
                                       nullptr));

    return {statement, [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); }};
}

void PreparedStmt::clear_bindings() {
    validate_return(sqlite3_clear_bindings(get_prepare_intern_statement()));
}

KNIGHT_PURE_FUNC
int PreparedStmt::get_idx(const std::string& name) const {
    return sqlite3_bind_parameter_index(get_prepare_intern_statement(),
                                        name.c_str());
}

void PreparedStmt::bind(const int idx, const int32_t val) {
    // knight_log_nl(llvm::outs()
    //               << "Binding param " << idx << " to int32_t: " << val);
    validate_return(sqlite3_bind_int(get_prepare_intern_statement(), idx, val));
}

void PreparedStmt::bind(const int idx, const uint32_t val) {
    // knight_log_nl(llvm::outs()
    //               << "Binding param " << idx << " to uint32_t: " << val);
    validate_return(
        sqlite3_bind_int64(get_prepare_intern_statement(), idx, val));
}

void PreparedStmt::bind(const int idx, const int64_t val) {
    // knight_log_nl(llvm::outs()
    //               << "Binding param " << idx << " to int64_t: " << val);
    validate_return(
        sqlite3_bind_int64(get_prepare_intern_statement(), idx, val));
}

void PreparedStmt::bind(const int idx, const double val) {
    // knight_log_nl(llvm::outs()
    //               << "Binding param " << idx << " to double: " << val);
    validate_return(
        sqlite3_bind_double(get_prepare_intern_statement(), idx, val));
}

void PreparedStmt::bind(const int idx, const std::string& val) {
    // knight_log_nl(llvm::outs()
    //               << "Binding param " << idx << " to string: " << val);
    validate_return(sqlite3_bind_text(get_prepare_intern_statement(),
                                      idx,
                                      val.c_str(),
                                      static_cast< int >(val.size()),
                                      SQLITE_TRANSIENT));
}

void PreparedStmt::bind(const int idx, const void* val, const int size) {
    // knight_log_nl(llvm::outs()
    //               << "Binding param " << idx << " to blob of size: " <<
    //               size);
    validate_return(sqlite3_bind_blob(get_prepare_intern_statement(),
                                      idx,
                                      val,
                                      size,
                                      SQLITE_TRANSIENT));
}

void PreparedStmt::bind_no_copy(const int idx, const std::string& val) {
    // knight_log_nl(llvm::outs() << "Binding param " << idx
    //                            << " to string (no copy): " << val);
    validate_return(sqlite3_bind_text(get_prepare_intern_statement(),
                                      idx,
                                      val.c_str(),
                                      static_cast< int >(val.size()),
                                      SQLITE_STATIC));
}

void PreparedStmt::bind_no_copy(const int idx,
                                const void* val,
                                const int size) {
    // knight_log_nl(llvm::outs() << "Binding param " << idx
    //                            << " to blob of size (no copy): " << size);
    validate_return(sqlite3_bind_blob(get_prepare_intern_statement(),
                                      idx,
                                      val,
                                      size,
                                      SQLITE_STATIC));
}

void PreparedStmt::bind_null(const int idx) {
    // knight_log_nl(llvm::outs() << "Binding param " << idx << " to NULL");
    validate_return(sqlite3_bind_null(get_prepare_intern_statement(), idx));
}
bool PreparedStmt::execute_step() {
    const int ret = try_execute_step();
    if ((SQLITE_ROW != ret) &&
        (SQLITE_DONE !=
         ret)) // on row or no (more) row ready, else it's a problem
    {
        if (ret == sqlite3_errcode(m_handle)) {
            throw_db_error(ret);
        } else {
            throw_db_error(ret, "Statement needs to be reset: ");
        }
    }

    return m_row_to_fetched;
}

int PreparedStmt::try_execute_step() noexcept {
    if (m_rows_done) {
        return SQLITE_MISUSE; // Statement needs to be reset !
    }
    knight_log_nl(llvm::outs()
                  << "Executing one step: " << get_expanded_sql() << "\n");
    const int ret = sqlite3_step(m_prepared_intern_stmt.get());
    if (SQLITE_ROW == ret) {
        m_row_to_fetched = true;
    } else {
        m_row_to_fetched = false;
        m_rows_done = ret == SQLITE_DONE;
    }
    return ret;
}

int PreparedStmt::execute() {
    const int ret = try_execute_step();
    if (SQLITE_DONE != ret) {
        if (SQLITE_ROW == ret) {
            throw_db_error(ret,
                           "exec() does not expect results. Use "
                           "execute_step.");
        } else if (ret == sqlite3_errcode(m_handle)) {
            throw_db_error(ret);
        } else {
            throw_db_error(ret, "Statement needs to be reset");
        }
    }

    // Return the number of rows modified by those SQL statements (INSERT,
    // UPDATE or DELETE)
    return sqlite3_changes(m_handle);
}

void PreparedStmt::validate_row_to_fetched() const {
    if (!m_row_to_fetched) {
        throw std::runtime_error(
            "No row to get a column from. execute_step() was not called, or "
            "returned false.");
    }
}

void PreparedStmt::validate_col_idx(const int idx) const {
    if ((idx < 0) || (idx >= m_res_col_cnt)) {
        throw std::runtime_error("Column index out of range.");
    }
}

ResColumn PreparedStmt::get_column(const int idx) const {
    validate_row_to_fetched();
    validate_col_idx(idx);
    return ResColumn(m_prepared_intern_stmt, idx);
}

int PreparedStmt::get_column_index(const char* name) const noexcept(false) {
    if (m_col_name_idx_cache.empty()) {
        for (int i = 0; i < m_res_col_cnt; ++i) {
            m_col_name_idx_cache
                [sqlite3_column_name(get_prepare_intern_statement(), i)] = i;
        }
    }

    const auto it = m_col_name_idx_cache.find(name);
    if (it == m_col_name_idx_cache.end()) {
        throw std::runtime_error("Unknown column name.");
    }
    return it->second;
}

ResColumn PreparedStmt::get_column(const std::string& name) const {
    validate_row_to_fetched();
    return ResColumn(m_prepared_intern_stmt, get_column_index(name.c_str()));
}

bool PreparedStmt::is_column_null(const int idx) const {
    validate_row_to_fetched();
    validate_col_idx(idx);
    return (SQLITE_NULL ==
            sqlite3_column_type(get_prepare_intern_statement(), idx));
}
bool PreparedStmt::is_column_null(const char* name) const {
    validate_row_to_fetched();
    return (SQLITE_NULL == sqlite3_column_type(get_prepare_intern_statement(),
                                               get_column_index(name)));
}

const char* PreparedStmt::get_column_name(const int idx) const {
    validate_col_idx(idx);
    return sqlite3_column_name(get_prepare_intern_statement(), idx);
}

const char* PreparedStmt::get_column_origin_name(const int idx) const {
    validate_col_idx(idx);
    return sqlite3_column_origin_name(get_prepare_intern_statement(), idx);
}

const char* PreparedStmt::get_column_declared_type(const int idx) const
    noexcept(false) {
    validate_col_idx(idx);
    const char* result =
        sqlite3_column_decltype(get_prepare_intern_statement(), idx);
    if (result == nullptr) {
        throw std::runtime_error("Could not determine declared column type.");
    }
    return result;
}

int PreparedStmt::get_last_changes() const noexcept {
    return sqlite3_changes(m_handle);
}

int PreparedStmt::get_bind_parameter_count() const noexcept {
    return sqlite3_bind_parameter_count(m_prepared_intern_stmt.get());
}

ResColumn Database::exec_and_get_first(const std::string& sql) const
    noexcept(false) {
    PreparedStmt query(*this, sql);
    (void)query.execute_step();
    return query.get_column(0);
}

bool Database::table_exists( // NOLINT
    const std::string& table_name) const noexcept {
    PreparedStmt query(*this,
                       "SELECT count(*) FROM sqlite_master WHERE type='table' "
                       "AND name=?");
    query.bind(1, table_name);
    (void)query.execute_step();
    return query.get_column(0).get_as_int() == 1;
}
int Database::get_last_dml_changes() const noexcept {
    return sqlite3_changes(get_handle());
}
int Database::get_total_dml_changes() const noexcept {
    return sqlite3_total_changes(get_handle());
}

// NOLINTBEGIN
DBHeader Database::parse_header(const std::string& file) const noexcept(false) {
    DBHeader header;
    unsigned char buf[100];
    char* pBuf = reinterpret_cast< char* >(&buf[0]);
    char* pHeaderStr = reinterpret_cast< char* >(&header.header_str[0]);

    if (file.empty()) {
        throw std::runtime_error("Filename cannot be empty");
    }

    {
        std::ifstream fileBuffer(file.c_str(), std::ios::in | std::ios::binary);
        if (fileBuffer.is_open()) {
            fileBuffer.seekg(0, std::ios::beg);
            fileBuffer.read(pBuf, 100);
            fileBuffer.close();
            if (fileBuffer.gcount() < 100) {
                throw std::runtime_error("Too short file :" + file);
            }
        } else {
            throw std::runtime_error("Error opening file: " + file);
        }
    }

    // If the "magic string" can't be found then header is invalid, corrupt
    // or unreadable
    memcpy(pHeaderStr, pBuf, 16);
    pHeaderStr[15] = '\0';
    if (strncmp(pHeaderStr, "SQLite format 3", 15) != 0) {
        throw std::runtime_error(
            "Invalid or encrypted SQLite header in file: " + file);
    }

    header.page_size_bytes = (buf[16] << 8) | buf[17];
    header.file_format_write_version = buf[18];
    header.file_format_read_version = buf[19];
    header.reserved_space_bytes = buf[20];
    header.max_embedded_payload_frac = buf[21];
    header.min_embedded_payload_frac = buf[22];
    header.leaf_payload_frac = buf[23];

    header.file_change_counter =
        (buf[24] << 24) | (buf[25] << 16) | (buf[26] << 8) | (buf[27] << 0);

    header.database_size_pages =
        (buf[28] << 24) | (buf[29] << 16) | (buf[30] << 8) | (buf[31] << 0);

    header.first_freelist_trunk_page =
        (buf[32] << 24) | (buf[33] << 16) | (buf[34] << 8) | (buf[35] << 0);

    header.total_freelist_pages =
        (buf[36] << 24) | (buf[37] << 16) | (buf[38] << 8) | (buf[39] << 0);

    header.schema_cookie =
        (buf[40] << 24) | (buf[41] << 16) | (buf[42] << 8) | (buf[43] << 0);

    header.schema_format_number =
        (buf[44] << 24) | (buf[45] << 16) | (buf[46] << 8) | (buf[47] << 0);

    header.default_page_cache_size_bytes =
        (buf[48] << 24) | (buf[49] << 16) | (buf[50] << 8) | (buf[51] << 0);

    header.largest_b_tree_page_number =
        (buf[52] << 24) | (buf[53] << 16) | (buf[54] << 8) | (buf[55] << 0);

    header.database_text_encoding =
        (buf[56] << 24) | (buf[57] << 16) | (buf[58] << 8) | (buf[59] << 0);

    header.user_version =
        (buf[60] << 24) | (buf[61] << 16) | (buf[62] << 8) | (buf[63] << 0);

    header.incremental_vacuum_mode =
        (buf[64] << 24) | (buf[65] << 16) | (buf[66] << 8) | (buf[67] << 0);

    header.application_id =
        (buf[68] << 24) | (buf[69] << 16) | (buf[70] << 8) | (buf[71] << 0);

    header.version_valid_for =
        (buf[92] << 24) | (buf[93] << 16) | (buf[94] << 8) | (buf[95] << 0);

    header.sqlite_version =
        (buf[96] << 24) | (buf[97] << 16) | (buf[98] << 8) | (buf[99] << 0);

    return header;
}
// NOLINTEND

int Database::get_error_code() const noexcept {
    return sqlite3_errcode(get_handle());
}
int Database::get_extended_error_code() const noexcept {
    return sqlite3_extended_errcode(get_handle());
}
std::string Database::get_error_msg() const noexcept {
    return sqlite3_errmsg(get_handle());
}
int64_t Database::get_last_insert_rowid() const noexcept {
    return sqlite3_last_insert_rowid(get_handle());
}

bool Database::is_unencrypted(const std::string& file) noexcept(false) {
    if (file.empty()) {
        throw std::runtime_error(
            "Could not determine if file is unencrypted: filename is "
            "empty");
    }

    std::ifstream file_buffer(file.c_str(), std::ios::in | std::ios::binary);
    std::array< char, DBHeaderStringSize > header{};
    if (file_buffer.is_open()) {
        file_buffer.seekg(0, std::ios::beg);
        file_buffer.getline(header.data(), DBHeaderStringSize);
        file_buffer.close();
    } else {
        throw std::runtime_error("Error opening file: " + file);
    }

    return strncmp(header.data(), "SQLite format 3\000", DBHeaderStringSize) ==
           0;
}

void Database::set_key(const std::string& key) {
    int len = static_cast< int >(key.length());
#ifdef SQLITE_HAS_ENCRYPTION
    if (len > 0) {
        valid_return(sqlite3_key(getHandle(), key.c_str(), len));
    }
#else  // SQLITE_HAS_ENCRYPTION
    if (len > 0) {
        throw std::runtime_error("No encryption support.");
    }
#endif // SQLITE_HAS_ENCRYPTIONs
}

// Reset the key for the current sqlite database instance.
void Database::reset_key(const std::string& key) {
#ifdef SQLITE_HAS_ENCRYPTION
    int len = key.length();
    if (len > 0) {
        valid_return(sqlite3_rekey(getHandle(), key.c_str(), passLen));
    } else {
        valid_return(sqlite3_rekey(getHandle(), nullptr, 0));
    }
#else  // SQLITE_HAS_ENCRYPTION
    static_cast< void >(key); // silence unused parameter warning
    throw std::runtime_error("No encryption support.");
#endif // SQLITE_HAS_ENCRYPTION
}

int PreparedStmt::get_error_code() const noexcept {
    return sqlite3_errcode(m_handle);
}

int PreparedStmt::get_extended_error_code() const noexcept {
    return sqlite3_extended_errcode(m_handle);
}

std::string PreparedStmt::get_error_msg() const noexcept {
    return sqlite3_errmsg(m_handle);
}

std::string PreparedStmt::get_expanded_sql() const {
    char* expanded = sqlite3_expanded_sql(get_prepare_intern_statement());
    std::string expanded_string(expanded);
    sqlite3_free(expanded);
    return expanded_string;
}

Transaction::Transaction(Database& db) noexcept(false) : m_db(db) {
    (void)m_db.execute("BEGIN TRANSACTION");
}

Transaction::~Transaction() noexcept {
    if (!m_committed) {
        try {
            (void)m_db.execute("ROLLBACK TRANSACTION");
        } catch (...) {
        }
    }
}

Transaction::Transaction(Database& db, TransactionMode behavior) noexcept(false)
    : m_db(db) {
    const char* stmt = nullptr;
    switch (behavior) {
        case TransactionMode::DEFERRED:
            stmt = "BEGIN DEFERRED";
            break;
        case TransactionMode::IMMEDIATE:
            stmt = "BEGIN IMMEDIATE";
            break;
        case TransactionMode::EXCLUSIVE:
            stmt = "BEGIN EXCLUSIVE";
            break;
        default:
            throw std::runtime_error("invalid/unknown transaction behavior");
    }
    (void)m_db.execute(stmt);
}

void Transaction::commit() noexcept(false) {
    if (!m_committed) {
        (void)m_db.execute("COMMIT TRANSACTION");
        m_committed = true;
    } else {
        throw std::runtime_error("Transaction already committed.");
    }
}

void Transaction::rollback() noexcept(false) {
    if (!m_committed) {
        (void)m_db.execute("ROLLBACK TRANSACTION");
    } else {
        throw std::runtime_error("Transaction already committed.");
    }
}

} // namespace knight::sqlite
