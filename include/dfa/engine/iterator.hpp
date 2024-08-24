//===- iterator.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the fixpoint iterator for the DFA engine.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/location_manager.hpp"
#include "dfa/program_state.hpp"
#include "dfa/stack_frame.hpp"
#include "support/graph.hpp"

namespace knight::dfa {

enum class IterationKind { Increasing, Decreasing };

template < graph G, typename GraphTrait = GraphTrait< G > >
class FixPointIterator {
  public:
    using GraphRef = GraphTrait::GraphRef;
    using NodeRef = GraphTrait::NodeRef;
    using InvariantMap = std::unordered_map< NodeRef, ProgramStateRef >;

  public:
    FixPointIterator() = default;
    FixPointIterator(const FixPointIterator&) noexcept = default;
    FixPointIterator(FixPointIterator&&) noexcept = default;
    FixPointIterator& operator=(const FixPointIterator&) noexcept = default;
    FixPointIterator& operator=(FixPointIterator&&) noexcept = default;
    virtual ~FixPointIterator() = default;

  public:
    virtual void run(ProgramStateRef init_state,
                     LocationManager& loc_mgr,
                     const StackFrame* frame) = 0;

    [[nodiscard]] virtual GraphRef get_cfg() const = 0;

    [[nodiscard]] virtual ProgramStateRef get_pre(NodeRef node) const = 0;
    [[nodiscard]] virtual ProgramStateRef get_post(NodeRef node) const = 0;
    [[nodiscard]] virtual bool is_converged() const = 0;
    virtual void clear() = 0;

    /// \brief transfer function for a graph node.
    ///
    /// \return the out program state after transferring to the given node.
    [[nodiscard]] virtual ProgramStateRef transfer_node(
        NodeRef node, ProgramStateRef pre_state) = 0;

    /// \brief transfer function for a graph edge.
    ///
    /// \return the out program state after transferring to the given edge,
    ///         to the in state of the destination node.
    [[nodiscard]] virtual ProgramStateRef transfer_edge(
        NodeRef src, NodeRef dst, ProgramStateRef src_post_state) = 0;

    /// \brief check the precondition of a node.
    virtual void check_pre(NodeRef, const ProgramStateRef&) = 0;

    /// \brief check the postcondition of a node.
    virtual void check_post(NodeRef, const ProgramStateRef&) = 0;

}; // class FixPointIterator

} // namespace knight::dfa