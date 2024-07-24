//===- analysis_manager.cpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the analysis manager.
//
//===------------------------------------------------------------------===//

#include "dfa/analysis_manager.hpp"
#include "dfa/analysis/analysis_base.hpp"
#include "util/assert.hpp"
#include <memory>

namespace knight::dfa {

bool AnalysisManager::is_analysis_registered(AnalysisID id) const {
    return m_analyses.count(id) > 0U;
}

void AnalysisManager::register_analysis(
    // TODO: add enabling logic for analyses (check filter and required checkers)
    std::unique_ptr< AnalysisBase > analysis) {
    auto id = analysis->get_id();
    m_analyses.emplace(id, std::move(analysis));
}

std::optional< AnalysisBase* > AnalysisManager::get_analysis(AnalysisID id) {
    auto it = m_analyses.find(id);
    if (it == m_analyses.end()) {
        return std::nullopt;
    }
    return it->second.get();
}

void AnalysisManager::add_analysis_dependency(AnalysisID id,
                                              AnalysisID required_id) {
    if (!is_analysis_registered(required_id)) {
        m_required_analyses.insert(required_id);
    }

    // Check for circular dependency.
    if (m_analysis_dependencies.count(required_id) > 0U) {
        knight_assert_msg(m_analysis_dependencies[required_id].count(id) == 0U,
                          "Circular dependency detected");
    }

    m_analysis_dependencies[id].insert(required_id);

    // Add dependencies of required analysis recursively.
    if (m_analysis_dependencies.count(required_id) > 0U) {
        for (auto dep_id : m_analysis_dependencies[required_id]) {
            add_analysis_dependency(id, dep_id);
        }
    }
}

std::unordered_set< AnalysisID > AnalysisManager::get_analysis_dependencies(
    AnalysisID id) const {
    auto it = m_analysis_dependencies.find(id);
    if (it == m_analysis_dependencies.end()) {
        return {};
    }
    return it->second;
}

void AnalysisManager::register_domain(AnalysisID analysis_id, DomID dom_id) {
    if (is_analysis_registered(analysis_id)) {
        m_analysis_domains[analysis_id].insert(dom_id);
    }
    m_analysis_domains[analysis_id].insert(dom_id);
}

std::unordered_set< DomID > AnalysisManager::get_registered_domains_in(
    AnalysisID id) const {
    auto it = m_analysis_domains.find(id);
    if (it == m_analysis_domains.end()) {
        return {};
    }
    return it->second;
}

void AnalysisManager::add_dom_dependency(DomID id, DomID required_id) {
    // Check for circular dependency.
    if (m_dom_dependencies.count(required_id) > 0U) {
        knight_assert_msg(m_dom_dependencies[required_id].count(id) == 0U,
                          "Circular dependency detected");
    }

    m_dom_dependencies[id].insert(required_id);

    // Add dependencies of required dom recursively.
    if (m_dom_dependencies.count(required_id) > 0U) {
        for (auto dep_id : m_dom_dependencies[required_id]) {
            add_dom_dependency(id, dep_id);
        }
    }
}

void AnalysisManager::register_for_stmt(internal::AnalyzeStmtCallBack anz_fn,
                                        internal::MatchStmtCallBack match_fn,
                                        internal::VisitStmtKind kind) {
    m_stmt_analyses.emplace_back(anz_fn, match_fn, kind);
}

void AnalysisManager::register_for_begin_function(
    internal::AnalyzeBeginFunctionCallBack anz_fn) {
    m_begin_function_analyses.emplace_back(anz_fn);
}

void AnalysisManager::register_for_end_function(
    internal::AnalyzeEndFunctionCallBack anz_fn) {
    m_end_function_analyses.emplace_back(anz_fn);
}

} // namespace knight::dfa