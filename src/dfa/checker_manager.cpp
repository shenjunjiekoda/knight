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
#include "dfa/analysis/analysis_base.hpp"
#include "dfa/checker/checker_base.hpp"
#include "util/assert.hpp"
#include <memory>

namespace knight::dfa {

void CheckerManager::register_checker(CheckerID id) {
    m_required_checkers.emplace(id);
}

bool CheckerManager::is_checker_required(CheckerID id) const {
    return m_required_checkers.count(id) > 0U;
}

void CheckerManager::enable_checker(
    // TODO: add enabling logic for checkers (check filter and required checkers)
    std::unique_ptr< CheckerBase > checker) {
    auto id = checker->get_id();
    m_required_checkers.emplace(id);
    m_enabled_checkers.emplace(id, std::move(checker));
}

std::optional< CheckerBase* > CheckerManager::get_checker(CheckerID id) {
    auto it = m_enabled_checkers.find(id);
    if (it == m_enabled_checkers.end()) {
        return std::nullopt;
    }
    return it->second.get();
}

void CheckerManager::add_checker_dependency(CheckerID id,
                                            AnalysisID required_analysis_id) {
    m_analysis_mgr.add_required_analysis(required_analysis_id);
}

void CheckerManager::register_for_stmt(internal::CheckStmtCallBack anz_fn,
                                       internal::MatchStmtCallBack match_fn,
                                       internal::CheckStmtKind kind) {
    m_stmt_checks.emplace_back(anz_fn, match_fn, kind);
}

void CheckerManager::register_for_begin_function(
    internal::CheckBeginFunctionCallBack anz_fn) {
    m_begin_function_checks.emplace_back(anz_fn);
}

void CheckerManager::register_for_end_function(
    internal::CheckEndFunctionCallBack anz_fn) {
    m_end_function_checks.emplace_back(anz_fn);
}

} // namespace knight::dfa