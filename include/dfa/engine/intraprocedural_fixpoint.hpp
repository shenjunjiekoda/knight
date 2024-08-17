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
#include "dfa/stack_frame.hpp"
#include "dfa/symbol_manager.hpp"
#include "support/graph.hpp"

namespace knight::dfa {

class IntraProceduralFixpointIterator final
    : public WtoBasedFixPointIterator< ProcCFG, GraphTrait< ProcCFG > > {
  private:
    using GraphTrait = GraphTrait< ProcCFG >;
    using FixPointIterator = WtoBasedFixPointIterator< ProcCFG, GraphTrait >;
    using GraphRef = typename FixPointIterator::GraphRef;
    using FunctionRef = ProcCFG::FunctionRef;
    using NodeRef = typename FixPointIterator::NodeRef;
    using StmtRef = ProcCFG::StmtRef;
    using StmtResultCache = std::unordered_map< StmtRef, ProgramStateRef >;

  private:
    KnightContext& m_ctx;
    CheckerManager& m_checker_mgr;
    AnalysisManager& m_analysis_mgr;
    SymbolManager& m_symbol_mgr;
    LocationManager& m_location_mgr;
    ProgramStateManager& m_state_mgr;
    const StackFrame* m_frame;

    StmtResultCache m_stmt_pre;
    StmtResultCache m_stmt_post;

  public:
    IntraProceduralFixpointIterator(knight::KnightContext& ctx,
                                    AnalysisManager& analysis_mgr,
                                    CheckerManager& checker_mgr,
                                    SymbolManager& symbol_mgr,
                                    LocationManager& location_mgr,
                                    ProgramStateManager& state_mgr,
                                    const StackFrame* frame);

    /// \brief transfer function for a graph node.
    ///
    /// \return the out program state after transferring to the given node.
    [[nodiscard]] ProgramStateRef transfer_node(
        NodeRef node, ProgramStateRef pre_state) override;

    /// \brief transfer function for a graph edge.
    ///
    /// \return the out program state after transferring to the given edge,
    ///         to the in state of the destination node.
    [[nodiscard]] ProgramStateRef transfer_edge(
        NodeRef src, NodeRef dst, ProgramStateRef src_post_state) override;

    /// \brief check the precondition of a node.
    void check_pre(NodeRef, const ProgramStateRef&) override;

    /// \brief check the postcondition of a node.
    void check_post(NodeRef, const ProgramStateRef&) override;

    void run();

}; // class IntraProceduralFixpointIterator

} // namespace knight::dfa