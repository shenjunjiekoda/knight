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
#include "dfa/checker_context.hpp"
#include "dfa/engine/block_engine.hpp"
#include "dfa/program_state.hpp"
#include "llvm/Support/raw_ostream.h"

namespace knight::dfa {

IntraProceduralFixpointIterator::IntraProceduralFixpointIterator(
    knight::KnightContext& ctx,
    AnalysisManager& analysis_mgr,
    CheckerManager& checker_mgr,
    ProgramStateManager& state_mgr,
    const StackFrame* frame)
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
                                m_stmt_post,
                                m_frame);
    engine.exec();
    return engine.get_state();
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

        CheckerContext checker_ctx(m_ctx);
        checker_ctx.set_current_state(pre_state);
        checker_ctx.set_current_stack_frame(m_frame);

        m_checker_mgr.run_checkers_for_pre_stmt(checker_ctx, stmt);
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
        CheckerContext checker_ctx(m_ctx);
        checker_ctx.set_current_state(post_state);
        checker_ctx.set_current_stack_frame(m_frame);
        m_checker_mgr.run_checkers_for_post_stmt(checker_ctx, stmt);
    }
}

void IntraProceduralFixpointIterator::run() {
    auto initial_state = m_state_mgr.get_default_state();

    AnalysisContext analysis_ctx(m_ctx, m_analysis_mgr.get_region_manager());
    analysis_ctx.set_current_stack_frame(m_frame);
    analysis_ctx.set_state(initial_state);

    CheckerContext checker_ctx(m_ctx);
    checker_ctx.set_current_stack_frame(m_frame);
    checker_ctx.set_current_state(initial_state);

    m_analysis_mgr.run_analyses_for_begin_function(analysis_ctx);

    m_checker_mgr.run_checkers_for_begin_function(checker_ctx);

    FixPointIterator::run(initial_state);

    NodeRef exit_node = ProcCFG::exit(get_cfg());

    auto exit_state = get_post(exit_node);
    analysis_ctx.set_state(exit_state);
    m_analysis_mgr.run_analyses_for_end_function(analysis_ctx, exit_node);

    checker_ctx.set_current_state(exit_state);
    m_checker_mgr.run_checkers_for_end_function(checker_ctx, exit_node);
}

} // namespace knight::dfa