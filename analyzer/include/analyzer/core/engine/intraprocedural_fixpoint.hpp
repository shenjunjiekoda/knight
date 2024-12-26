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
//  analyzer engine.
//
//===------------------------------------------------------------------===//

#pragma once

#include "analyzer/core/checker_manager.hpp"
#include "analyzer/core/engine/wto_iterator.hpp"
#include "analyzer/core/proc_cfg.hpp"
#include "analyzer/core/program_state.hpp"
#include "analyzer/core/stack_frame.hpp"
#include "analyzer/core/symbol_manager.hpp"
#include "common/support/graph.hpp"

namespace knight::analyzer {

class IntraProceduralFixpointIterator final
    : public WtoBasedFixPointIterator< ProcCFG, GraphTrait< ProcCFG > > {
  private:
    using GraphTraitT = GraphTrait< ProcCFG >;
    using FixPointIterator = WtoBasedFixPointIterator< ProcCFG, GraphTraitT >;
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

} // namespace knight::analyzer