//===- wto_iterator.hpp -----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the fixpoint iterator for the WTO algorithm.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/engine/iterator.hpp"
#include "dfa/location_context.hpp"
#include "dfa/program_state.hpp"
#include "dfa/stack_frame.hpp"
#include "support/graph.hpp"
#include "util/wto.hpp"

namespace knight::dfa {

namespace impl {

template < graph G, typename GraphTrait >
class WtoIterator;
template < graph G, typename GraphTrait >
class WtoChecker;

} // namespace impl

template < graph CFG, typename GraphTrait = GraphTrait< CFG > >
class WtoBasedFixPointIterator : public FixPointIterator< CFG, GraphTrait > {
    friend class impl::WtoIterator< CFG, GraphTrait >;

  public:
    using Base = FixPointIterator< CFG, GraphTrait >;
    using GraphRef = typename Base::GraphRef;
    using NodeRef = typename Base::NodeRef;
    using InvariantMap = typename Base::InvariantMap;
    using Wto = Wto< CFG, GraphTrait >;
    using WtoIterator = impl::WtoIterator< CFG, GraphTrait >;
    using WtoChecker = impl::WtoChecker< CFG, GraphTrait >;

  private:
    GraphRef m_cfg;
    Wto m_wto;
    InvariantMap m_pre;
    InvariantMap m_post;
    bool m_converged{};

    ProgramStateRef m_bottom;

  public:
    WtoBasedFixPointIterator(const StackFrame* frame, ProgramStateRef bottom)
        : m_cfg(frame->get_cfg()),
          m_wto(frame->get_cfg()),
          m_bottom(std::move(bottom)) {}
    WtoBasedFixPointIterator(const WtoBasedFixPointIterator&) = delete;
    WtoBasedFixPointIterator& operator=(const WtoBasedFixPointIterator&) =
        delete;
    WtoBasedFixPointIterator(WtoBasedFixPointIterator&&) = default;
    WtoBasedFixPointIterator& operator=(WtoBasedFixPointIterator&&) = default;
    ~WtoBasedFixPointIterator() = default;

  public:
    [[nodiscard]] bool is_converged() const override { return m_converged; }
    [[nodiscard]] GraphRef get_cfg() const override { return m_cfg; }
    [[nodiscard]] const Wto& get_wto() const { return m_wto; }
    [[nodiscard]] const ProgramStateRef& get_bottom() const { return m_bottom; }

  public:
    [[nodiscard]] ProgramStateRef get_pre(NodeRef node) const override {
        return get(m_pre, node);
    }

    [[nodiscard]] ProgramStateRef get_post(NodeRef node) const override {
        return get(m_post, node);
    }

    /// \brief Merge the state at cycle head after an increasing iteration
    ///
    /// \param head Head of the cycle
    /// \param iteration Iteration number
    /// \param state_before State before the iteration
    /// \param state_after State after the iteration
    [[nodiscard]] virtual ProgramStateRef merge_at_head_when_increasing(
        [[maybe_unused]] NodeRef head,
        unsigned iter_cnt,
        const ProgramStateRef& state_before,
        const ProgramStateRef& state_after,
        const LocationContext* loc_ctx) {
        if (iter_cnt < 2) {
            return state_before->join_consecutive_iter(state_after, loc_ctx);
        }
        return state_before->widen(state_after, loc_ctx);
    }

    /// \brief Check if the increasing iterations fixpoint is reached
    ///
    /// \param head Head of the cycle
    /// \param iter_cnt Iteration count
    /// \param state_before State before the iteration
    /// \param state_after State after the iteration
    [[nodiscard]] virtual bool is_increasing_fixpoint_reached(
        [[maybe_unused]] NodeRef head,
        [[maybe_unused]] unsigned iter_cnt,
        const ProgramStateRef& state_before,
        const ProgramStateRef& state_after) {
        return state_after->leq(*state_before);
    }

    /// \brief Narrow the state at loop head after a decreasing iteration
    ///
    /// \param head Head of the loop
    /// \param iter_cnt Iteration count
    /// \param state_before State before the iteration
    /// \param state_after State after the iteration
    [[nodiscard]] virtual ProgramStateRef narrow_at_loop_head_when_decreasing(
        [[maybe_unused]] NodeRef head,
        [[maybe_unused]] unsigned iter_cnt,
        [[maybe_unused]] const ProgramStateRef& state_before,
        [[maybe_unused]] const ProgramStateRef& state_after) {
        return state_before->narrow(state_after);
    }

    /// \brief Check if the decreasing iterations fixpoint is reached
    ///
    /// \param head Head of the loop
    /// \param iter_cnt Iteration count
    /// \param state_before State before the iteration
    /// \param state_after State after the iteration
    virtual bool is_decreasing_fixpoint_reached(
        [[maybe_unused]] NodeRef head,
        [[maybe_unused]] unsigned iter_cnt,
        [[maybe_unused]] const ProgramStateRef& state_before,
        [[maybe_unused]] const ProgramStateRef& state_after) {
        return state_before->leq(*state_after);
    }

    /// \brief Notify the beginning of handling a cycle
    virtual void notify_enter_cycle([[maybe_unused]] NodeRef head) {}

    /// \brief Notify the beginning of an iteration on a cycle
    virtual void notify_each_cycle_iteration(
        [[maybe_unused]] NodeRef head,
        [[maybe_unused]] unsigned iter_cnt,
        [[maybe_unused]] IterationKind kind) {}

    /// \brief Notify the end of the handling a cycle
    virtual void notify_exit_cycle([[maybe_unused]] NodeRef head) {}

    void run(ProgramStateRef init_state,
             LocationManager& loc_mgr,
             const StackFrame* frame) override {
        this->clear();
        this->set_pre(GraphTrait::entry(this->m_cfg), std::move(init_state));

        // Compute the fixpoint
        WtoIterator iterator(*this, loc_mgr, frame);
        this->m_wto.accept(iterator);
        this->m_converged = true;

        WtoChecker checker(*this, loc_mgr, frame);
        this->m_wto.accept(checker);
    }

    /// \brief Clear the current fixpoint
    void clear() override {
        this->m_converged = false;
        InvariantMap().swap(this->m_pre);
        InvariantMap().swap(this->m_post);
    }

  private:
    void set(InvariantMap& inv_map,
             const NodeRef& node,
             ProgramStateRef state) {
        state = state->normalize();
        inv_map[node] = std::move(state);
    }

    void set_pre(NodeRef node, ProgramStateRef state) {
        set(m_pre, node, std::move(state));
    }

    void set_post(NodeRef node, ProgramStateRef state) {
        set(m_post, node, std::move(state));
    }

    [[nodiscard]] const ProgramStateRef& get(const InvariantMap& inv_map,
                                             const NodeRef& node) const {
        auto it = inv_map.find(node);
        return it == inv_map.end() ? m_bottom : it->second;
    }

}; // class WtoBasedFixPointIterator

namespace impl {

template < graph G, typename GraphTrait = GraphTrait< G > >
class WtoIterator final : public WtoComponentVisitor< G, GraphTrait > {
  public:
    using WtoFPIterator = WtoBasedFixPointIterator< G, GraphTrait >;
    using GraphRef = typename WtoFPIterator::GraphRef;
    using NodeRef = typename WtoFPIterator::NodeRef;

    using Wto = Wto< G, GraphTrait >;
    using WtoNesting = WtoNesting< G, GraphTrait >;
    using WtoVertex = WtoVertex< G, GraphTrait >;
    using WtoCycle = WtoCycle< G, GraphTrait >;

  private:
    /// \brief Fixpoint iterator
    WtoFPIterator& m_fp_iterator;

    /// \brief Graph entry point
    NodeRef m_entry;

    /// \brief Location manager
    LocationManager& m_loc_mgr;

    /// \brief Stack frame
    const StackFrame* m_frame;

  public:
    explicit WtoIterator(WtoFPIterator& fp_iter,
                         LocationManager& loc_mgr,
                         const StackFrame* frame)
        : m_fp_iterator(fp_iter),
          m_loc_mgr(loc_mgr),
          m_frame(frame),
          m_entry(GraphTrait::entry(fp_iter.get_cfg())) {}

  public:
    /// \brief Update the pre and post states of a WTO vertex
    ///
    /// Use join of pred-dst edge out status to update the pre-state,
    /// and apply node transfer function on pre to update the post-state.
    void visit(const WtoVertex& vertex) override;

    void visit(const WtoCycle& cycle) override;

    const LocationContext* get_location_context(const NodeRef& node) const;

}; // class WtoIterator
template < graph G, typename GraphTrait >
class WtoChecker final : public WtoComponentVisitor< G, GraphTrait > {
  public:
    using WtoFPIterator = WtoBasedFixPointIterator< G, GraphTrait >;
    using GraphRef = typename WtoFPIterator::GraphRef;
    using NodeRef = typename WtoFPIterator::NodeRef;

    using Wto = Wto< G, GraphTrait >;
    using WtoNesting = WtoNesting< G, GraphTrait >;
    using WtoVertex = WtoVertex< G, GraphTrait >;
    using WtoCycle = WtoCycle< G, GraphTrait >;

  private:
    WtoFPIterator& m_fp_iterator;

    LocationManager& m_loc_mgr;

    const StackFrame* m_frame;

  public:
    explicit WtoChecker(WtoFPIterator& fp_iter,
                        LocationManager& loc_mgr,
                        const StackFrame* frame)
        : m_fp_iterator(fp_iter), m_loc_mgr(loc_mgr), m_frame(frame) {}

  public:
    void visit(const WtoVertex& vertex) override;

    void visit(const WtoCycle& cycle) override;

}; // class WtoChecker

} // namespace impl

} // namespace knight::dfa

#include "wto_iterator_impl.tpp"