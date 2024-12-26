//===- db.hpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the core cg database driver.
//
//===------------------------------------------------------------------===//

#pragma once

#include "cg/core/cg.hpp"
#include "common/util/sqlite3.hpp"

namespace knight::cg {

constexpr unsigned DefaultWriterElemSize = 512U;

class Database {
  public:
    explicit Database(
        const std::string& knight_dir,
        int busy_time_mills,
        int writer_elem_size = DefaultWriterElemSize) noexcept(false);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    Database(Database&& aDatabase) = default;
    Database& operator=(Database&& aDatabase) = default;

  public:
    void insert_callsite(const CallSite& callsite) noexcept(false);
    void insert_cg_node(const CallGraphNode& cg_node) noexcept(false);
    [[nodiscard]] std::vector< CallGraphNode > get_all_cg_nodes()
        const noexcept;
    [[nodiscard]] std::vector< CallSite > get_all_callsites() const noexcept;

  private:
    void create_table_if_not_exist() const noexcept;
    void flush_cg_nodes();
    void flush_callsites();

  private:
    sqlite::Database m_db;

    int m_writer_elem_size;

    std::vector< CallGraphNode > m_cg_nodes;
    std::vector< CallSite > m_callsites;

}; // class Database

} // namespace knight::cg