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
#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"
#include "dfa/program_state.hpp"
#include "dfa/symbol_manager.hpp"
#include "util/assert.hpp"

#include "util/log.hpp"

#include <memory>
#include <queue>

#define DEBUG_TYPE "AnalysisManager"

namespace knight::dfa {

namespace {

std::vector< AnalysisID > compute_topological_order(
    const std::unordered_map< AnalysisID, AnalysisIDSet >& dependencies,
    const AnalysisIDSet& all_ids,
    const AnalysisIDSet& privileged_ids) {
    std::unordered_map< AnalysisID, int > in_degree;
    std::queue< AnalysisID > zero_in_degree;
    std::vector< AnalysisID > sorted_ids;

    // Firstly, add privileged analyses to the sorted list.
    // TODO: shall we add privileged analysis ordering by dependencies here?
    for (const auto& id : privileged_ids) {
        sorted_ids.push_back(id);
    }

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
        const AnalysisID current = zero_in_degree.front();
        zero_in_degree.pop();
        if (std::find(sorted_ids.begin(), sorted_ids.end(), current) ==
            sorted_ids.end()) {
            sorted_ids.push_back(current);
        }
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
    for (const AnalysisID id : full_order) {
        if (subset.find(id) != subset.end()) {
            res.push_back(id);
        }
    }
    return res;
}

} // anonymous namespace

AnalysisManager::AnalysisManager(KnightContext& ctx) : m_ctx(ctx) {
    m_sym_mgr = std::make_unique< dfa::SymbolManager >(m_ctx.get_allocator());
    m_region_mgr =
        std::make_unique< dfa::RegionManager >(m_ctx.get_allocator());
    m_state_mgr =
        std::make_unique< dfa::ProgramStateManager >(*this,
                                                     *m_region_mgr,
                                                     *m_sym_mgr,
                                                     m_ctx.get_allocator());
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
        knight_log(llvm::outs() << "push one already in required analyses: "
                                << get_analysis_name_by_id(id) << "\n");
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

        knight_log(llvm::outs() << "add required in dep-computing: "
                                << get_analysis_name_by_id(id) << "\n";);

        for (auto dep_id : get_analysis_dependencies(id)) {
            knight_log(llvm::outs()
                           << "current:" << get_analysis_name_by_id(id)
                           << " dep:" << get_analysis_name_by_id(dep_id)
                           << "\n";);

            q.push(dep_id);
        }
    }
}

void AnalysisManager::compute_full_order_analyses_after_registry() {
    m_analysis_full_order = compute_topological_order(m_analysis_dependencies,
                                                      m_analyses,
                                                      m_privileged_analysis);
}

void AnalysisManager::enable_analysis(
    std::unique_ptr< AnalysisBase > analysis) {
    auto id = get_analysis_id(analysis->kind);
    m_enabled_analyses.emplace(id, std::move(analysis));
    knight_log(llvm::outs()
               << "Enabling analysis " << get_analysis_name_by_id(id) << "\n");
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

void AnalysisManager::register_for_stmt(internal::AnalyzeStmtCallBack cb,
                                        internal::MatchStmtCallBack match_cb,
                                        internal::VisitStmtKind kind) {
    m_stmt_analyses.push_back({cb, match_cb, kind});
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
    AnalysisContext& analysis_ctx,
    internal::StmtRef stmt,
    internal::VisitStmtKind visit_kind) {
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
        (*callbacks[id])(stmt, analysis_ctx);
    }
}

void AnalysisManager::run_analyses_for_pre_stmt(AnalysisContext& analysis_ctx,
                                                internal::StmtRef stmt) {
    run_analyses_for_stmt(analysis_ctx, stmt, internal::VisitStmtKind::Pre);
}
void AnalysisManager::run_analyses_for_eval_stmt(AnalysisContext& analysis_ctx,
                                                 internal::StmtRef stmt) {
    run_analyses_for_stmt(analysis_ctx, stmt, internal::VisitStmtKind::Eval);
}
void AnalysisManager::run_analyses_for_post_stmt(AnalysisContext& analysis_ctx,
                                                 internal::StmtRef stmt) {
    run_analyses_for_stmt(analysis_ctx, stmt, internal::VisitStmtKind::Post);
}

void AnalysisManager::run_analyses_for_begin_function(
    AnalysisContext& analysis_ctx) {
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
        (*callbacks[id])(analysis_ctx);
    }
}
void AnalysisManager::run_analyses_for_end_function(
    AnalysisContext& analysis_ctx, ProcCFG::NodeRef node) {
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
        (*callbacks[id])(node, analysis_ctx);
    }
}

dfa::ProgramStateManager& AnalysisManager::get_state_manager() const {
    return *m_state_mgr;
}

dfa::SymbolManager& AnalysisManager::get_symbol_manager() const {
    return *m_sym_mgr;
}

} // namespace knight::dfa