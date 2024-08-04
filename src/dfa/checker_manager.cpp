//===- checker_manager.cpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the checker manager.
//
//===------------------------------------------------------------------===//

#include "dfa/checker_manager.hpp"
#include "dfa/analysis/analyses.hpp"
#include "dfa/analysis/analysis_base.hpp"
#include "dfa/checker/checker_base.hpp"
#include "dfa/checker/checkers.hpp"
#include "util/assert.hpp"

#include <memory>
namespace knight::dfa {

void CheckerManager::add_required_checker(CheckerID id) {
    m_required_checkers.emplace(id);
}

bool CheckerManager::is_checker_required(CheckerID id) const {
    return m_required_checkers.contains(id);
}

void CheckerManager::enable_checker(std::unique_ptr< CheckerBase > checker) {
    auto id = get_checker_id(checker->kind);
    m_enabled_checkers.emplace(id, std::move(checker));
}

std::optional< CheckerBase* > CheckerManager::get_checker(CheckerID id) {
    auto it = m_enabled_checkers.find(id);
    if (it == m_enabled_checkers.end()) {
        return std::nullopt;
    }
    return it->second.get();
}

void CheckerManager::register_for_stmt(internal::CheckStmtCallBack cb,
                                       internal::MatchStmtCallBack match_fn,
                                       internal::CheckStmtKind kind) {
    m_stmt_checks.emplace_back(cb, match_fn, kind);
}

void CheckerManager::register_for_begin_function(
    internal::CheckBeginFunctionCallBack cb) {
    m_begin_function_checks.emplace_back(cb);
}

void CheckerManager::register_for_end_function(
    internal::CheckEndFunctionCallBack cb) {
    m_end_function_checks.emplace_back(cb);
}

void CheckerManager::run_checkers_for_stmt(internal::StmtRef stmt,
                                           internal::CheckStmtKind check_kind) {
    CheckerIDSet tgt_ids;
    std::unordered_map< AnalysisID, internal::AnalyzeStmtCallBack* > callbacks;
    for (auto& info : m_stmt_checks) {
        if (info.kind != check_kind || !info.match_cb(stmt)) {
            continue;
        }
        auto& callback = info.anz_cb;
        auto id = callback.get_id();
        if (is_checker_required(id)) {
            callback(stmt, *m_checker_ctx);
        }
    }
}

void CheckerManager::run_checkers_for_pre_stmt(internal::StmtRef stmt) {
    run_checkers_for_stmt(stmt, internal::CheckStmtKind::Pre);
}

void CheckerManager::run_checkers_for_post_stmt(internal::StmtRef stmt) {
    run_checkers_for_stmt(stmt, internal::CheckStmtKind::Post);
}

void CheckerManager::run_checkers_for_begin_function() {
    for (auto& callback : m_begin_function_checks) {
        auto id = callback.get_id();
        if (is_checker_required(id)) {
            callback(*m_checker_ctx);
        }
    }
}

void CheckerManager::run_checkers_for_end_function(ProcCFG::NodeRef node) {
    for (auto& callback : m_end_function_checks) {
        auto id = callback.get_id();
        if (is_checker_required(id)) {
            callback(node, *m_checker_ctx);
        }
    }
}

} // namespace knight::dfa