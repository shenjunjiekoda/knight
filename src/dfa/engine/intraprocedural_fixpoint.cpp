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
#include "llvm/Support/raw_ostream.h"

namespace knight::dfa {

IntraProceduralFixpointIterator::IntraProceduralFixpointIterator(
    knight::KnightContext& ctx,
    AnalysisManager& analysis_mgr,
    CheckerManager& checker_mgr,
    ProgramStateManager& state_mgr,
    FunctionRef func)
    : m_func(func), m_ctx(ctx), m_analysis_mgr(analysis_mgr),
      m_checker_mgr(checker_mgr), m_state_mgr(state_mgr),
      WtoBasedFixPointIterator(ProcCFG::build(func),
                               state_mgr.get_default_state()) {}

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
    FixPointIterator::run(m_state_mgr.get_default_state());
    m_analysis_mgr.run_analyses_for_end_function(ProcCFG::exit(get_cfg()));
}

} // namespace knight::dfa