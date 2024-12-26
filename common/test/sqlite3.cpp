#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include <llvm/ADT/StringRef.h>

#include "common/util/sqlite3.hpp"

using namespace knight::sqlite;

Database get_rw_and_create_db(const std::string& db_name) {
    return Database(db_name, OpenMode::READWRITE | OpenMode::CREATE);
}

#define DBName "mamba_out"
#define TableName "james_out"
#define MemoryDB ":memory:"
#define CreateTableDefaultSql \
    "CREATE TABLE " TableName " (id INTEGER PRIMARY KEY, value TEXT)"
#define CreateMemoryDB Database db(MemoryDB, OpenMode::READWRITE);
#define DropTableIfExistSql "DROP TABLE IF EXISTS " TableName
#define PragmaBusyTimeoutSql "PRAGMA busy_timeout"
#define InsertDefaultTableWithValue(Val) \
    "INSERT INTO " TableName " VALUES (NULL, '" Val "')"
#define SelectAllSqlFromDefaultTable "SELECT * FROM " TableName

TEST(Database, CreateAndDropTable) {
    (void)remove(DBName);
    {
        // Try to open a non-existing database
        EXPECT_THROW(::Database not_found(DBName), std::runtime_error);

        Database db = get_rw_and_create_db(DBName);

        EXPECT_STREQ(DBName, db.get_file().c_str());
        EXPECT_FALSE(db.table_exists(TableName));
        EXPECT_EQ(0, db.get_last_insert_rowid());

        (void)db.execute(CreateTableDefaultSql);
        EXPECT_TRUE(db.table_exists(TableName));
        EXPECT_EQ(0, db.get_last_insert_rowid());

        (void)db.execute(DropTableIfExistSql);
        EXPECT_FALSE(db.table_exists(TableName));
        EXPECT_EQ(0, db.get_last_insert_rowid());
    }
    EXPECT_EQ(0, remove(DBName));
}

TEST(Database, CreateAndReopen) {
    (void)remove(DBName);
    {
        EXPECT_THROW(Database not_found(DBName), std::runtime_error);
        Database db = get_rw_and_create_db(DBName);
        EXPECT_FALSE(db.table_exists(TableName));
        (void)db.execute(CreateTableDefaultSql);
        EXPECT_TRUE(db.table_exists(TableName));
    }
    {
        Database db = get_rw_and_create_db(DBName);
        EXPECT_TRUE(db.table_exists(TableName));
    }
    (void)remove(DBName);
}

TEST(Database, TempTableInMemory) {
    {
        CreateMemoryDB;
        EXPECT_FALSE(db.table_exists(TableName));
        (void)db.execute(CreateTableDefaultSql);
        EXPECT_TRUE(db.table_exists(TableName));
        Database db2(MemoryDB);
        EXPECT_FALSE(db2.table_exists(TableName));
    }
    {
        Database db(MemoryDB);
        EXPECT_FALSE(db.table_exists(TableName));
    }
}

TEST(Database, DBSetBusyTimeout) {
    {
        Database db(MemoryDB);
#define DEFAULT_BUSY_TIMEOUT 0
        EXPECT_EQ(DEFAULT_BUSY_TIMEOUT,
                  db.exec_and_get_first(PragmaBusyTimeoutSql).get_as_int());
#undef DEFAULT_BUSY_TIMEOUT

        db.set_busy_timeout(9527); // NOLINT
        EXPECT_EQ(9527,
                  db.exec_and_get_first(PragmaBusyTimeoutSql).get_as_int());

        db.set_busy_timeout(0);
        EXPECT_EQ(0, db.exec_and_get_first(PragmaBusyTimeoutSql).get_as_int());
    }
    {
        Database db(MemoryDB, OpenMode::READWRITE, 9527);
        EXPECT_EQ(9527,
                  db.exec_and_get_first(PragmaBusyTimeoutSql).get_as_int());

        db.set_busy_timeout(0);
        EXPECT_EQ(0, db.exec_and_get_first(PragmaBusyTimeoutSql).get_as_int());
    }
    {
        Database db(MemoryDB, OpenMode::READWRITE, 9527);
        EXPECT_EQ(9527,
                  db.exec_and_get_first(PragmaBusyTimeoutSql).get_as_int());

        db.set_busy_timeout(0);
        EXPECT_EQ(0, db.exec_and_get_first(PragmaBusyTimeoutSql).get_as_int());
    }
}

TEST(Database, DBExecute) {
    CreateMemoryDB;

    (void)db.execute(CreateTableDefaultSql);
    EXPECT_EQ(0, db.get_last_dml_changes());
    EXPECT_EQ(0, db.get_last_insert_rowid());
    EXPECT_EQ(0, db.get_total_dml_changes());

    EXPECT_EQ(1, db.execute(InsertDefaultTableWithValue("first")));
    EXPECT_EQ(1, db.get_last_dml_changes());
    EXPECT_EQ(1, db.get_last_insert_rowid());
    EXPECT_EQ(1, db.get_total_dml_changes());

    EXPECT_EQ(1, db.execute(InsertDefaultTableWithValue("second")));
    EXPECT_EQ(1, db.get_last_dml_changes());
    EXPECT_EQ(2, db.get_last_insert_rowid());
    EXPECT_EQ(2, db.get_total_dml_changes());

    EXPECT_EQ(1, db.execute(InsertDefaultTableWithValue("third")));
    EXPECT_EQ(1, db.get_last_dml_changes());
    EXPECT_EQ(3, db.get_last_insert_rowid());
    EXPECT_EQ(3, db.get_total_dml_changes());

    EXPECT_EQ(1,
              db.execute("UPDATE " TableName
                         " SET value='second-updated' WHERE id='2'"));
    EXPECT_EQ(1, db.get_last_dml_changes());
    EXPECT_EQ(3, db.get_last_insert_rowid());
    EXPECT_EQ(4, db.get_total_dml_changes());

    EXPECT_EQ(1, db.execute("DELETE from " TableName " WHERE id='3'"));
    EXPECT_EQ(1, db.get_last_dml_changes());
    EXPECT_EQ(3, db.get_last_insert_rowid());
    EXPECT_EQ(5, db.get_total_dml_changes());

    (void)db.execute(DropTableIfExistSql);
    EXPECT_FALSE(db.table_exists(TableName));
    EXPECT_EQ(5, db.get_total_dml_changes());

    (void)db.execute(CreateTableDefaultSql);
    EXPECT_EQ(5, db.get_total_dml_changes());

    // only for second one returns 1
    EXPECT_EQ(1,
              db.execute(InsertDefaultTableWithValue(
                  "first") ";" InsertDefaultTableWithValue("second")));
    EXPECT_EQ(1, db.get_last_dml_changes());
    EXPECT_EQ(2, db.get_last_insert_rowid());
    EXPECT_EQ(7, db.get_total_dml_changes());

    EXPECT_EQ(2,
              db.execute("INSERT INTO " TableName
                         " VALUES (NULL, 'third'), (NULL, "
                         "'fourth');"));
    EXPECT_EQ(2, db.get_last_dml_changes());
    EXPECT_EQ(4, db.get_last_insert_rowid());
    EXPECT_EQ(9, db.get_total_dml_changes());
}

TEST(Database, DBTryExecute) {
    CreateMemoryDB;

    EXPECT_EQ(SqliteOK, db.try_execute(CreateTableDefaultSql));
    EXPECT_EQ(0, db.get_last_dml_changes());
    EXPECT_EQ(0, db.get_last_insert_rowid());
    EXPECT_EQ(0, db.get_total_dml_changes());

    EXPECT_EQ(SqliteOK, db.try_execute(InsertDefaultTableWithValue("first")));
    EXPECT_EQ(1, db.get_last_dml_changes());
    EXPECT_EQ(1, db.get_last_insert_rowid());
    EXPECT_EQ(1, db.get_total_dml_changes());

    EXPECT_EQ(SqliteOK, db.try_execute(InsertDefaultTableWithValue("second")));
    EXPECT_EQ(1, db.get_last_dml_changes());
    EXPECT_EQ(2, db.get_last_insert_rowid());
    EXPECT_EQ(2, db.get_total_dml_changes());

    EXPECT_EQ(SqliteOK, db.try_execute(InsertDefaultTableWithValue("third")));
    EXPECT_EQ(1, db.get_last_dml_changes());
    EXPECT_EQ(3, db.get_last_insert_rowid());
    EXPECT_EQ(3, db.get_total_dml_changes());

    EXPECT_EQ(SqliteOK,
              db.try_execute("UPDATE " TableName
                             " SET value='second-updated' WHERE id='2'"));
    EXPECT_EQ(1, db.get_last_dml_changes());
    EXPECT_EQ(3, db.get_last_insert_rowid());
    EXPECT_EQ(4, db.get_total_dml_changes());

    EXPECT_EQ(SqliteOK,
              db.try_execute("DELETE FROM " TableName " WHERE id='3'"));
    EXPECT_EQ(1, db.get_last_dml_changes());
    EXPECT_EQ(3, db.get_last_insert_rowid());
    EXPECT_EQ(5, db.get_total_dml_changes());

    EXPECT_EQ(SqliteOK, db.try_execute(DropTableIfExistSql));
    EXPECT_FALSE(db.table_exists(TableName));
    EXPECT_EQ(5, db.get_total_dml_changes());

    EXPECT_EQ(SqliteOK, db.try_execute(CreateTableDefaultSql));
    EXPECT_EQ(5, db.get_total_dml_changes());

    EXPECT_EQ(SqliteOK,
              db.try_execute(InsertDefaultTableWithValue(
                  "first") ";" InsertDefaultTableWithValue("second")));
    EXPECT_EQ(1, db.get_last_dml_changes());
    EXPECT_EQ(2, db.get_last_insert_rowid());
    EXPECT_EQ(7, db.get_total_dml_changes());

    EXPECT_EQ(SqliteOK,
              db.try_execute("INSERT INTO " TableName
                             " VALUES (NULL, 'third'), (NULL, "
                             "'fourth');"));
    EXPECT_EQ(2, db.get_last_dml_changes());
    EXPECT_EQ(4, db.get_last_insert_rowid());
    EXPECT_EQ(9, db.get_total_dml_changes());
}

TEST(Database, QueryAndGetFirst) {
    CreateMemoryDB;

    (void)db.execute("CREATE TABLE " TableName
                     " (id INTEGER PRIMARY KEY, value TEXT, weight INTEGER)");

    EXPECT_EQ(1,
              db.execute("INSERT INTO " TableName
                         " VALUES (NULL, 'first',  3)"));
    EXPECT_EQ(1,
              db.execute("INSERT INTO " TableName
                         " VALUES (NULL, 'second', 5)"));
    EXPECT_EQ(1,
              db.execute("INSERT INTO " TableName
                         " VALUES (NULL, 'third',  7)"));

    EXPECT_STREQ("second",
                 db.exec_and_get_first("SELECT value FROM " TableName
                                       " WHERE id=2")
                     .get_as_text());
    EXPECT_STREQ("third",
                 db.exec_and_get_first("SELECT value FROM " TableName
                                       " WHERE weight=7")
                     .get_as_text());
    const std::string query("SELECT weight FROM " TableName
                            " WHERE value='first'");
    EXPECT_EQ(3, db.exec_and_get_first(query).get_as_int());
}

TEST(Database, ExecuteThrow) {
    CreateMemoryDB;
    EXPECT_EQ(SqliteOK, db.get_error_code());
    EXPECT_EQ(SqliteOK, db.get_extended_error_code());

    EXPECT_THROW((void)db.execute(InsertDefaultTableWithValue("second")),
                 std::runtime_error);
    EXPECT_EQ(SQLITE_ERROR, db.get_error_code());
    EXPECT_EQ(SQLITE_ERROR, db.get_extended_error_code());
    EXPECT_STREQ("no such table: " TableName, db.get_error_msg().c_str());

    (void)db.execute("CREATE TABLE " TableName
                     " (id INTEGER PRIMARY KEY, value TEXT, weight "
                     "INTEGER)");
    EXPECT_EQ(SqliteOK, db.get_error_code());
    EXPECT_EQ(SqliteOK, db.get_extended_error_code());
    EXPECT_STREQ("not an error", db.get_error_msg().c_str());

    EXPECT_THROW((void)db.execute("INSERT INTO " TableName
                                  " VALUES (NULL,  1)"),
                 std::runtime_error);
    EXPECT_EQ(SQLITE_ERROR, db.get_error_code());
    EXPECT_EQ(SQLITE_ERROR, db.get_extended_error_code());
    EXPECT_STREQ("table " TableName " has 3 columns but 2 values were supplied",
                 db.get_error_msg().c_str());

    // exception with SQL error: "No row to get a column from"
    EXPECT_THROW((void)db.exec_and_get_first("SELECT weight FROM " TableName
                                             " WHERE value='first'"),
                 std::runtime_error);

    EXPECT_EQ(1,
              db.execute("INSERT INTO " TableName
                         " VALUES (NULL, 'first',  3)"));
    EXPECT_THROW((void)db.exec_and_get_first("SELECT weght FROM " TableName
                                             " WHERE value='second'"),
                 std::runtime_error);
    EXPECT_EQ(SQLITE_ERROR, db.get_error_code());
    EXPECT_EQ(SQLITE_ERROR, db.get_extended_error_code());
    EXPECT_STREQ("no such column: weght", db.get_error_msg().c_str());

    EXPECT_THROW((void)db.execute("INSERT INTO " TableName
                                  " VALUES (NULL, 'first', 123, 0.123)"),
                 std::runtime_error);
    EXPECT_EQ(SQLITE_ERROR, db.get_error_code());
    EXPECT_EQ(SQLITE_ERROR, db.get_extended_error_code());
    EXPECT_STREQ("table " TableName " has 3 columns but 4 values were supplied",
                 db.get_error_msg().c_str());
}

TEST(Database, TryExecuteError) {
    CreateMemoryDB;
    EXPECT_EQ(SqliteOK, db.get_error_code());
    EXPECT_EQ(SqliteOK, db.get_extended_error_code());

    EXPECT_EQ(SQLITE_ERROR,
              db.try_execute("INSERT INTO " TableName
                             " VALUES (NULL, 'first',  3)"));
    EXPECT_EQ(SQLITE_ERROR, db.get_error_code());
    EXPECT_EQ(SQLITE_ERROR, db.get_extended_error_code());
    EXPECT_STREQ("no such table: " TableName, db.get_error_msg().c_str());

    EXPECT_EQ(SqliteOK,
              db.try_execute("CREATE TABLE " TableName
                             " (id INTEGER PRIMARY KEY, value "
                             "TEXT, weight INTEGER)"));
    EXPECT_EQ(SqliteOK, db.get_error_code());
    EXPECT_EQ(SqliteOK, db.get_extended_error_code());
    EXPECT_STREQ("not an error", db.get_error_msg().c_str());

    EXPECT_EQ(SQLITE_ERROR,
              db.try_execute("INSERT INTO " TableName " VALUES (NULL,  3)"));
    EXPECT_EQ(SQLITE_ERROR, db.get_error_code());
    EXPECT_EQ(SQLITE_ERROR, db.get_extended_error_code());
    EXPECT_STREQ("table " TableName " has 3 columns but 2 values were supplied",
                 db.get_error_msg().c_str());

    EXPECT_EQ(SQLITE_ERROR,
              db.try_execute("INSERT INTO " TableName
                             " VALUES (NULL, 'first', 123, 0.123)"));
    EXPECT_EQ(SQLITE_ERROR, db.get_error_code());
    EXPECT_EQ(SQLITE_ERROR, db.get_extended_error_code());
    EXPECT_STREQ("table " TableName " has 3 columns but 4 values were supplied",
                 db.get_error_msg().c_str());

    EXPECT_EQ(SqliteOK,
              db.try_execute("INSERT INTO " TableName
                             " VALUES (NULL, 'first',  3)"));
    EXPECT_EQ(1, db.get_last_insert_rowid());

    EXPECT_EQ(SQLITE_CONSTRAINT,
              db.try_execute("INSERT INTO " TableName
                             " VALUES (1, 'impossible', 456)"));
    EXPECT_EQ(SQLITE_CONSTRAINT, db.get_error_code());
    EXPECT_EQ(SQLITE_CONSTRAINT_PRIMARYKEY, db.get_extended_error_code());
    EXPECT_STREQ("UNIQUE constraint failed: " TableName ".id",
                 db.get_error_msg().c_str());
}

TEST(Database, ExecuteMutiple) {
    CreateMemoryDB;

    EXPECT_EQ(0, db.execute(DropTableIfExistSql));
    EXPECT_EQ(0,
              db.execute("CREATE TABLE " TableName
                         " (id INTEGER PRIMARY KEY, value TEXT "
                         "DEFAULT 'default')"));
    EXPECT_TRUE(db.table_exists(TableName));
    {
        db.execute_multiple("INSERT INTO " TableName " VALUES (?, ?)",
                            1,
                            std::make_tuple(2),
                            std::make_tuple(3, "three"));
    }
    {
        PreparedStmt query(db,
                           std::string{"SELECT id, value FROM " TableName
                                       " ORDER BY id"});
        std::vector< std::pair< int, std::string > > results;
        while (query.execute_step()) {
            const int id = query.get_column(0).get_as_int();
            std::string value = query.get_column(1).get_as_string();
            results.emplace_back(id, std::move(value));
        }
        EXPECT_EQ(std::size_t(3), results.size());

        EXPECT_EQ(std::make_pair(1, std::string{""}), results.at(0));
        EXPECT_EQ(std::make_pair(2, std::string{""}), results.at(1));
        EXPECT_EQ(std::make_pair(3, std::string{"three"}), results.at(2));
    }
}

TEST(Database, PreparedStatementInvalid) {
    CreateMemoryDB;
    EXPECT_EQ(SqliteOK, db.get_error_code());
    EXPECT_EQ(SqliteOK, db.get_extended_error_code());

    EXPECT_THROW(PreparedStmt query(db, SelectAllSqlFromDefaultTable),
                 std::runtime_error);
    EXPECT_EQ(SQLITE_ERROR, db.get_error_code());
    EXPECT_EQ(SQLITE_ERROR, db.get_extended_error_code());
    EXPECT_STREQ("no such table: " TableName, db.get_error_msg().c_str());

    EXPECT_EQ(0, db.execute(CreateTableDefaultSql));
    EXPECT_EQ(SqliteOK, db.get_error_code());
    EXPECT_EQ(SqliteOK, db.get_extended_error_code());

    PreparedStmt query(db, SelectAllSqlFromDefaultTable);
    EXPECT_STREQ(SelectAllSqlFromDefaultTable, query.get_sql().c_str());
    EXPECT_EQ(2, query.get_result_column_count());
    EXPECT_FALSE(query.has_row_to_fetched());
    EXPECT_FALSE(query.does_rows_fetched_done());
    EXPECT_EQ(SqliteOK, query.get_error_code());
    EXPECT_EQ(SqliteOK, query.get_extended_error_code());
    EXPECT_THROW((void)query.is_column_null(-1), std::runtime_error);
    EXPECT_THROW((void)query.is_column_null(0), std::runtime_error);
    EXPECT_THROW((void)query.is_column_null(1), std::runtime_error);
    EXPECT_THROW((void)query.is_column_null(2), std::runtime_error);
    EXPECT_THROW((void)query.get_column(-1), std::runtime_error);
    EXPECT_THROW((void)query.get_column(0), std::runtime_error);
    EXPECT_THROW((void)query.get_column(1), std::runtime_error);
    EXPECT_THROW((void)query.get_column(2), std::runtime_error);

    query.reset();
    EXPECT_FALSE(query.has_row_to_fetched());
    EXPECT_FALSE(query.does_rows_fetched_done());

    (void)query.execute_step();
    EXPECT_FALSE(query.has_row_to_fetched());
    EXPECT_TRUE(query.does_rows_fetched_done());
    query.reset();
    EXPECT_FALSE(query.has_row_to_fetched());
    EXPECT_FALSE(query.does_rows_fetched_done());

    query.reset();
    EXPECT_THROW(query.bind(-1, 123), std::runtime_error);
    EXPECT_STREQ("column index out of range", db.get_error_msg().c_str());
    EXPECT_THROW(query.bind(0, 123), std::runtime_error);
    EXPECT_STREQ("column index out of range", db.get_error_msg().c_str());
    EXPECT_THROW(query.bind(1, 123), std::runtime_error);
    EXPECT_STREQ("column index out of range", db.get_error_msg().c_str());
    EXPECT_THROW(query.bind(2, 123), std::runtime_error);
    EXPECT_STREQ("column index out of range", db.get_error_msg().c_str());
    EXPECT_THROW(query.bind(0, "abc"), std::runtime_error);
    EXPECT_STREQ("column index out of range", db.get_error_msg().c_str());
    EXPECT_THROW(query.bind_null(0), std::runtime_error);
    EXPECT_STREQ("column index out of range", db.get_error_msg().c_str());
    EXPECT_EQ(SQLITE_RANGE, db.get_error_code());
    EXPECT_EQ(SQLITE_RANGE, db.get_extended_error_code());
    EXPECT_STREQ("column index out of range", db.get_error_msg().c_str());
    EXPECT_EQ(SQLITE_RANGE, query.get_error_code());
    EXPECT_EQ(SQLITE_RANGE, query.get_extended_error_code());
    EXPECT_STREQ("column index out of range", query.get_error_msg().c_str());

    (void)query.execute();
    EXPECT_THROW((void)query.is_column_null(0), std::runtime_error);
    EXPECT_STREQ("no more rows available", query.get_error_msg().c_str());

    EXPECT_THROW((void)query.get_column(0), std::runtime_error);
    EXPECT_STREQ("no more rows available", query.get_error_msg().c_str());

    // need to be reseted before execute
    EXPECT_THROW((void)query.execute(), std::runtime_error);

    EXPECT_EQ(1, db.execute(InsertDefaultTableWithValue("first")));
    EXPECT_EQ(1, db.get_last_insert_rowid());
    EXPECT_EQ(1, db.get_total_dml_changes());

    query.reset();
    EXPECT_FALSE(query.has_row_to_fetched());
    EXPECT_FALSE(query.does_rows_fetched_done());

    EXPECT_THROW((void)query.execute(), std::runtime_error);
}

TEST(Database, PreparedStmtExecuteStep) {
    CreateMemoryDB;
    EXPECT_EQ(SqliteOK, db.get_error_code());

    EXPECT_EQ(0,
              db.execute("CREATE TABLE " TableName
                         " (id INTEGER PRIMARY KEY, msg TEXT, "
                         "int INTEGER, double REAL)"));
    EXPECT_EQ(SqliteOK, db.get_error_code());

    EXPECT_EQ(1,
              db.execute("INSERT INTO " TableName
                         " VALUES (NULL, 'first', 123, 0.123)"));
    EXPECT_EQ(1, db.get_last_insert_rowid());

    PreparedStmt query(db, SelectAllSqlFromDefaultTable);
    EXPECT_STREQ(SelectAllSqlFromDefaultTable, query.get_sql().c_str());
    EXPECT_EQ(4, query.get_result_column_count());

    EXPECT_TRUE(query.execute_step());
    EXPECT_TRUE(query.has_row_to_fetched());
    EXPECT_FALSE(query.does_rows_fetched_done());

    EXPECT_EQ(1, query.get_column(0).get_as_int64());
    EXPECT_EQ("first", query.get_column(1).get_as_string());
    EXPECT_EQ(123, query.get_column(2).get_as_int());
    EXPECT_EQ(123, query.get_column(2).get_as_int64());
    EXPECT_DOUBLE_EQ(0.123, query.get_column(3).get_as_double());

    // fetch done
    EXPECT_FALSE(query.execute_step());
    EXPECT_FALSE(query.has_row_to_fetched());
    EXPECT_TRUE(query.does_rows_fetched_done());

    EXPECT_THROW((void)query.execute_step(), std::runtime_error);

    PreparedStmt insert(db,
                        "INSERT INTO " TableName
                        " VALUES (1, 'impossible', 456, 0.456)");
    EXPECT_THROW((void)insert.execute_step(), std::runtime_error);
    EXPECT_STREQ("UNIQUE constraint failed: " TableName ".id",
                 insert.get_error_msg().c_str());
    EXPECT_THROW(insert.reset(), std::runtime_error);

    PreparedStmt insert2(db,
                         "INSERT INTO " TableName
                         " VALUES (1, 'impossible', 456, 0.456)");
    EXPECT_THROW((void)insert2.execute(), std::runtime_error);
    EXPECT_STREQ("UNIQUE constraint failed: " TableName ".id",
                 insert.get_error_msg().c_str());
}

TEST(Database, PreparedStmtTryExecuteStep) {
    CreateMemoryDB;
    EXPECT_EQ(SqliteOK, db.get_error_code());

    EXPECT_EQ(0,
              db.execute("CREATE TABLE " TableName
                         " (id INTEGER PRIMARY KEY, msg TEXT, "
                         "int INTEGER, double REAL)"));
    EXPECT_EQ(SqliteOK, db.get_error_code());

    EXPECT_EQ(1,
              db.execute("INSERT INTO " TableName
                         " VALUES (NULL, 'first', 123, 0.123)"));
    EXPECT_EQ(1, db.get_last_insert_rowid());

    PreparedStmt query(db, SelectAllSqlFromDefaultTable);
    EXPECT_STREQ(SelectAllSqlFromDefaultTable, query.get_sql().c_str());
    EXPECT_EQ(4, query.get_result_column_count());

    EXPECT_EQ(query.try_execute_step(), SQLITE_ROW);
    EXPECT_TRUE(query.has_row_to_fetched());
    EXPECT_FALSE(query.does_rows_fetched_done());

    EXPECT_EQ(1, query.get_column(0).get_as_int64());
    EXPECT_EQ("first", query.get_column(1).get_as_string());
    EXPECT_EQ(123, query.get_column(2).get_as_int());
    EXPECT_EQ(123, query.get_column(2).get_as_int64());
    EXPECT_DOUBLE_EQ(0.123, query.get_column(3).get_as_double());

    // fetch done
    EXPECT_EQ(query.try_execute_step(), SQLITE_DONE);
    EXPECT_FALSE(query.has_row_to_fetched());
    EXPECT_TRUE(query.does_rows_fetched_done());

    PreparedStmt insert(db,
                        "INSERT INTO " TableName
                        " VALUES (1, 'impossible', 456, 0.456)");
    EXPECT_EQ(insert.try_execute_step(), SQLITE_CONSTRAINT);
    EXPECT_STREQ("UNIQUE constraint failed: " TableName ".id",
                 insert.get_error_msg().c_str());
    EXPECT_EQ(insert.try_reset(), SQLITE_CONSTRAINT);
}

TEST(Database, PreparedStmtBindings) {
    CreateMemoryDB;
    EXPECT_EQ(SqliteOK, db.get_error_code());

    EXPECT_EQ(0,
              db.execute("CREATE TABLE " TableName
                         " (id INTEGER PRIMARY KEY, msg TEXT, "
                         "int INTEGER, double REAL)"));
    EXPECT_EQ(SqliteOK, db.get_error_code());

    PreparedStmt insert(db, "INSERT INTO " TableName " VALUES (NULL, ?, ?, ?)");

    PreparedStmt query(db, SelectAllSqlFromDefaultTable);
    EXPECT_STREQ(SelectAllSqlFromDefaultTable, query.get_sql().c_str());
    EXPECT_EQ(4, query.get_result_column_count());

    {
        insert.bind(1, "first");
        insert.bind(2, -123);
        insert.bind(3, 0.123);
        EXPECT_EQ(insert.get_expanded_sql(),
                  "INSERT INTO " TableName
                  " VALUES (NULL, 'first', -123, 0.123)");
        EXPECT_EQ(1, insert.execute());
        EXPECT_EQ(SQLITE_DONE, db.get_error_code());

        EXPECT_TRUE(query.execute_step());
        EXPECT_TRUE(query.has_row_to_fetched());
        EXPECT_FALSE(query.does_rows_fetched_done());
        EXPECT_EQ(1, query.get_column(0).get_as_int64());
        EXPECT_STREQ("first", query.get_column(1).get_as_text());
        EXPECT_EQ(-123, query.get_column(2).get_as_int());
        EXPECT_EQ(0.123, query.get_column(3).get_as_double());
    }

    // no clear_bindings()
    insert.reset();

    // same value as first without clear_bindings()
    {
        EXPECT_EQ(1, insert.execute());
        EXPECT_EQ(SQLITE_DONE, db.get_error_code());

        EXPECT_TRUE(query.execute_step());
        EXPECT_TRUE(query.has_row_to_fetched());
        EXPECT_FALSE(query.does_rows_fetched_done());
        EXPECT_EQ(2, query.get_column(0).get_as_int64());
        EXPECT_STREQ("first", query.get_column(1).get_as_text());
        EXPECT_EQ(-123, query.get_column(2).get_as_int());
        EXPECT_EQ(0.123, query.get_column(3).get_as_double());
    }

    insert.reset();
    insert.clear_bindings();

    // no binding will bind to null
    {
        EXPECT_EQ(1, insert.execute());
        EXPECT_EQ(SQLITE_DONE, db.get_error_code());

        EXPECT_TRUE(query.execute_step());
        EXPECT_TRUE(query.has_row_to_fetched());
        EXPECT_FALSE(query.does_rows_fetched_done());
        EXPECT_EQ(3, query.get_column(0).get_as_int64());
        EXPECT_TRUE(query.is_column_null(1));
        EXPECT_STREQ("", query.get_column(1).get_as_text());
        EXPECT_TRUE(query.is_column_null(2));
        EXPECT_EQ(0, query.get_column(2).get_as_int());
        EXPECT_TRUE(query.is_column_null(3));
        EXPECT_EQ(0.0, query.get_column(3).get_as_double());
    }

    insert.reset();
    insert.clear_bindings();

    {
        insert.bind(1, "fourth");
        insert.bind(2, (int64_t)12345678900000LL);
        insert.bind(3, 0.234f);
        EXPECT_EQ(1, insert.execute());
        EXPECT_EQ(SQLITE_DONE, db.get_error_code());

        EXPECT_TRUE(query.execute_step());
        EXPECT_TRUE(query.has_row_to_fetched());
        EXPECT_FALSE(query.does_rows_fetched_done());
        EXPECT_EQ(4, query.get_column(0).get_as_int64());
        EXPECT_STREQ("fourth", query.get_column(1).get_as_text());
        EXPECT_EQ(12345678900000LL, query.get_column(2).get_as_int64());
        EXPECT_FLOAT_EQ(0.234f, (float)query.get_column(3).get_as_double());
    }

    // no clear_bindings()
    insert.reset();

    {
        const char buffer[] = "binary";
        insert.bind(1, buffer, sizeof(buffer));
        insert.bind_null(2); // bind a NULL value
        EXPECT_EQ(1, insert.execute());

        // Check the result
        EXPECT_TRUE(query.execute_step());
        EXPECT_TRUE(query.has_row_to_fetched());
        EXPECT_FALSE(query.does_rows_fetched_done());
        EXPECT_EQ(5, query.get_column(0).get_as_int64());
        EXPECT_STREQ(buffer, query.get_column(1).get_as_text());
        EXPECT_TRUE(query.is_column_null(2));
        EXPECT_EQ(0, query.get_column(2).get_as_int());
        EXPECT_FLOAT_EQ(0.234f, (float)query.get_column(3).get_as_double());
    }

    // no clear_bindings()
    insert.reset();

    {
        insert.bind(2, 4294967295U);
        insert.bind(3, -123);
        EXPECT_EQ(1, insert.execute());
        EXPECT_EQ(SQLITE_DONE, db.get_error_code());

        EXPECT_TRUE(query.execute_step());
        EXPECT_TRUE(query.has_row_to_fetched());
        EXPECT_FALSE(query.does_rows_fetched_done());
        EXPECT_EQ(6, query.get_column(0).get_as_int64());
        EXPECT_EQ(4294967295U, query.get_column(2).get_as_int64());
        EXPECT_EQ(-123, query.get_column(3).get_as_int());
    }

    // no clear_bindings()
    insert.reset();

    {
        insert.bind(2, (int64_t)12345678900000LL);
        EXPECT_EQ(1, insert.execute());
        EXPECT_EQ(SQLITE_DONE, db.get_error_code());

        // Check the result
        EXPECT_TRUE(query.execute_step());
        EXPECT_TRUE(query.has_row_to_fetched());
        EXPECT_FALSE(query.does_rows_fetched_done());
        EXPECT_EQ(7, query.get_column(0).get_as_int());
        EXPECT_EQ(12345678900000LL, query.get_column(2).get_as_int64());
    }
}

TEST(Database, PreparedStmtBindByName) {
    CreateMemoryDB;
    EXPECT_EQ(SqliteOK, db.get_error_code());

    EXPECT_EQ(0,
              db.execute("CREATE TABLE " TableName
                         " (id INTEGER PRIMARY KEY, msg TEXT, "
                         "int INTEGER, double REAL, long INTEGER)"));
    EXPECT_EQ(SqliteOK, db.get_error_code());

    PreparedStmt insert(db,
                        "INSERT INTO " TableName
                        " VALUES (NULL, @msg, @int, @double, @long)");

    insert.bind("@msg", "first");
    insert.bind("@int", 123);
    insert.bind("@long", -123);
    insert.bind("@double", 0.123);

    EXPECT_EQ(1, insert.execute());
    EXPECT_EQ(SQLITE_DONE, db.get_error_code());

    PreparedStmt query(db, SelectAllSqlFromDefaultTable);
    EXPECT_STREQ(SelectAllSqlFromDefaultTable, query.get_sql().c_str());
    EXPECT_EQ(5, query.get_result_column_count());

    EXPECT_TRUE(query.execute_step());
    EXPECT_TRUE(query.has_row_to_fetched());
    EXPECT_FALSE(query.does_rows_fetched_done());
    EXPECT_EQ(1, query.get_column(0).get_as_int64());
    EXPECT_STREQ("first", query.get_column(1).get_as_text());
    EXPECT_EQ(123, query.get_column(2).get_as_int());
    EXPECT_DOUBLE_EQ(0.123, query.get_column(3).get_as_double());
    EXPECT_EQ(-123, query.get_column(4).get_as_int());

    insert.reset();
    insert.clear_bindings();

    {
        insert.bind("@msg", "second");
        insert.bind("@int", (int64_t)12345678900000LL);
        insert.bind("@double", 0.234f);
        insert.bind("@long", -123);
        EXPECT_EQ(1, insert.execute());
        EXPECT_EQ(SQLITE_DONE, db.get_error_code());

        // Check the result
        EXPECT_TRUE(query.execute_step());
        EXPECT_TRUE(query.has_row_to_fetched());
        EXPECT_FALSE(query.does_rows_fetched_done());
        EXPECT_EQ(2, query.get_column(0).get_as_int());
        EXPECT_STREQ("second", query.get_column(1).get_as_text());
        EXPECT_EQ(12345678900000LL, query.get_column(2).get_as_int64());
        EXPECT_FLOAT_EQ(0.234f, (float)query.get_column(3).get_as_double());
        EXPECT_EQ(-123, query.get_column(4).get_as_int());
    }

    insert.reset();

    {
        const char buffer[] = "binary";
        insert.bind("@msg", buffer, sizeof(buffer));
        insert.bind_null("@int"); // bind a NULL value
        EXPECT_EQ(1, insert.execute());

        // Check the result
        EXPECT_TRUE(query.execute_step());
        EXPECT_TRUE(query.has_row_to_fetched());
        EXPECT_FALSE(query.does_rows_fetched_done());
        EXPECT_EQ(3, query.get_column(0).get_as_int64());
        EXPECT_STREQ(buffer, query.get_column(1).get_as_text());
        EXPECT_TRUE(query.is_column_null(2));
        EXPECT_EQ(0, query.get_column(2).get_as_int());
        EXPECT_FLOAT_EQ(0.234f, (float)query.get_column(3).get_as_double());
    }

    insert.reset();

    {
        insert.bind("@int", 4294967295U);
        insert.bind("@long", (int64_t)12345678900000LL);
        EXPECT_EQ(1, insert.execute());
        EXPECT_EQ(SQLITE_DONE, db.get_error_code());

        EXPECT_TRUE(query.execute_step());
        EXPECT_TRUE(query.has_row_to_fetched());
        EXPECT_FALSE(query.does_rows_fetched_done());
        EXPECT_EQ(4, query.get_column(0).get_as_int64());
        EXPECT_EQ(4294967295U, query.get_column(2).get_as_int64());
        EXPECT_EQ(12345678900000LL, query.get_column(4).get_as_int64());
    }
}

TEST(Database, PreparedStmtGetColumnByName) {
    CreateMemoryDB;
    EXPECT_EQ(SqliteOK, db.get_error_code());
    EXPECT_EQ(SqliteOK, db.get_extended_error_code());

    EXPECT_EQ(0,
              db.execute("CREATE TABLE " TableName
                         " (id INTEGER PRIMARY KEY, msg TEXT, int INTEGER, "
                         "double REAL)"));
    EXPECT_EQ(SqliteOK, db.get_error_code());
    EXPECT_EQ(SqliteOK, db.get_extended_error_code());

    EXPECT_EQ(1,
              db.execute("INSERT INTO " TableName
                         " VALUES (NULL, 'first', 123, 0.123)"));
    EXPECT_EQ(1, db.get_last_insert_rowid());
    EXPECT_EQ(1, db.get_total_dml_changes());

    // Compile a SQL query
    PreparedStmt query(db, SelectAllSqlFromDefaultTable);
    EXPECT_STREQ(SelectAllSqlFromDefaultTable, query.get_sql().c_str());
    EXPECT_EQ(4, query.get_result_column_count());
    EXPECT_TRUE(query.execute_step());
    EXPECT_TRUE(query.has_row_to_fetched());
    EXPECT_FALSE(query.does_rows_fetched_done());

    EXPECT_THROW((void)query.get_column("unknown"), std::runtime_error);
    EXPECT_THROW((void)query.get_column(""), std::runtime_error);

    EXPECT_EQ("first", query.get_column("msg").get_as_string());
    EXPECT_EQ(123, query.get_column("int").get_as_int());
    EXPECT_DOUBLE_EQ(0.123, query.get_column("double").get_as_double());
    EXPECT_EQ(0, query.get_bind_parameter_count());
}

TEST(Database, PreparedStmtTransaction) {
    CreateMemoryDB;
    EXPECT_EQ(SqliteOK, db.get_error_code());

    {
        Transaction transaction(db);

        EXPECT_EQ(0, db.execute(CreateTableDefaultSql));
        EXPECT_EQ(SqliteOK, db.get_error_code());

        EXPECT_EQ(1, db.execute(InsertDefaultTableWithValue("first")));
        EXPECT_EQ(1, db.get_last_insert_rowid());

        transaction.commit();

        EXPECT_THROW(transaction.commit(), std::runtime_error);
    }

    {
        for (auto mode : {TransactionMode::DEFERRED,
                          TransactionMode::IMMEDIATE,
                          TransactionMode::EXCLUSIVE}) {
            Transaction transaction(db, mode);
            transaction.commit();
        }

        EXPECT_THROW(Transaction(db, static_cast< TransactionMode >(-1)),
                     std::runtime_error);
    }

    // Auto rollback
    {
        Transaction transaction(db);
        EXPECT_EQ(1, db.execute(InsertDefaultTableWithValue("third")));
        EXPECT_EQ(2, db.get_last_insert_rowid());
    }

    // Auto rollback
    try {
        Transaction transaction(db);

        EXPECT_EQ(1, db.execute("INSERT INTO test VALUES (NULL, 'second')"));
        EXPECT_EQ(2, db.get_last_insert_rowid());

        // Exception would rollback the transaction
        (void)db.execute(
            "DesiredSyntaxError to raise an exception to rollback the "
            "transaction");

        knight_unreachable("unreachable");
        transaction.commit();
    } catch (std::runtime_error& e) {
        std::cout << "exception: " << e.what() << std::endl;
    }

    {
        Transaction transaction(db);

        EXPECT_EQ(1, db.execute(InsertDefaultTableWithValue("third")));
        EXPECT_EQ(2, db.get_last_insert_rowid());

        transaction.rollback();
    }

    PreparedStmt query(db, SelectAllSqlFromDefaultTable);
    int n = 0;
    while (query.execute_step()) {
        n++;
        EXPECT_EQ(1, query.get_column(0).get_as_int());
        EXPECT_STREQ("first", query.get_column(1).get_as_text());
    }
    EXPECT_EQ(1, n);
}
