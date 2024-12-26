//===- sqlite3.hpp -------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the interface of the SQLite3 library.
//
//===------------------------------------------------------------------===//

#include <array>
#include <cstdint>
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include "common/util/assert.hpp"
#include "common/util/export.hpp"

#include <llvm/Support/raw_ostream.h>

#include <sqlite3.h>

namespace knight::sqlite {

constexpr unsigned DBHeaderStringSize = 16U;
constexpr unsigned DBHeaderPaddingSize = 20U;

class Database;
class PreparedStmt;
using PreparedInternStmt = sqlite3_stmt;
using PreparedInternStmtRef = std::shared_ptr< PreparedInternStmt >;

constexpr const char* const SqliteVersion = SQLITE_VERSION;
constexpr const int SqliteVersionNumber = SQLITE_VERSION_NUMBER;

constexpr int SqliteOK = SQLITE_OK;

const char* error_code_message(int code) noexcept;

class KNIGHT_API ResColumn {
  public:
    enum ColumnKind : unsigned {
        INTEGER = SQLITE_INTEGER, // 1
        FLOAT = SQLITE_FLOAT,     // 2
        TEXT = SQLITE_TEXT,       // 3
        BLOB = SQLITE_BLOB,       // 4
        NULL_ = SQLITE_NULL       // 5
    };

  private:
    PreparedInternStmtRef m_stmt;
    unsigned m_idx;

  public:
    explicit ResColumn(PreparedInternStmtRef stmt, unsigned idx) noexcept
        : m_stmt(std::move(stmt)), m_idx(idx) {}

  public:
    [[nodiscard]] std::string get_name() const noexcept;

    [[nodiscard]] int32_t get_as_int() const noexcept;

    [[nodiscard]] int64_t get_as_int64() const noexcept;

    [[nodiscard]] double get_as_double() const noexcept;

    [[nodiscard]] const char* get_as_text() const noexcept;

    [[nodiscard]] std::string get_as_string() const noexcept;

    [[nodiscard]] const void* get_as_blob() const noexcept;

    [[nodiscard]] unsigned get_num_bytes() const noexcept;

    [[nodiscard]] ResColumn::ColumnKind get_value_type() const noexcept;

    [[nodiscard]] bool is_null() const noexcept {
        return get_value_type() == ResColumn::NULL_;
    }

    [[nodiscard]] bool is_integer() const noexcept {
        return get_value_type() == ResColumn::INTEGER;
    }

    [[nodiscard]] bool is_float() const noexcept {
        return get_value_type() == ResColumn::FLOAT;
    }

    [[nodiscard]] bool is_text() const noexcept {
        return get_value_type() == ResColumn::TEXT;
    }

    [[nodiscard]] bool is_blob() const noexcept {
        return get_value_type() == ResColumn::BLOB;
    }

}; // class Column

KNIGHT_API inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                                const ResColumn& col) {
    return os.write(col.get_as_text(), col.get_num_bytes());
}

const char* column_type_str(knight::sqlite::ResColumn::ColumnKind type);

/// \brief SQLite prepared statement wrapper.
class KNIGHT_API PreparedStmt {
  public:
    /** \brief Constructor.
     * \param[in] db the SQLite Database Connection
     * \param[in] sql an UTF-8 encoded query string
     *
     * \throw std::runtime_error if the error occurs.
     */
    PreparedStmt(const Database& db, std::string sql);
    PreparedStmt(const PreparedStmt&) = delete;
    PreparedStmt& operator=(const PreparedStmt&) = delete;

    PreparedStmt(PreparedStmt&& aStatement) noexcept;
    PreparedStmt& operator=(PreparedStmt&& aStatement) = default;

    ~PreparedStmt() = default;

  public:
    /// \brief Reset the statement to make it ready for a new execution
    ///
    /// * Call this function before any news calls to bind() if the statement
    /// was already executed before.
    /// * Calling reset() does not clear the bindings (see clearBindings()).
    ///
    /// \throw std::run_time_error if the error occurs.
    void reset() noexcept(false);

    /// \brief Reset the statement.
    /// \returns the sqlite result code
    [[nodiscard]] int try_reset() noexcept;

    ///
    /// \brief Clears away all the bindings of a prepared statement.
    ///
    /// Contrary to the intuition of many, reset() does not reset the bindings
    /// on a prepared statement. Use this routine to reset all parameters to
    /// NULL.
    ///
    /// \throw std::run_time_error if the error occurs.
    void clear_bindings();

    [[nodiscard]] KNIGHT_PURE_FUNC int get_idx(const std::string& name) const;

    /// \brief Bind an value to a parameter "?", "?NNN", ":VVV", "\VVV" or
    /// "$VVV" in the SQL prepared statement (idx >= 1)
    // \\{
    void bind(int idx, int32_t val);
    void bind(int idx, uint32_t val);
    void bind(int idx, int64_t val);
    void bind(int idx, double val);
    void bind(int idx, const std::string& val);
    void bind(int idx, const void* val, int size);
    void bind_null(int idx);
    void bind_no_copy(int idx, const std::string& val);
    void bind_no_copy(int idx, std::string&& val) = delete;
    void bind_no_copy(int idx, const void* val, int size);

    void bind(const std::string& name, int32_t val) {
        bind(get_idx(name), val);
    }
    void bind(const std::string& name, uint32_t val) {
        bind(get_idx(name), val);
    }
    void bind(const std::string& name, int64_t val) {
        bind(get_idx(name), val);
    }
    void bind(const std::string& name, double val) { bind(get_idx(name), val); }
    void bind(const std::string& name, const std::string& val) {
        bind(get_idx(name), val);
    }
    void bind(const std::string& name, const void* val, int size) {
        bind(get_idx(name), val, size);
    }
    void bind_null(const std::string& name) { bind_null(get_idx(name)); }
    void bind_no_copy(const std::string& name, const std::string& val) {
        bind_no_copy(get_idx(name), val);
    }
    void bind_no_copy(const std::string& name, std::string&& val) = delete;
    void bind_no_copy(const std::string& name, const void* val, int size) {
        bind_no_copy(get_idx(name), val, size);
    }

    /// PreparedStmt stmt("SELECT * FROM tableX WHERE colA=? && colB=? &&
    /// colC=?");
    /// stmt.bind(a,b,c);
    /// //...is equivalent to
    /// stmt.bind(1,a);
    /// stmt.bind(2,b);
    /// stmt.bind(3,c);
    template < class... Args >
    void bind_multiple(const Args&... args) {
        int pos = 0;
        (void)std::initializer_list< int >{
            ((void)this->bind(++pos, std::forward< decltype(args) >(args)),
             0)...};
    }

    template < typename... Types >
    void bind_multiple(const std::tuple< Types... >& tuple) {
        this->bind_multiple(tuple, std::index_sequence_for< Types... >());
    }

    template < typename... Types, std::size_t... Indices >
    void bind_multiple(const std::tuple< Types... >& tuple,
                       std::index_sequence< Indices... >) {
        this->bind_multiple(std::get< Indices >(tuple)...);
    }

    template < typename TupleT >
    void bind_execute(TupleT&& tuple) {
        this->bind_multiple(std::forward< TupleT >(tuple));
        while (this->execute_step()) {
        }
    }

    template < typename TupleT >
    void reset_bind_execute(TupleT&& tuple) {
        this->reset();
        this->bind_execute(std::forward< TupleT >(tuple));
    }

    /// \}

    ///
    /// \brief Execute a step of the prepared query to fetch one row of
    /// results.
    ///
    ///  While true is returned, a row of results is available, and can be
    /// accessed through the get_column() method
    ///
    /// \returns true (SQLITE_ROW) if there is another row ready : you can
    ///            call get_column(N) to get it then you have to call
    ///            execute_step() again to fetch more rows until the query
    ///            is finished
    /// \returns false (SQLITE_DONE) if the query has finished executing :
    ///         there is no (more) row of result (case of a query with no
    ///         result, or after N rows fetched successfully)
    ///
    /// \throw std::runtime_error if the error occurs.
    ///
    [[nodiscard]] bool execute_step();

    ///
    /// \brief Try to execute a step of the prepared query to fetch one row
    /// of results, returning the sqlite result code.
    ///
    /// \see exec() execute a one-step prepared statement with no expected
    /// result \see executeStep() execute a step of the prepared query to
    /// fetch one row of results \see Database::exec() is a shortcut to
    /// execute one or multiple statements without results
    ///
    /// \return the sqlite result code.
    [[nodiscard]] int try_execute_step() noexcept;

    ///
    /// \brief NON-query statement execution.
    ///
    /// It is similar to Database::execute(), but using a precompiled
    /// statement, it adds :
    /// - the ability to bind() arguments to it (best way to insert data),
    /// - reusing it allows for better performances (efficient for multiple
    /// insertion).
    [[nodiscard]] int execute();

    ///
    /// \brief Return a column result data corresponding to the specified
    /// index
    ///
    ///  use after the execute_step() method returns true.
    ///
    /// \param[in] idx Column index (starting at 0)
    ///
    /// \return a column result specified by its index
    ///
    /// \throw std::runtime_error if the error occurs.
    ///
    [[nodiscard]] ResColumn get_column(int idx) const;

    ///
    /// \brief Return a column result data corresponding to the specified
    /// index
    ///
    ///  use after the execute_step() method returns true.
    ///
    /// \param[in] name Column name (potential aliased name, not origin
    /// name)
    ///
    /// \return a column result specified by its index
    ///
    /// \throw std::runtime_error if the error occurs.
    ///
    [[nodiscard]] ResColumn get_column(const std::string& name) const;

    template < typename Cols, int N >
    [[nodiscard]] Cols get_columns_as() {
        validate_row_to_fetched();
        validate_col_idx(N - 1);
        return get_columns< Cols >(std::make_integer_sequence< int, N >{});
    }

    [[nodiscard]] int get_column_index(const char* name) const noexcept(false);
    [[nodiscard]] const char* get_column_name(int idx) const;
    [[nodiscard]] const char* get_column_origin_name(int idx) const;
    [[nodiscard]] const char* get_column_declared_type(int idx) const
        noexcept(false);
    [[nodiscard]] int get_last_changes() const noexcept;
    [[nodiscard]] int get_bind_parameter_count() const noexcept;

    [[nodiscard]] bool is_column_null(int idx) const;
    [[nodiscard]] bool is_column_null(const char* name) const;

    [[nodiscard]] int get_error_code() const noexcept;
    [[nodiscard]] int get_extended_error_code() const noexcept;
    [[nodiscard]] std::string get_error_msg() const noexcept;

    [[nodiscard]] std::string get_expanded_sql() const;

  public:
    [[nodiscard]] std::string get_sql() const noexcept { return m_sql; }
    [[nodiscard]] int get_result_column_count() const noexcept {
        return m_res_col_cnt;
    }

    [[nodiscard]] bool does_rows_fetched_done() const noexcept {
        return m_rows_done;
    }

    [[nodiscard]] bool has_row_to_fetched() const noexcept {
        return m_row_to_fetched;
    }

  private:
    sqlite3* m_handle;

    std::string m_sql;
    PreparedInternStmtRef m_prepared_intern_stmt;

    int m_res_col_cnt = 0;
    bool m_row_to_fetched = false;
    bool m_rows_done = false;

    mutable std::unordered_map< std::string, int > m_col_name_idx_cache;

  private:
    PreparedInternStmtRef get_prepare_intern_statement_ref();
    PreparedInternStmt* get_prepare_intern_statement() const;

    void throw_db_error(int ret, const std::string& extra_msg = "") const
        noexcept(false);
    void validate_return(int ret) const noexcept(false);
    void validate_row_to_fetched() const;
    void validate_col_idx(int idx) const;

    template < typename T, const int... Is >
    T get_columns(const std::integer_sequence< int, Is... >) {
        return T{ResColumn(m_prepared_intern_stmt, Is)...};
    }

}; // class PreparedQueryStmt

enum class TransactionMode {
    DEFERRED,
    IMMEDIATE,
    EXCLUSIVE,
}; // enum class TransactionMode

class KNIGHT_API Transaction {
  public:
    explicit Transaction(Database& db) noexcept(false);

    Transaction(Database& db, TransactionMode behavior) noexcept(false);

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    // rollback when not committed.
    ~Transaction() noexcept;

  public:
    void commit() noexcept(false);
    void rollback() noexcept(false);

  private:
    Database& m_db;
    bool m_committed = false;

}; // class Transaction

/// \brief SQLite database connection open modes
enum KNIGHT_API OpenMode : unsigned {
    /// The database is opened in read-only mode. If the database does not
    /// already exist, an error is returned.
    READONLY = SQLITE_OPEN_READONLY,
    /// The database is opened for reading and writing if possible, or
    /// reading
    /// only if the file is write protected
    /// by the operating system. In either case the database must already
    /// exist,
    /// otherwise an error is returned.
    READWRITE = SQLITE_OPEN_READWRITE,
    /// With OPEN_READWRITE: The database is opened for reading and writing,
    /// and
    /// is created if it does not already exist.
    CREATE = SQLITE_OPEN_CREATE,
    /// Enable URI filename interpretation, parsed according to RFC 3986
    /// (ex.
    /// "file:data.db?mode=ro&cache=private")
    URI = SQLITE_OPEN_URI,
    /// Open in memory database
    MEMORY = SQLITE_OPEN_MEMORY,
    /// Open database in multi-thread threading mode
    NOMUTEX = SQLITE_OPEN_NOMUTEX,
    /// Open database with thread-safety in serialized threading mode
    FULLMUTEX = SQLITE_OPEN_FULLMUTEX,
    /// Open database with shared cache enabled
    SHAREDCACHE = SQLITE_OPEN_SHAREDCACHE,
    /// Open database with shared cache disabled
    PRIVATECACHE = SQLITE_OPEN_PRIVATECACHE,
    /// Database filename is not allowed to be a symbolic link (Note: only
    /// since
    /// SQlite 3.31.0 from 2020-01-22)
    NOFOLLOW = SQLITE_OPEN_NOFOLLOW
};

const char* get_lib_version() noexcept;
int get_lib_version_number() noexcept;

/// \brief SQLite DBFile header
///
/// \see https://www.sqlite.org/fileformat.html#the_database_header
#pragma pack(push, 1)
struct DBHeader { // NOLINT
    // Offset 0: The header string "SQLite format 3\000"
    char header_str[DBHeaderStringSize]; // NOLINT
    // Offset 16: The database page size in bytes
    uint16_t page_size_bytes;
    // Offset 18: File format write version
    uint8_t file_format_write_version;
    // Offset 19: File format read version
    uint8_t file_format_read_version;
    // Offset 20: Bytes of unused "reserved" space at the end of each page
    uint8_t reserved_space_bytes;
    // Offset 21: Maximum embedded payload fraction
    uint8_t max_embedded_payload_frac;
    // Offset 22: Minimum embedded payload fraction
    uint8_t min_embedded_payload_frac;
    // Offset 23: Leaf payload fraction
    uint8_t leaf_payload_frac;
    // Offset 24: File change counter
    uint32_t file_change_counter;
    // Offset 28: Size of the database file in pages
    uint32_t database_size_pages;
    // Offset 32: Page number of the first freelist trunk page
    uint32_t first_freelist_trunk_page;
    // Offset 36: Total number of freelist pages
    uint32_t total_freelist_pages;
    // Offset 40: The schema cookie
    uint32_t schema_cookie;
    // Offset 44: The schema format number
    uint32_t schema_format_number;
    // Offset 48: Default page cache size
    uint32_t default_page_cache_size_bytes;
    // Offset 52: Page number of the largest root b-tree page
    uint32_t largest_b_tree_page_number;
    // Offset 56: The database text encoding
    uint32_t database_text_encoding;
    // Offset 60: The "user version"
    uint32_t user_version;
    // Offset 64: True for incremental-vacuum mode
    uint32_t incremental_vacuum_mode;
    // Offset 68: The "Application ID"
    uint32_t application_id;
    // Offset 72: Reserved for expansion
    std::array< uint8_t, DBHeaderPaddingSize > paddings;
    // Offset 92: The version-valid-for number
    uint32_t version_valid_for;
    // Offset 96: SQLITE_VERSION_NUMBER
    uint32_t sqlite_version;
}; // struct DBHeader
#pragma pack(pop)

/// \brief RAII management of a SQLite Database Connection.
///
/// \note This class is not thread-safe itself.
class KNIGHT_API Database {
    friend class PreparedStmt;

  private:
    struct HandleCloser {
        KNIGHT_API void operator()(sqlite3* handle);
    };
    std::unique_ptr< sqlite3, HandleCloser > m_sqlite3_handle;
    std::string m_file;

  public:
    ///
    /// \brief Open the database.
    ///
    /// Uses `sqlite3_open_v2()` with `readonly` default flag, which is the
    /// opposite behavior of the old sqlite3_open() function
    /// (READWRITE+CREATE).
    ///
    /// This makes sense if you want to use it on a readonly filesystem
    /// or to prevent creation of a void file when a required file is
    /// missing.
    ///
    /// \param[in] file The database filename.
    /// \param[in] flags The flags to use for opening the database.
    /// \param[in] busy_timeout_milliseconds The busy timeout in
    /// milliseconds. \param[in] vfs The name of the VFS to use for the
    /// database, empty for default.
    ///
    /// \throws std::runtime_error if error occurs during opening the
    /// database.
    ///
    ///
    explicit Database(const std::string& file,
                      int flags = READONLY,
                      int busy_timeout_milliseconds = 0,
                      const std::string& vfs = "") noexcept(false);

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    Database(Database&& aDatabase) = default;
    Database& operator=(Database&& aDatabase) = default;

    ~Database() = default;

  public:
    ///
    /// \brief Set busy timeout for the database when it is locked.
    ///
    /// \throw std::runtime_error if error occurs during setting busy
    /// timeout.
    ///
    void set_busy_timeout(int milliseconds) const noexcept(false);

    ///
    /// \brief execute one or multiple non-query statements.
    ///
    ///  Other than (DQL) "SELECT" can be used with this method, including:
    ///  - Data Manipulation Language (DML): "INSERT", "UPDATE" and "DELETE"
    ///  - Data Definition Language (DDL): "CREATE", "ALTER" and "DROP"
    ///  - Data Control Language (DCL): "GRANT", "REVOKE", "COMMIT" and
    /// "ROLLBACK"
    ///
    /// \see Database::try_exec() to execute, returning the sqlite result
    /// code \see Statement::exec() to handle precompiled statements (for
    /// better performances) without results \see Statement::executeStep()
    /// to handle "SELECT" queries with results
    ///
    /// \param[in] sqls one or multiple UTF-8 encoded, semicolon-separate
    /// SQL statements
    ///
    /// \return number of rows modified by the *last* INSERT, UPDATE or
    /// DELETE statement (beware of multiple statements) \warning undefined
    /// for CREATE or DROP table: returns the value of a previous INSERT,
    /// UPDATE or DELETE statement.
    ///
    /// \throw std::runtime_error if error occurs during execution.
    ///
    [[nodiscard]] int execute(const std::string& sqls) const noexcept(false);

    ///
    /// \brief Try to execute one or multiple statements (including
    /// validation of the sqls execution).
    ///
    /// \param[in] sqls one or multiple UTF-8 encoded, semicolon-separate
    /// SQL statements \param[out] invalid true if the sqls execution is
    /// invalid
    ///
    /// \return number of rows modified by the *last* INSERT, UPDATE or
    /// DELETE statement (beware of multiple statements) \warning undefined
    /// for CREATE or DROP table: returns the value of a previous INSERT,
    /// UPDATE or DELETE statement.
    ///
    ///
    [[nodiscard]] int try_execute(const std::string& sqls,
                                  bool& invalid) const noexcept;
    ///
    /// \brief Try to execute one or multiple statements
    ///
    ///  Other than (DQL) "SELECT" can be used with this method, including:
    ///  - Data Manipulation Language (DML): "INSERT", "UPDATE" and "DELETE"
    ///  - Data Definition Language (DDL): "CREATE", "ALTER" and "DROP"
    ///  - Data Control Language (DCL): "GRANT", "REVOKE", "COMMIT" and
    /// "ROLLBACK"
    ///
    /// \see exec() to execute, returning number of rows modified
    ///
    /// \param[in] sqls  one or multiple UTF-8 encoded, semicolon-separate
    /// SQL statements
    ///
    /// \return the sqlite result code.
    ///
    [[nodiscard]] int try_execute(const std::string& sqls) const noexcept;

    ///
    /// \brief Shortcut to query and fetch the first column result.
    ///
    /// \see also Statement class for handling queries with multiple results
    ///
    /// \param[in] sql query sql
    ///
    /// \return a temporary ResColumn object with the first column of
    /// result.
    ///
    /// \throw std::runtime_error if error occurs.
    ///
    [[nodiscard]] ResColumn exec_and_get_first(const std::string& sql) const
        noexcept(false);

    template < typename Arg, typename... Types >
    void execute_multiple(const std::string& sql,
                          Arg&& aArg,
                          Types&&... aParams) {
        PreparedStmt query(*this, sql);
        query.bind_execute(std::forward< Arg >(aArg));
        (void)std::initializer_list< int >{
            ((void)query.reset_bind_execute(std::forward< Types >(aParams)),
             0)...};
    }

    [[nodiscard]] bool table_exists(
        const std::string& table_name) const noexcept;

    ///
    /// \brief Get the row-id of the most recent successful INSERT into the
    /// database from the *current connection*.
    ///
    /// Each entry in an SQLite table always has a unique 64-bit signed
    /// integer key called the row-id. If the table has a column of type
    /// INTEGER PRIMARY KEY, then it is an alias for the rowid.
    [[nodiscard]] int64_t get_last_insert_rowid() const noexcept;

    [[nodiscard]] int get_last_dml_changes() const noexcept;

    [[nodiscard]] int get_total_dml_changes() const noexcept;

    [[nodiscard]] DBHeader parse_header(const std::string& file) const
        noexcept(false);

    // Set the key for the current sqlite database instance.
    static void set_key(const std::string& key);

    static void reset_key(const std::string& key);

    [[nodiscard]] static bool is_unencrypted(const std::string& file) noexcept(
        false);

  public:
    [[nodiscard, gnu::returns_nonnull]] sqlite3* get_handle() const noexcept;
    [[nodiscard]] const std::string& get_file() const noexcept {
        return m_file;
    }

    [[nodiscard]] int get_error_code() const noexcept;

    [[nodiscard]] int get_extended_error_code() const noexcept;

    [[nodiscard]] std::string get_error_msg() const noexcept;

  private:
    void validate_return(int ret) const noexcept(false);

}; // class Database

} // namespace knight::sqlite