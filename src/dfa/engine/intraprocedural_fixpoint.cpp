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
#include "dfa/engine/block_engine.hpp"
#include "dfa/program_state.hpp"
#include "dfa/analysis/analysis_base.hpp"
#include "dfa/checker/checker_base.hpp"

namespace knight::dfa {

IntraProceduralFixpointIterator::IntraProceduralFixpointIterator(
    knight::KnightContext& ctx,
    AnalysisManager& analysis_mgr,
    CheckerManager& checker_mgr,
    FunctionRef func)
    : m_func(func), m_ctx(ctx), m_analysis_mgr(analysis_mgr),
      m_checker_mgr(checker_mgr),
      WtoBasedFixPointIterator(ProcCFG::build(func),
                               create_and_set_state_manager(ctx, analysis_mgr)
                                   ->get_default_state()) {}

ProgramStateManager* IntraProceduralFixpointIterator::
    create_and_set_state_manager(knight::KnightContext& ctx,
                                 AnalysisManager& analysis_mgr) {
    this->m_state_mgr =
        std::make_unique< ProgramStateManager >(analysis_mgr,
                                                ctx.get_allocator());
    return m_state_mgr.get();
}

ProgramStateRef IntraProceduralFixpointIterator::transfer_node(
    NodeRef node, ProgramStateRef pre_state) {
    BlockEngine engine(get_cfg(), node, m_analysis_mgr, pre_state);
    engine.exec();
    return pre_state;
}

ProgramStateRef IntraProceduralFixpointIterator::transfer_edge(
    NodeRef src, NodeRef dst, ProgramStateRef src_post_state) {
    return src_post_state;
}

void IntraProceduralFixpointIterator::check_pre(NodeRef,
                                                const ProgramStateRef&) {}

void IntraProceduralFixpointIterator::check_post(NodeRef,
                                                 const ProgramStateRef&) {}

void IntraProceduralFixpointIterator::run() {
    m_analysis_mgr.run_analyses_for_begin_function();
    FixPointIterator::run(m_state_mgr->get_default_state());
    m_analysis_mgr.run_analyses_for_end_function(ProcCFG::exit(get_cfg()));
}

} // namespace knight::dfa