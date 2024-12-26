//===- db.cpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the core cg database driver.
//
//===------------------------------------------------------------------===//

#include "cg/db/db.hpp"
#include "cg/core/cg.hpp"
#include "llvm/ADT/STLExtras.h"

namespace knight::cg {

void Database::create_table_if_not_exist() const noexcept {
    if (!m_db.table_exists("cg_node")) {
        auto ret = m_db.try_execute(
            "CREATE TABLE cg_node (id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "line INTEGER, col INTEGER, name TEXT, mangled_name TEXT, file "
            "TEXT)");
        knight_assert_msg(ret == sqlite::SqliteOK,
                          "Failed to create "
                          "table 'cg_node'");
    }
    if (!m_db.table_exists("callsite")) {
        auto ret = m_db.try_execute(
            "CREATE TABLE callsite (id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "line INTEGER, col INTEGER, caller TEXT, callee TEXT)");
        knight_assert_msg(ret == sqlite::SqliteOK,
                          "Failed to create "
                          "table 'callsite'");
    }
}

Database::Database(const std::string& knight_dir,
                   const int busy_time_mills,
                   const int writer_elem_size) noexcept(false)
    : m_db(sqlite::Database(knight_dir + "/cg.db",
                            sqlite::OpenMode::READWRITE |
                                sqlite::OpenMode::CREATE,
                            busy_time_mills)),
      m_writer_elem_size(writer_elem_size) {
    create_table_if_not_exist();
    m_cg_nodes.reserve(m_writer_elem_size);
    m_callsites.reserve(m_writer_elem_size);
}

Database::~Database() {
    flush_cg_nodes();
    flush_callsites();
}

void Database::insert_callsite(const CallSite& callsite) noexcept(false) {
    m_callsites.emplace_back(callsite);
    if (m_callsites.size() >= m_writer_elem_size) {
        flush_callsites();
    }
}

void Database::insert_cg_node(const CallGraphNode& cg_node) noexcept(false) {
    m_cg_nodes.emplace_back(cg_node);
    if (m_cg_nodes.size() >= m_writer_elem_size) {
        flush_cg_nodes();
    }
}

std::vector< CallGraphNode > Database::get_all_cg_nodes() const noexcept {
    std::vector< CallGraphNode > result;
    sqlite::PreparedStmt stmt(m_db, "SELECT * FROM cg_node");
    while (stmt.execute_step()) {
        const auto id = stmt.get_column(0).get_as_int();
        const auto line = stmt.get_column(1).get_as_int();
        const auto col = stmt.get_column(2).get_as_int();
        const auto* name = stmt.get_column(3).get_as_text();
        const auto* mangled_name = stmt.get_column(4).get_as_text();
        const auto* file = stmt.get_column(5).get_as_text();
        result.emplace_back(line, col, name, mangled_name, file);
    }
    return result;
}

std::vector< CallSite > Database::get_all_callsites() const noexcept {
    std::vector< CallSite > result;
    sqlite::PreparedStmt stmt(m_db, "SELECT * FROM callsite");
    while (stmt.execute_step()) {
        const auto id = stmt.get_column(0).get_as_int();
        const auto line = stmt.get_column(1).get_as_int();
        const auto col = stmt.get_column(2).get_as_int();
        const auto* caller = stmt.get_column(3).get_as_text();
        const auto* callee = stmt.get_column(4).get_as_text();
        result.emplace_back(line, col, caller, callee);
    }
    return result;
}

void Database::flush_cg_nodes() {
    if (m_cg_nodes.empty()) {
        return;
    }
    sqlite::Transaction transaction((m_db));
    sqlite::PreparedStmt stmt(m_db,
                              "INSERT INTO cg_node (line, col, name, "
                              "mangled_name, file) VALUES (?,?,?,?,?)");
    const auto insert = [&](const CallGraphNode& cg_node) {
        stmt.bind(1, cg_node.line);
        stmt.bind(2, cg_node.col);
        stmt.bind(3, cg_node.name);
        stmt.bind(4, cg_node.mangled_name);
        stmt.bind(5, cg_node.file);
        const auto ret = stmt.execute();
        knight_assert_msg(ret == 1, "Failed to insert cg_node");
        stmt.reset();
        stmt.clear_bindings();
    };
    llvm::for_each(m_cg_nodes, insert);
    transaction.commit();
    m_cg_nodes.clear();
}

void Database::flush_callsites() {
    if (m_callsites.empty()) {
        return;
    }
    sqlite::Transaction transaction((m_db));
    sqlite::PreparedStmt stmt(m_db,
                              "INSERT INTO callsite (line, col, caller, "
                              "callee) VALUES (?,?,?,?)");
    const auto insert = [&](const CallSite& callsite) {
        stmt.bind(1, callsite.line);
        stmt.bind(2, callsite.col);
        stmt.bind(3, callsite.caller);
        stmt.bind(4, callsite.callee);
        const auto ret = stmt.execute();
        knight_assert_msg(ret == 1, "Failed to insert callsite");
        stmt.reset();
        stmt.clear_bindings();
    };
    llvm::for_each(m_callsites, insert);
    transaction.commit();
    m_callsites.clear();
}

} // namespace knight::cg