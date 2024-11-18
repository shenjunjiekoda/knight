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

#include <exception>
#include <memory>
#include <string>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>

#include "util/assert.hpp"

#include <sqlite3.h>

namespace knight::sqlite {

constexpr unsigned DBExceptionAlignment = 16U;
/// \brief Maximum number of rows per transaction, in CommitPolicy::Auto
constexpr unsigned MaxRowsPerTransaction = 8192;

// NOLINTNEXTLINE
struct DBException : std::exception {
    /// \brief Code Constructor
    explicit DBException(int code);
    /// \brief Code and Context Constructor
    DBException(int code, const std::string& context);
    /// \brief Copy constructor
    DBException(const DBException&) noexcept = default;
    /// \brief Move constructor
    DBException(DBException&&) noexcept = default;
    /// \brief Copy assignment operator
    DBException& operator=(const DBException&) noexcept = default;
    /// \brief Move assignment operator
    DBException& operator=(DBException&&) noexcept = default;
    /// \brief Destructor
    ~DBException() override = default;

    /// \brief Get the error code
    [[nodiscard]] const char* error_msg() const noexcept;
    /// \brief Get the error message
    [[nodiscard]] const char* what() const noexcept override;

  private:
    /// \brief Error code
    int m_code;
    /// \brief Context messagge
    std::shared_ptr< const std::string > m_context;
    /// \brief Computed message for what() method
    std::shared_ptr< const std::string > m_what;
} __attribute__((aligned(DBExceptionAlignment))); // struct DBException

/// \brief Integer type for SQLite
using DbInt64 = sqlite3_int64;

/// \brief Double type for SQLite
using DbDouble = double;

/// \brief Column type
enum class DbColumnType { Text, Integer, Real, Blob };

/// \brief Journal mode
enum class JournalMode { Delete, Truncate, Persist, Memory, WAL, Off };

/// \brief Synchronous mode
enum class SynchronousFlag { Off, Normal, Full, Extra };

/// \brief Commit policy for a DbConnection
enum class CommitPolicy {
    /// \brief Call begin_transaction() and commit_transaction() manually
    Manual = 0,

    /// \brief Automatically start new transactions every MaxRowsPerTransaction
    /// inserted rows
    Auto = 1,
};

/// \brief SQLite connection
class DbConnection {
  private:
    /// \brief Filename
    std::string m_filename;

    /// \brief SQLite3 handle
    sqlite3* m_handle = nullptr;

    /// \brief Commit policy
    CommitPolicy m_commit_policy = CommitPolicy::Manual;

    /// \brief Number of inserted rows, in CommitPolicy::Auto
    std::size_t m_inserted_rows = 0;

  public:
    /// \brief No default constructor
    DbConnection() = delete;

    /// \brief Open a database connection
    explicit DbConnection(std::string filename);

    /// \brief No copy constructor
    DbConnection(const DbConnection&) = delete;

    /// \brief No move constructor
    DbConnection(DbConnection&&) = delete;

    /// \brief No copy assignment operator
    DbConnection& operator=(const DbConnection&) = delete;

    /// \brief No move assignment operator
    DbConnection& operator=(DbConnection&&) = delete;

    /// \brief Destructor
    ~DbConnection();

    /// \brief Return the database filename
    [[nodiscard]] const std::string& filename() const {
        return this->m_filename;
    }

    /// \brief Execute a SQL command
    void exec_command(const char* cmd);

    /// \brief Execute a SQL command
    void exec_command(const std::string& cmd);

    /// \brief Start a transaction
    void begin_transaction();

    /// \brief Commit a transaction
    void commit_transaction();

    /// \brief Rollback a transaction
    void rollback_transaction();

    /// \brief Change the commit policy
    void set_commit_policy(CommitPolicy policy);

    /// \brief Return the current commit policy
    [[nodiscard]] CommitPolicy commit_policy() const {
        return this->m_commit_policy;
    }

  private:
    /// \brief Called upon a row insertion
    void row_inserted();

  public:
    /// \brief Remove a table
    void drop_table(llvm::StringRef name);

    /// \brief Return the last inserted row ID
    [[nodiscard]] DbInt64 last_insert_rowid() const;

    /// \brief Create a table with the given column names and types
    ///
    /// If a column is called "id", automatically mark it as a primary key.
    void create_table(
        llvm::StringRef name,
        llvm::ArrayRef< std::pair< llvm::StringRef, DbColumnType > > columns);

    /// \brief Create an index on the given column of the given table
    void create_index(llvm::StringRef index,
                      llvm::StringRef table,
                      llvm::StringRef column);

    /// \brief Set the journal mode
    void set_journal_mode(JournalMode mode);

    /// \brief Set the synchronous flag
    void set_synchronous_flag(SynchronousFlag flag);

    // friends
    friend class DbOstream;
    friend class DbIstream;

}; // end class DbConnection

/// \brief Stream-based interface for populating tables
class DbOstream {
  private:
    /// \brief Database connection
    DbConnection& m_db;

    /// \brief SQLite3 prepared statement
    sqlite3_stmt* m_stmt = nullptr;

    /// \brief Number of columns
    int m_columns;

    /// \brief Current number of column entered
    int m_current_column = 1;

  public:
    /// \brief No default constructor
    DbOstream() = delete;

    /// \brief Constructor
    ///
    /// \param db The database connection
    /// \param table_name The table name
    /// \param columns Number of columns
    DbOstream(DbConnection& db, llvm::StringRef table_name, int columns);

    /// \brief No copy constructor
    DbOstream(const DbOstream&) = delete;

    /// \brief No move constructor
    DbOstream(DbOstream&&) = delete;

    /// \brief No copy assignment operator
    DbOstream& operator=(const DbOstream&) = delete;

    /// \brief No move assignment operator
    DbOstream& operator=(DbOstream&&) = delete;

    /// \brief Destructor
    ~DbOstream();

  public:
    /// \brief Insert a string
    void add(llvm::StringRef s);

    /// \brief Insert NULL
    void add_null();

    /// \brief Insert an integer
    void add(DbInt64 n);

    /// \brief Insert a double
    void add(DbDouble d);

    /// \brief Flush the row
    void flush();

    // friends
    friend DbOstream& end_row(DbOstream& o);

}; // end class DbOstream

/// \brief Insert a string
inline DbOstream& operator<<(DbOstream& o, llvm::StringRef s) {
    o.add(s);
    return o;
}

/// \brief Mark the end of a row
inline DbOstream& end_row(DbOstream& o) {
    o.flush();
    return o;
}

/// \brief Insert NULL
inline DbOstream& null(DbOstream& o) {
    o.add_null();
    return o;
}

/// \brief Insert an integer
inline DbOstream& operator<<(DbOstream& o, DbInt64 n) {
    o.add(n);
    return o;
}

/// \brief Insert an double
inline DbOstream& operator<<(DbOstream& o, DbDouble d) {
    o.add(d);
    return o;
}

/// \brief Insert sqlite::end_row or sqlite::null
inline DbOstream& operator<<(DbOstream& o, DbOstream& (*m)(DbOstream&)) {
    if (m == &end_row) {
        o.flush();
    } else if (m == &null) {
        o.add_null();
    } else {
        knight_unreachable("invalid function pointer argument");
    }
    return o;
}

/// \brief Stream-based interface for retrieving results of a SQL query
class DbIstream {
  private:
    /// \brief Database connection
    DbConnection& m_db;

    /// \brief SQLite3 prepared statement
    sqlite3_stmt* m_stmt = nullptr;

    /// \brief SQL query
    std::string m_query;

    /// \brief Are we done ready?
    bool m_done = false;

    /// \brief Number of columns
    int m_columns;

    /// \brief Current number of column retrieved
    int m_current_column;

  public:
    /// \brief No default constructor
    DbIstream() = delete;

    /// \brief Constructor
    ///
    /// \param db The database connection
    /// \param query SQL query
    DbIstream(DbConnection& db, std::string query);

    /// \brief No copy constructor
    DbIstream(const DbIstream&) = delete;

    /// \brief No copy constructor
    DbIstream(DbIstream&&) = delete;

    /// \brief No copy assignment operator
    DbIstream& operator=(const DbIstream&) = delete;

    /// \brief No move assignment operator
    DbIstream& operator=(DbIstream&&) = delete;

    /// \brief Destructor
    ~DbIstream();

    /// \brief Return the SQL query
    [[nodiscard]] const std::string& query() const { return this->m_query; }

    /// \brief Is the stream empty?
    [[nodiscard]] bool empty() const { return m_done; }

  private:
    /// \brief Retrieve the next row
    void step();

    /// \brief Retrieve the next column
    void skip_column();

    /// \brief Read a string
    friend DbIstream& operator>>(DbIstream&, std::string&);

    /// \brief Read an integer
    friend DbIstream& operator>>(DbIstream&, DbInt64&);

    /// \brief Read a double
    friend DbIstream& operator>>(DbIstream&, DbDouble&);

}; // class DbIstream

} // namespace knight::sqlite