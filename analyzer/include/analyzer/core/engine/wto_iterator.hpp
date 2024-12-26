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

#include "analyzer/core/analyzer_options.hpp"
#include "analyzer/core/engine/iterator.hpp"
#include "analyzer/core/location_context.hpp"
#include "analyzer/core/program_state.hpp"
#include "analyzer/core/stack_frame.hpp"
#include "analyzer/util/wto.hpp"
#include "common/support/graph.hpp"

namespace knight::analyzer {

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
    using WtoT = Wto< CFG, GraphTrait >;
    using WtoIterator = impl::WtoIterator< CFG, GraphTrait >;
    using WtoChecker = impl::WtoChecker< CFG, GraphTrait >;
    using HeadThresholdMap = llvm::DenseMap< NodeRef, std::optional< ZNum > >;

  protected:
    AnalyzerOptions m_analyzer_opts;

    GraphRef m_cfg;
    WtoT m_wto;

    InvariantMap m_pre;
    InvariantMap m_post;
    HeadThresholdMap m_head_thresholds;
    bool m_converged{};

    ProgramStateRef m_bottom;

  public:
    WtoBasedFixPointIterator(AnalyzerOptions analyzer_opts,
                             const StackFrame* frame,
                             ProgramStateRef bottom)
        : m_analyzer_opts(analyzer_opts),
          m_cfg(frame->get_cfg()),
          m_wto(frame->get_cfg()),
          m_bottom(std::move(bottom)) {}
    WtoBasedFixPointIterator(const WtoBasedFixPointIterator&) = delete;
    WtoBasedFixPointIterator& operator=(const WtoBasedFixPointIterator&) =
        delete;
    WtoBasedFixPointIterator(WtoBasedFixPointIterator&&) = default;
    WtoBasedFixPointIterator& operator=(WtoBasedFixPointIterator&&) = default;
    ~WtoBasedFixPointIterator() = default;

  public:
    [[nodiscard]] const AnalyzerOptions& get_analyzer_options() const {
        return m_analyzer_opts;
    }
    [[nodiscard]] bool is_converged() const override { return m_converged; }
    [[nodiscard]] GraphRef get_cfg() const override { return m_cfg; }
    [[nodiscard]] const WtoT& get_wto() const { return m_wto; }
    [[nodiscard]] const ProgramStateRef& get_bottom() const { return m_bottom; }

  public:
    [[nodiscard]] ProgramStateRef get_pre(NodeRef node) const override {
        return get(m_pre, node);
    }

    [[nodiscard]] ProgramStateRef get_post(NodeRef node) const override {
        return get(m_post, node);
    }

    [[nodiscard]] std::optional< ZNum > get_threshold(NodeRef head) const;

    /// \brief Enlarge the state at cycle head after an increasing iteration
    ///
    /// \param head Head of the cycle
    /// \param iteration Iteration number
    /// \param state_before State before the iteration
    /// \param state_after State after the iteration
    [[nodiscard]] virtual ProgramStateRef enlarge_at_head_when_increasing(
        [[maybe_unused]] NodeRef head,
        unsigned iter_cnt,
        const ProgramStateRef& state_before,
        const ProgramStateRef& state_after,
        const LocationContext* loc_ctx) {
        if (iter_cnt < m_analyzer_opts.widening_delay + 1) {
            return state_before->join_consecutive_iter(state_after, loc_ctx);
        }
        if (m_analyzer_opts.analyze_with_threshold) {
            if (!m_head_thresholds.contains(head)) {
                m_head_thresholds[head] = get_threshold(head);
            }
            if (auto threshold = m_head_thresholds[head]) {
                return state_before->widen_with_threshold(state_after,
                                                          loc_ctx,
                                                          *threshold);
            }
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
        return iter_cnt == m_analyzer_opts.max_widening_iterations ||
               state_after->leq(*state_before);
    }

    /// \brief Narrow the state at loop head after a decreasing iteration
    ///
    /// \param head Head of the loop
    /// \param iter_cnt Iteration count
    /// \param state_before State before the iteration
    /// \param state_after State after the iteration
    [[nodiscard]] virtual ProgramStateRef refine_at_loop_head_when_decreasing(
        [[maybe_unused]] NodeRef head,
        [[maybe_unused]] unsigned iter_cnt,
        [[maybe_unused]] const ProgramStateRef& state_before,
        [[maybe_unused]] const ProgramStateRef& state_after) {
        if (m_analyzer_opts.analyze_with_threshold) {
            if (!m_head_thresholds.contains(head)) {
                m_head_thresholds[head] = get_threshold(head);
            }
            if (auto threshold = m_head_thresholds[head]) {
                return state_before->narrow_with_threshold(state_after,
                                                           *threshold);
            }
        }
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
        return iter_cnt == m_analyzer_opts.max_narrowing_iterations ||
               state_before->leq(*state_after);
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

} // namespace knight::analyzer

#include "wto_iterator_impl.tpp"