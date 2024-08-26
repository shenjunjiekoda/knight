#pragma once

#include "dfa/location_context.hpp"
#include "util/log.hpp"
#include "wto_iterator.hpp"

#ifdef DEBUG_TYPE
#    define DEBUG_TYPE_BACKUP DEBUG_TYPE
#    undef DEBUG_TYPE
#endif

#define DEBUG_TYPE "wto-iterator"

namespace knight::dfa::impl {

template < graph G, typename GraphTrait >
void WtoIterator< G, GraphTrait >::visit(const WtoVertex& vertex) {
    auto node = vertex.get_node();
    ProgramStateRef state_pre = this->m_fp_iterator.get_pre(node);
    knight_log(llvm::outs()
               << "wto visit node: " << node->getBlockID() << "\n");

    for (auto it = GraphTrait::pred_begin(node),
              end = GraphTrait::pred_end(node);
         it != end;
         ++it) {
        auto pred = *it;

        knight_log(llvm::outs()
                       << "join pred `" << node->getBlockID() << "` with"
                       << " node `" << pred->getBlockID() << "`\n";);
        knight_log_nl(llvm::outs() << "state_pre before join: ";
                      state_pre->dump(llvm::outs()););
        knight_log_nl(
            llvm::outs() << "\npre transfer edge state: ";
            this->m_fp_iterator
                .transfer_edge(pred, node, this->m_fp_iterator.get_post(pred))
                ->dump(llvm::outs()););

        state_pre =
            state_pre->join(this->m_fp_iterator
                                .transfer_edge(pred,
                                               node,
                                               this->m_fp_iterator.get_post(
                                                   pred)),
                            get_location_context(node));

        knight_log_nl(llvm::outs() << "state_pre after join: ";
                      state_pre->dump(llvm::outs()););
    }
    this->m_fp_iterator.set_pre(node, state_pre);
    this->m_fp_iterator
        .set_post(node,
                  this->m_fp_iterator.transfer_node(node,
                                                    std::move(state_pre)));
}

template < graph G, typename GraphTrait >
void WtoIterator< G, GraphTrait >::visit(const WtoCycle& cycle) {
    auto head = cycle.get_head();
    ProgramStateRef state_pre = this->m_fp_iterator.get_bottom();
    auto& wto = this->m_fp_iterator.get_wto();
    const auto& nesting = wto.get_nesting(head);

    this->m_fp_iterator.notify_enter_cycle(head);

    const auto* loc_ctx = get_location_context(head);

    for (auto it = GraphTrait::pred_begin(head),
              end = GraphTrait::pred_end(head);
         it != end;
         ++it) {
        auto pred = *it;
        if (wto.get_nesting(pred) <= nesting) {
            state_pre =
                state_pre->join(this->m_fp_iterator
                                    .transfer_edge(pred,
                                                   head,
                                                   this->m_fp_iterator.get_post(
                                                       pred)),
                                loc_ctx);
        }
    }

    // Compute the fixpoint
    IterationKind kind = IterationKind::Increasing;
    for (unsigned iter_cnt = 1;; ++iter_cnt) {
        this->m_fp_iterator.notify_each_cycle_iteration(head, iter_cnt, kind);
        this->m_fp_iterator.set_pre(head, state_pre);
        this->m_fp_iterator
            .set_post(head, this->m_fp_iterator.transfer_node(head, state_pre));

        for (auto* component : cycle.components()) {
            component->accept(*this);
        }

        // from the head of loop
        ProgramStateRef new_state_front = this->m_fp_iterator.get_bottom();
        // from the tail of loop
        ProgramStateRef new_state_back = this->m_fp_iterator.get_bottom();

        for (auto it = GraphTrait::pred_begin(head),
                  end = GraphTrait::pred_end(head);
             it != end;
             ++it) {
            auto pred = *it;
            ProgramStateRef head_in =
                this->m_fp_iterator.transfer_edge(pred,
                                                  head,
                                                  this->m_fp_iterator.get_post(
                                                      pred));
            if (wto.get_nesting(pred) <= nesting) {
                new_state_front = new_state_front->join(head_in, loc_ctx);
            } else {
                new_state_back = new_state_back->join(head_in, loc_ctx);
            }
        }

        new_state_front =
            new_state_front->join_at_loop_head(new_state_back, loc_ctx);
        new_state_front = new_state_front->normalize();
        if (kind == IterationKind::Increasing) {
            ProgramStateRef increased =
                this->m_fp_iterator
                    .merge_at_head_when_increasing(head,
                                                   iter_cnt,
                                                   state_pre,
                                                   new_state_front,
                                                   get_location_context(head));
            increased = increased->normalize();
            if (this->m_fp_iterator.is_increasing_fixpoint_reached(head,
                                                                   iter_cnt,
                                                                   state_pre,
                                                                   increased)) {
                // Increasing fixpoint is reached
                kind = IterationKind::Decreasing;
                iter_cnt = 1U;
            } else {
                state_pre = std::move(increased);
            }
        }

        if (kind == IterationKind::Decreasing) {
            ProgramStateRef refined =
                this->m_fp_iterator
                    .narrow_at_loop_head_when_decreasing(head,
                                                         iter_cnt,
                                                         state_pre,
                                                         new_state_front);
            refined = refined->normalize();
            if (this->m_fp_iterator.is_decreasing_fixpoint_reached(head,
                                                                   iter_cnt,
                                                                   state_pre,
                                                                   refined)) {
                // Decreasing fixpoint is reached
                this->m_fp_iterator.set_pre(head, std::move(refined));
                break;
            }
            state_pre = std::move(refined);
        }
    }

    this->m_fp_iterator.notify_exit_cycle(head);
}

template < graph G, typename GraphTrait >
const LocationContext* WtoIterator< G, GraphTrait >::get_location_context(
    const NodeRef& node) const {
    return m_loc_mgr.create_location_context(m_frame, -1, node);
}

template < graph G, typename GraphTrait >
void WtoChecker< G, GraphTrait >::visit(const WtoVertex& vertex) {
    auto node = vertex.get_node();
    this->m_fp_iterator.check_pre(node, this->m_fp_iterator.get_pre(node));
    this->m_fp_iterator.check_post(node, this->m_fp_iterator.get_post(node));
}

template < graph G, typename GraphTrait >
void WtoChecker< G, GraphTrait >::visit(const WtoCycle& cycle) {
    auto head = cycle.get_head();
    this->m_fp_iterator.check_pre(head, this->m_fp_iterator.get_pre(head));
    this->m_fp_iterator.check_post(head, this->m_fp_iterator.get_post(head));

    for (auto* component : cycle.components()) {
        component->accept(*this);
    }
}

} // namespace knight::dfa::impl

#ifdef DEBUG_TYPE_BACKUP
#    define DEBUG_TYPE DEBUG_TYPE_BACKUP
#    undef DEBUG_TYPE_BACKUP
#else
#    undef DEBUG_TYPE
#endif
