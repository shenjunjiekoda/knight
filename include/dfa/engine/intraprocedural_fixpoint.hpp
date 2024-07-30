//===- intraprocedural_fixpoint.hpp -----------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the intra-procedural fixpoint iterator for the
//  DFA engine.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/checker_manager.hpp"
#include "dfa/engine/wto_iterator.hpp"
#include "dfa/proc_cfg.hpp"
#include "dfa/program_state.hpp"
#include "support/graph.hpp"

namespace knight::dfa {

class IntraProceduralFixpointIterator final
    : public WtoBasedFixPointIterator< ProcCFG, GraphTrait< ProcCFG > > {
  private:
    using GraphTrait = GraphTrait< ProcCFG >;
    using FixPointIterator = WtoBasedFixPointIterator< ProcCFG, GraphTrait >;
    using GraphRef = typename FixPointIterator::GraphRef;
    using NodeRef = typename FixPointIterator::NodeRef;
    using FunctionRef = ProcCFG::FunctionRef;

  private:
    KnightContext& m_ctx;
    CheckerManager& m_checker_mgr;
    AnalysisManager& m_analysis_mgr;
    std::unique_ptr< ProgramStateManager > m_state_mgr;

  public:
    IntraProceduralFixpointIterator(knight::KnightContext& ctx,
                                    AnalysisManager& analysis_mgr,
                                    CheckerManager& checker_mgr,
                                    FunctionRef func);

    /// \brief transfer function for a graph node.
    ///
    /// \return the out program state after transfering to the given node.
    ProgramStateRef transfer_node(NodeRef node,
                                  ProgramStateRef pre_state) override;

    /// \brief transfer function for a graph edge.
    ///
    /// \return the out program state after transfering to the given edge,
    ///         to the in state of the destination node.
    ProgramStateRef transfer_edge(NodeRef src,
                                  NodeRef dst,
                                  ProgramStateRef src_post_state) override;

    /// \brief check the precondition of a node.
    void check_pre(NodeRef, const ProgramStateRef&) override;

    /// \brief check the postcondition of a node.
    void check_post(NodeRef, const ProgramStateRef&) override;

  private:
    ProgramStateManager* create_and_set_state_manager(
        knight::KnightContext& ctx, AnalysisManager& analysis_mgr);

}; // class IntraProceduralFixpointIterator

} // namespace knight::dfa