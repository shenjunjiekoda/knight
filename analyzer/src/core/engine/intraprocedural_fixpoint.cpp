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
//  analyzer engine.
//
//===------------------------------------------------------------------===//

#include "analyzer/core/engine/intraprocedural_fixpoint.hpp"
#include "analyzer/core/analysis/analysis_base.hpp"
#include "analyzer/core/checker/checker_base.hpp"
#include "analyzer/core/checker_context.hpp"
#include "analyzer/core/engine/block_engine.hpp"
#include "analyzer/core/proc_cfg.hpp"
#include "analyzer/core/program_state.hpp"
#include "common/util/log.hpp"

#define DEBUG_TYPE "intra-fixpoint"

namespace knight::analyzer {

IntraProceduralFixpointIterator::IntraProceduralFixpointIterator(
    knight::KnightContext& ctx,
    AnalysisManager& analysis_mgr,
    CheckerManager& checker_mgr,
    LocationManager& location_mgr,
    ProgramStateManager& state_mgr,
    const StackFrame* frame)
    : m_frame(frame),
      m_ctx(ctx),
      m_analysis_mgr(analysis_mgr),
      m_checker_mgr(checker_mgr),
      m_symbol_mgr(analysis_mgr.get_symbol_manager()),
      m_location_mgr(location_mgr),
      m_state_mgr(state_mgr),
      WtoBasedFixPointIterator(ctx.get_current_options().analyzer_opts,
                               frame,
                               state_mgr.get_bottom_state()) {}

ProgramStateRef IntraProceduralFixpointIterator::transfer_node(
    NodeRef node, ProgramStateRef pre_state) {
    if (ProcCFG::entry(get_cfg()) == node) {
        AnalysisContext analysis_ctx(m_ctx,
                                     m_analysis_mgr.get_region_manager(),
                                     m_frame,
                                     m_symbol_mgr,
                                     m_location_mgr
                                         .create_location_context(m_frame,

                                                                  node));
        analysis_ctx.set_state(pre_state);
        m_analysis_mgr.run_analyses_for_begin_function(analysis_ctx);
    }
    if (ProcCFG::exit(get_cfg()) == node) {
        AnalysisContext analysis_ctx(m_ctx,
                                     m_analysis_mgr.get_region_manager(),
                                     m_frame,
                                     m_symbol_mgr,
                                     m_location_mgr
                                         .create_location_context(m_frame,
                                                                  -1,
                                                                  node));
        analysis_ctx.set_state(get_post(node));
        m_analysis_mgr.run_analyses_for_end_function(analysis_ctx, node);
    }

    BlockExecutionEngine engine(get_cfg(),
                                node,
                                m_analysis_mgr,
                                m_symbol_mgr,
                                m_location_mgr,
                                pre_state,
                                m_stmt_pre,
                                m_stmt_post,
                                m_frame);
    engine.exec();

    knight_log_nl(llvm::outs()
                      << "after transfer node: " << node->getBlockID() << " "
                      << "state: ";
                  engine.get_state()->dump(llvm::outs());
                  llvm::outs() << "\n";);

    return engine.get_state();
}

ProgramStateRef IntraProceduralFixpointIterator::transfer_edge(
    [[maybe_unused]] NodeRef src,
    [[maybe_unused]] NodeRef dst,
    ProgramStateRef src_post_state) {
    // TODO(branch): add transfer logic for condition filter
    return src_post_state;
}

void IntraProceduralFixpointIterator::check_pre(
    NodeRef node, [[maybe_unused]] const ProgramStateRef& state) {
    CheckerContext checker_ctx(m_ctx, m_frame, m_symbol_mgr, nullptr);
    int elem_idx = -1;

    auto set_loc = [&]() {
        checker_ctx.set_location_context(
            m_location_mgr.create_location_context(m_frame, elem_idx, node));
    };

    if (node->empty()) {
        set_loc();
        if (node == ProcCFG::entry(get_cfg())) {
            auto init_state = get_pre(node);
            checker_ctx.set_current_state(init_state);
            m_checker_mgr.run_checkers_for_begin_function(checker_ctx);
        }
        return;
    }
    for (const auto& elem : node->Elements) {
        elem_idx++;
        auto stmt_opt = elem.getAs< clang::CFGStmt >();
        if (!stmt_opt) {
            continue;
        }
        const auto* stmt = stmt_opt.value().getStmt();
        if (stmt == nullptr) {
            continue;
        }

        auto it = m_stmt_pre.find(stmt);
        auto pre_state = it == m_stmt_pre.end() ? m_state_mgr.get_bottom_state()
                                                : it->second;

        knight_log_nl(llvm::outs() << "pre state for stmt: "; stmt->dumpColor();
                      llvm::outs() << " : ";
                      pre_state->dump(llvm::outs());
                      llvm::outs() << "\n";
                      llvm::outs() << "state in cache: "
                                   << (it != m_stmt_pre.end()) << "\n";);

        checker_ctx.set_current_state(pre_state);
        set_loc();

        m_checker_mgr.run_checkers_for_pre_stmt(checker_ctx, stmt);
    }
}

void IntraProceduralFixpointIterator::check_post(
    NodeRef node, [[maybe_unused]] const ProgramStateRef& state) {
    CheckerContext checker_ctx(m_ctx, m_frame, m_symbol_mgr, nullptr);
    int elem_idx = -1;

    auto set_loc = [&]() {
        checker_ctx.set_location_context(
            m_location_mgr.create_location_context(m_frame, elem_idx, node));
    };
    if (node->empty()) {
        set_loc();
        if (node == ProcCFG::exit(get_cfg())) {
            auto exit_state = get_post(node);
            checker_ctx.set_current_state(exit_state);
            m_checker_mgr.run_checkers_for_end_function(checker_ctx, node);
        }
        return;
    }

    for (const auto& elem : node->Elements) {
        elem_idx++;
        auto stmt_opt = elem.getAs< clang::CFGStmt >();
        if (!stmt_opt) {
            continue;
        }
        const auto* stmt = stmt_opt.value().getStmt();
        if (stmt == nullptr) {
            continue;
        }
        auto it = m_stmt_post.find(stmt);
        auto post_state = it == m_stmt_post.end()
                              ? m_state_mgr.get_bottom_state()
                              : it->second;

        checker_ctx.set_current_state(post_state);
        set_loc();

        m_checker_mgr.run_checkers_for_post_stmt(checker_ctx, stmt);
    }
}

void IntraProceduralFixpointIterator::run() {
    FixPointIterator::run(m_state_mgr.get_default_state(),
                          m_location_mgr,
                          m_frame);
}

} // namespace knight::analyzer