//===- intraprocedural_fixpoint.cpp -----------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements the intra-procedural fixpoint iterator for the
//  DFA engine.
//
//===------------------------------------------------------------------===//

#include "dfa/engine/intraprocedural_fixpoint.hpp"
#include "dfa/analysis/analysis_base.hpp"
#include "dfa/checker/checker_base.hpp"
#include "dfa/engine/block_engine.hpp"
#include "dfa/program_state.hpp"
#include "llvm/Support/raw_ostream.h"

namespace knight::dfa {

IntraProceduralFixpointIterator::IntraProceduralFixpointIterator(
    knight::KnightContext& ctx,
    AnalysisManager& analysis_mgr,
    CheckerManager& checker_mgr,
    ProgramStateManager& state_mgr,
    StackFrame* frame)
    : m_frame(frame),
      m_ctx(ctx),
      m_analysis_mgr(analysis_mgr),
      m_checker_mgr(checker_mgr),
      m_state_mgr(state_mgr),
      WtoBasedFixPointIterator(frame, state_mgr.get_bottom_state()) {}

ProgramStateRef IntraProceduralFixpointIterator::transfer_node(
    NodeRef node, ProgramStateRef pre_state) {
    BlockExecutionEngine engine(get_cfg(),
                                node,
                                m_analysis_mgr,
                                pre_state,
                                m_stmt_pre,
                                m_stmt_post);
    engine.exec();
    return pre_state;
}

ProgramStateRef IntraProceduralFixpointIterator::transfer_edge(
    [[maybe_unused]] NodeRef src,
    [[maybe_unused]] NodeRef dst,
    ProgramStateRef src_post_state) {
    return src_post_state;
}

void IntraProceduralFixpointIterator::check_pre(
    NodeRef node, [[maybe_unused]] const ProgramStateRef& state) {
    for (const auto& elem : node->Elements) {
        auto stmt_opt = elem.getAs< clang::CFGStmt >();
        if (!stmt_opt) {
            continue;
        }
        const auto* stmt = stmt_opt.value().getStmt();
        if (stmt == nullptr) {
            return;
        }

        auto it = m_stmt_pre.find(stmt);
        auto pre_state = it == m_stmt_pre.end() ? m_state_mgr.get_bottom_state()
                                                : it->second;
        m_checker_mgr.get_checker_context().set_current_state(pre_state);

        m_checker_mgr.run_checkers_for_pre_stmt(stmt);
    }
}

void IntraProceduralFixpointIterator::check_post(
    NodeRef node, [[maybe_unused]] const ProgramStateRef& state) {
    for (const auto& elem : node->Elements) {
        auto stmt_opt = elem.getAs< clang::CFGStmt >();
        if (!stmt_opt) {
            continue;
        }
        const auto* stmt = stmt_opt.value().getStmt();
        if (stmt == nullptr) {
            return;
        }
        auto it = m_stmt_post.find(stmt);
        auto post_state = it == m_stmt_post.end()
                              ? m_state_mgr.get_bottom_state()
                              : it->second;
        m_checker_mgr.get_checker_context().set_current_state(post_state);
        m_checker_mgr.run_checkers_for_post_stmt(stmt);
    }
}

void IntraProceduralFixpointIterator::run() {
    m_analysis_mgr.run_analyses_for_begin_function();
    m_checker_mgr.run_checkers_for_begin_function();

    FixPointIterator::run(m_state_mgr.get_bottom_state());

    NodeRef exit_node = ProcCFG::exit(get_cfg());
    m_analysis_mgr.run_analyses_for_end_function(exit_node);
    m_checker_mgr.run_checkers_for_end_function(exit_node);
}

} // namespace knight::dfa