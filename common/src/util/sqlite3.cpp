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

namespace {

/// \brief Return the error message for the given code
///
/// Taken from the SQLite online documentation.
static const char* error_code_message(int code) noexcept {
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

const char* column_type_str(knight::sqlite::DbColumnType type) {
    switch (type) {
        using enum knight::sqlite::DbColumnType;
        case Text:
            return "TEXT";
        case Integer:
            return "INTEGER";
        case Real:
            return "REAL";
        case Blob:
            return "BLOB";
        default:
            knight_unreachable("invalid column type");
    }
}

} // anonymous namespace

namespace knight::sqlite {

DBException::DBException(int code)
    : m_code(code),
      m_context(std::make_shared< const std::string >()),
      m_what(std::make_shared< const std::string >(
          "[" + std::to_string(code) + "] " + error_code_message(code))) {}

DBException::DBException(int code, const std::string& context)
    : m_code(code),
      m_context(std::make_shared< const std::string >(context)),
      m_what(std::make_shared< const std::string >(context + "\n[" +
                                                   std::to_string(code) + "] " +
                                                   error_code_message(code))) {}

const char* DBException::error_msg() const noexcept {
    return error_code_message(this->m_code);
}

const char* DBException::what() const noexcept {
    return this->m_what->c_str();
}

// DbConnection

DbConnection::DbConnection(std::string filename)
    : m_filename(std::move(filename)) {
    int status = sqlite3_open_v2(this->m_filename.c_str(),
                                 &this->m_handle,
                                 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                 nullptr);
    if (status != SQLITE_OK) {
        throw DBException(status,
                          "DbConnection: cannot open database: " +
                              this->m_filename);
    }
}

DbConnection::~DbConnection() {
    // The destructor shall not throw an exception. No error check.
    if (this->m_commit_policy == CommitPolicy::Auto) {
        sqlite3_exec(this->m_handle, "COMMIT", nullptr, nullptr, nullptr);
    }

    sqlite3_close(this->m_handle);
}

void DbConnection::exec_command(const char* cmd) {
    knight_assert_msg(cmd != nullptr, "cmd is null");

    int status = sqlite3_exec(this->m_handle, cmd, nullptr, nullptr, nullptr);
    if (status != SQLITE_OK) {
        throw DBException(status,
                          "DbConnection::exec_command(): " + std::string(cmd));
    }
}

void DbConnection::exec_command(const std::string& cmd) {
    this->exec_command(cmd.c_str());
}

void DbConnection::begin_transaction() {
    knight_assert(this->m_commit_policy == CommitPolicy::Manual);
    this->exec_command("BEGIN");
}

void DbConnection::commit_transaction() {
    knight_assert(this->m_commit_policy == CommitPolicy::Manual);
    this->exec_command("COMMIT");
}

void DbConnection::rollback_transaction() {
    knight_assert(this->m_commit_policy == CommitPolicy::Manual);
    this->exec_command("ROLLBACK");
}

void DbConnection::set_commit_policy(CommitPolicy policy) {
    if (this->m_commit_policy == CommitPolicy::Auto) {
        this->exec_command("COMMIT");
        this->m_inserted_rows = 0;
    }

    this->m_commit_policy = policy;

    if (this->m_commit_policy == CommitPolicy::Auto) {
        this->exec_command("BEGIN");
        this->m_inserted_rows = 0;
    }
}

void DbConnection::row_inserted() {
    if (this->m_commit_policy == CommitPolicy::Auto) {
        this->m_inserted_rows++;

        if (this->m_inserted_rows >= MaxRowsPerTransaction) {
            this->exec_command("COMMIT");
            this->m_inserted_rows = 0;
            this->exec_command("BEGIN");
        }
    }
}

void DbConnection::drop_table(llvm::StringRef name) {
    std::string cmd("DROP TABLE IF EXISTS ");
    cmd += name;
    this->exec_command(cmd);
}

DbInt64 DbConnection::last_insert_rowid() const {
    return sqlite3_last_insert_rowid(this->m_handle);
}

void DbConnection::create_table(
    llvm::StringRef name,
    llvm::ArrayRef< std::pair< llvm::StringRef, DbColumnType > > columns) {
    knight_assert_msg(!columns.empty(), "columns is empty");

    std::string cmd("CREATE TABLE IF NOT EXISTS ");
    cmd += name;
    cmd += '(';
    for (auto it = columns.begin(), et = columns.end(); it != et;) {
        cmd += it->first;
        cmd += ' ';
        cmd += column_type_str(it->second);
        if (it->first == "id") {
            cmd += " PRIMARY KEY";
        }
        ++it;
        if (it != et) {
            cmd += ',';
        }
    }
    cmd += ')';
    this->exec_command(cmd.c_str());
}

void DbConnection::create_index(llvm::StringRef index,
                                llvm::StringRef table,
                                llvm::StringRef column) {
    std::string cmd("CREATE INDEX IF NOT EXISTS ");
    cmd += index;
    cmd += " ON ";
    cmd += table;
    cmd += '(';
    cmd += column;
    cmd += ')';
    this->exec_command(cmd.c_str());
}

void DbConnection::set_journal_mode(JournalMode mode) {
    switch (mode) {
        case JournalMode::Delete: {
            this->exec_command("PRAGMA journal_mode = DELETE");
        } break;
        case JournalMode::Truncate: {
            this->exec_command("PRAGMA journal_mode = TRUNCATE");
        } break;
        case JournalMode::Persist: {
            this->exec_command("PRAGMA journal_mode = PERSIST");
        } break;
        case JournalMode::Memory: {
            this->exec_command("PRAGMA journal_mode = MEMORY");
        } break;
        case JournalMode::WAL: {
            this->exec_command("PRAGMA journal_mode = WAL");
        } break;
        case JournalMode::Off: {
            this->exec_command("PRAGMA journal_mode = OFF");
        } break;
    }
}

void DbConnection::set_synchronous_flag(SynchronousFlag flag) {
    switch (flag) {
        case SynchronousFlag::Off: {
            this->exec_command("PRAGMA synchronous = OFF");
        } break;
        case SynchronousFlag::Normal: {
            this->exec_command("PRAGMA synchronous = NORMAL");
        } break;
        case SynchronousFlag::Full: {
            this->exec_command("PRAGMA synchronous = FULL");
        } break;
        case SynchronousFlag::Extra: {
            this->exec_command("PRAGMA synchronous = EXTRA");
        } break;
    }
}

// DbOstream

DbOstream::DbOstream(DbConnection& db, llvm::StringRef table_name, int columns)
    : m_db(db), m_columns(columns) {
    knight_assert_msg(columns > 0, "invalid number of columns");

    // Create SQL command
    std::string insert("INSERT INTO ");
    insert += table_name;
    insert += " VALUES (";
    for (int i = 0; i < columns; i++) {
        insert += '?';
        insert += (i + 1 < columns) ? ',' : ')';
    }

    int status = sqlite3_prepare_v2(this->m_db.m_handle,
                                    insert.c_str(),
                                    -1,
                                    &this->m_stmt,
                                    nullptr);
    if (status != SQLITE_OK) {
        throw DBException(status,
                          "DbOstream: cannot populate " + table_name.str() +
                              " in database " + this->m_db.filename());
    }
}

DbOstream::~DbOstream() {
    // The destructor shall not throw an exception. No error check is performed.
    sqlite3_finalize(m_stmt);
}

void DbOstream::add(llvm::StringRef s) {
    knight_assert(s.size() <= static_cast< std::size_t >(
                                  std::numeric_limits< int >::max()));

    int status = sqlite3_bind_text(this->m_stmt,
                                   this->m_current_column++,
                                   s.data(),
                                   static_cast< int >(s.size()),
                                   SQLITE_TRANSIENT);
    if (status != SQLITE_OK) {
        throw DBException(status, "DbOstream::add(llvm::StringRef)");
    }
}

void DbOstream::add_null() {
    int status = sqlite3_bind_null(this->m_stmt, this->m_current_column++);
    if (status != SQLITE_OK) {
        throw DBException(status, "DbOstream::add_null()");
    }
}

void DbOstream::add(DbInt64 n) {
    int status = sqlite3_bind_int64(this->m_stmt, this->m_current_column++, n);
    if (status != SQLITE_OK) {
        throw DBException(status, "DbOstream::add(DbInt64)");
    }
}

void DbOstream::add(DbDouble d) {
    int status = sqlite3_bind_double(this->m_stmt, this->m_current_column++, d);
    if (status != SQLITE_OK) {
        throw DBException(status, "DbOstream::add(DbDouble)");
    }
}

void DbOstream::flush() {
    knight_assert_msg(this->m_current_column == this->m_columns + 1,
                      "incomplete row");

    int status = sqlite3_step(this->m_stmt);
    if (status != SQLITE_DONE) {
        throw DBException(status, "DbOstream::flush(): step failed");
    }

    status = sqlite3_clear_bindings(this->m_stmt);
    if (status != SQLITE_OK) {
        throw DBException(status, "DbOstream::flush(): clear bindings failed");
    }

    status = sqlite3_reset(this->m_stmt);
    if (status != SQLITE_OK) {
        throw DBException(status, "DbOstream::flush(): reset failed");
    }

    this->m_current_column = 1;
    this->m_db.row_inserted();
}

// DbIstream

DbIstream::DbIstream(DbConnection& db, std::string query)
    : m_db(db), m_query(std::move(query)) {
    int status = sqlite3_prepare_v2(this->m_db.m_handle,
                                    this->m_query.c_str(),
                                    -1,
                                    &this->m_stmt,
                                    nullptr);
    if (status != SQLITE_OK) {
        throw DBException(status,
                          "DbIstream: cannot prepare query '" + this->m_query +
                              "' in database " + this->m_db.filename());
    }

    this->step();

    this->m_columns = sqlite3_column_count(m_stmt);
    if (this->m_columns == 0) {
        throw DBException(SQLITE_MISMATCH,
                          "DbIstream: malformed query '" + this->m_query +
                              "': no column in result");
    }

    this->m_current_column = 0;
}

DbIstream::~DbIstream() {
    // The destructor shall not throw an exception. No error check.
    sqlite3_finalize(this->m_stmt);
}

void DbIstream::step() {
    int status = sqlite3_step(this->m_stmt);
    if (status == SQLITE_DONE) {
        this->m_done = true;
    } else if (status != SQLITE_ROW) {
        throw DBException(status, "DbIstream::step(): query " + this->m_query);
    }
}

void DbIstream::skip_column() {
    this->m_current_column++;
    if (!this->m_done && (this->m_current_column == this->m_columns)) {
        this->step();
        this->m_current_column = 0;
    }
}

DbIstream& operator>>(DbIstream& i, std::string& s) {
    if (i.empty()) {
        throw DBException(SQLITE_MISUSE,
                          "DbIstream::>>: no more data for query '" +
                              i.query() + "'");
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    s.assign(reinterpret_cast< const char* >(
        sqlite3_column_text(i.m_stmt, i.m_current_column)));
    i.skip_column();
    return i;
}

DbIstream& operator>>(DbIstream& i, DbInt64& n) {
    if (i.empty()) {
        throw DBException(SQLITE_MISUSE,
                          "DbIstream::>>: no more data for query '" +
                              i.query() + "'");
    }
    n = sqlite3_column_int64(i.m_stmt, i.m_current_column);
    i.skip_column();
    return i;
}

DbIstream& operator>>(DbIstream& i, DbDouble& d) {
    if (i.empty()) {
        throw DBException(SQLITE_MISUSE,
                          "DbIstream::>>: no more data for query '" +
                              i.query() + "'");
    }
    d = sqlite3_column_double(i.m_stmt, i.m_current_column);
    i.skip_column();
    return i;
}

} // namespace knight::sqlite
