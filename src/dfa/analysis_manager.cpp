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
#include "dfa/analysis/analyses.hpp"
#include "dfa/analysis/analysis_base.hpp"
#include "dfa/analysis_context.hpp"
#include "dfa/domain/domains.hpp"
#include "util/assert.hpp"

#include <llvm/Support/raw_ostream.h>

#include <memory>
#include <queue>

namespace knight::dfa {

namespace {

std::vector< AnalysisID > compute_topological_order(
    const std::unordered_map< AnalysisID, std::unordered_set< AnalysisID > >&
        dependencies,
    const std::unordered_set< AnalysisID >& all_ids) {
    std::unordered_map< AnalysisID, int > in_degree;
    std::queue< AnalysisID > zero_in_degree;
    std::vector< AnalysisID > sorted_ids;

    for (const auto& id : all_ids) {
        in_degree[id] = 0;
    }

    for (const auto& pair : dependencies) {
        for (const auto& dep : pair.second) {
            in_degree[dep]++;
        }
    }

    for (const auto& id : all_ids) {
        if (in_degree[id] == 0) {
            zero_in_degree.push(id);
        }
    }

    while (!zero_in_degree.empty()) {
        AnalysisID current = zero_in_degree.front();
        zero_in_degree.pop();
        sorted_ids.push_back(current);
        auto it = dependencies.find(current);
        if (it == dependencies.end()) {
            continue;
        }
        for (const AnalysisID& neighbor : it->second) {
            if (--in_degree[neighbor] == 0) {
                zero_in_degree.push(neighbor);
            }
        }
    }

    knight_assert_msg(sorted_ids.size() == all_ids.size(),
                      "Cycle detected in the graph");

    return sorted_ids;
}

std::vector< AnalysisID > get_subset_order(
    const std::vector< AnalysisID >& full_order, const AnalysisIDSet& subset) {
    std::vector< AnalysisID > res;
    for (AnalysisID id : full_order) {
        if (subset.find(id) != subset.end()) {
            res.push_back(id);
        }
    }
    return res;
}

} // anonymous namespace

AnalysisManager::AnalysisManager(KnightContext& ctx)
    : m_ctx(ctx), m_analysis_ctx(std::make_unique< AnalysisContext >(ctx)) {}

AnalysisContext& AnalysisManager::get_analysis_context() const {
    return *m_analysis_ctx;
}

bool AnalysisManager::is_analysis_required(AnalysisID id) const {
    return m_required_analyses.contains(id);
}

void AnalysisManager::add_required_analysis(AnalysisID id) {
    m_required_analyses.insert(id);
}

void AnalysisManager::add_analysis_dependency(AnalysisID id,
                                              AnalysisID required_id) {
    if (m_analysis_dependencies.contains(required_id)) {
        knight_assert_msg(!m_analysis_dependencies[required_id].contains(id),
                          "Circular dependency detected");
    }

    m_analysis_dependencies[id].insert(required_id);

    // Add dependencies of required analysis recursively.
    if (m_analysis_dependencies.contains(required_id)) {
        for (auto dep_id : m_analysis_dependencies[required_id]) {
            add_analysis_dependency(id, dep_id);
        }
    }
}

void AnalysisManager::compute_all_required_analyses_by_dependencies() {
    std::queue< AnalysisID > q;
    for (auto id : m_required_analyses) {
        q.push(id);
    }

    AnalysisIDSet visited;
    while (!q.empty()) {
        auto id = q.front();
        q.pop();
        if (visited.contains(id)) {
            continue;
        }
        visited.insert(id);
        add_required_analysis(id);
        for (auto dep_id : get_analysis_dependencies(id)) {
            q.push(dep_id);
        }
    }
}

void AnalysisManager::compute_full_order_analyses_after_registry() {
    m_analysis_full_order =
        compute_topological_order(m_analysis_dependencies, m_analyses);
}

void AnalysisManager::enable_analysis(
    std::unique_ptr< AnalysisBase > analysis) {
    auto id = get_analysis_id(analysis->kind);
    m_enabled_analyses.emplace(id, std::move(analysis));
}

std::optional< AnalysisBase* > AnalysisManager::get_analysis(AnalysisID id) {
    auto it = m_enabled_analyses.find(id);
    if (it == m_enabled_analyses.end()) {
        return std::nullopt;
    }
    return it->second.get();
}

std::unordered_set< AnalysisID > AnalysisManager::get_analysis_dependencies(
    AnalysisID id) const {
    auto it = m_analysis_dependencies.find(id);
    if (it == m_analysis_dependencies.end()) {
        return {};
    }
    return it->second;
}

std::unordered_set< DomID > AnalysisManager::get_registered_domains_in(
    AnalysisID id) const {
    auto it = m_analysis_domains.find(id);
    if (it == m_analysis_domains.end()) {
        return {};
    }
    return it->second;
}

std::optional< AnalysisManager::DomainDefaultValFn > AnalysisManager::
    get_domain_default_val_fn(DomID id) const {
    auto it = m_domain_default_fn.find(id);
    if (it == m_domain_default_fn.end()) {
        return std::nullopt;
    }
    return it->second;
}
std::optional< AnalysisManager::DomainBottomValFn > AnalysisManager::
    get_domain_bottom_val_fn(DomID id) const {
    auto it = m_domain_bottom_fn.find(id);
    if (it == m_domain_bottom_fn.end()) {
        return std::nullopt;
    }
    return it->second;
}

void AnalysisManager::register_for_stmt(internal::AnalyzeStmtCallBack cb,
                                        internal::MatchStmtCallBack match_cb,
                                        internal::VisitStmtKind kind) {
    m_stmt_analyses.emplace_back(cb, match_cb, kind);
}

void AnalysisManager::register_for_begin_function(
    internal::AnalyzeBeginFunctionCallBack cb) {
    m_begin_function_analyses.emplace_back(cb);
}

void AnalysisManager::register_for_end_function(
    internal::AnalyzeEndFunctionCallBack cb) {
    m_end_function_analyses.emplace_back(cb);
}

void AnalysisManager::run_analyses_for_stmt(
    internal::StmtRef stmt, internal::VisitStmtKind visit_kind) {
    AnalysisIDSet tgt_ids;
    std::unordered_map< AnalysisID, internal::AnalyzeStmtCallBack* > callbacks;
    for (auto& info : m_stmt_analyses) {
        if (info.kind != visit_kind || !info.match_cb(stmt)) {
            continue;
        }
        auto& callback = info.anz_cb;
        auto id = callback.get_id();
        if (is_analysis_required(id)) {
            tgt_ids.insert(id);
            callbacks.emplace(id, &callback);
        }
    }

    for (auto id : get_subset_order(m_analysis_full_order, tgt_ids)) {
        (*callbacks[id])(stmt, *m_analysis_ctx);
    }
}

void AnalysisManager::run_analyses_for_pre_stmt(internal::StmtRef stmt) {
    run_analyses_for_stmt(stmt, internal::VisitStmtKind::Pre);
}
void AnalysisManager::run_analyses_for_eval_stmt(internal::StmtRef stmt) {
    run_analyses_for_stmt(stmt, internal::VisitStmtKind::Eval);
}
void AnalysisManager::run_analyses_for_post_stmt(internal::StmtRef stmt) {
    run_analyses_for_stmt(stmt, internal::VisitStmtKind::Post);
}

void AnalysisManager::run_analyses_for_begin_function() {
    AnalysisIDSet tgt_ids;
    std::unordered_map< AnalysisID, internal::AnalyzeBeginFunctionCallBack* >
        callbacks;
    for (auto& info : m_begin_function_analyses) {
        auto id = info.get_id();
        if (is_analysis_required(id)) {
            tgt_ids.insert(id);
            callbacks.emplace(id, &info);
        }
    }
    for (auto id : get_subset_order(m_analysis_full_order, tgt_ids)) {
        (*callbacks[id])(*m_analysis_ctx);
    }
}
void AnalysisManager::run_analyses_for_end_function(ProcCFG::NodeRef node) {
    AnalysisIDSet tgt_ids;
    std::unordered_map< AnalysisID, internal::AnalyzeEndFunctionCallBack* >
        callbacks;
    for (auto& info : m_end_function_analyses) {
        auto id = info.get_id();
        if (is_analysis_required(id)) {
            tgt_ids.insert(id);
            callbacks.emplace(id, &info);
        }
    }
    for (auto id : get_subset_order(m_analysis_full_order, tgt_ids)) {
        (*callbacks[id])(node, *m_analysis_ctx);
    }
}

} // namespace knight::dfa