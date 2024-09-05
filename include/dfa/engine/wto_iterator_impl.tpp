#pragma once

#include "dfa/domain/num/znum.hpp"
#include "dfa/location_context.hpp"
#include "util/log.hpp"
#include "wto_iterator.hpp"

#include <clang/AST/Expr.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#ifdef DEBUG_TYPE
#    define DEBUG_TYPE_BACKUP DEBUG_TYPE
#    undef DEBUG_TYPE
#endif

#define DEBUG_TYPE "wto-iterator"

namespace knight::dfa {

template < graph CFG, typename GraphTrait >
std::optional< ZNum > WtoBasedFixPointIterator< CFG, GraphTrait >::
    get_threshold(NodeRef head) const {
    if (const auto* cond = head->getLastCondition()) {
        if (cond == nullptr) {
            return std::nullopt;
        }
        cond = cond->IgnoreParenImpCasts();

        if (llvm::isa< clang::DeclRefExpr >(cond)) {
            return ZNum(0);
        }
        if (const auto* unary_cond =
                llvm::dyn_cast_or_null< clang::UnaryOperator >(cond)) {
            if (unary_cond->getOpcode() == clang::UO_LNot) {
                return ZNum(0);
            }
        }
        if (const auto* binary_cond =
                llvm::dyn_cast_or_null< clang::BinaryOperator >(cond);
            binary_cond != nullptr && binary_cond->isComparisonOp()) {
            bool lhs_cst = false;
            std::optional< ZNum > cst = std::nullopt;
            clang::Expr::EvalResult res;
            if (binary_cond->getLHS()
                    ->EvaluateAsInt(res,
                                    get_cfg()->get_proc()->getASTContext())) {
                cst = res.Val.getInt().getZExtValue();
                lhs_cst = true;
            } else if (binary_cond->getRHS()
                           ->EvaluateAsInt(res,
                                           get_cfg()
                                               ->get_proc()
                                               ->getASTContext())) {
                cst = res.Val.getInt().getZExtValue();
            }
            if (!cst.has_value()) {
                return std::nullopt;
            }
            ZNum threshold = *cst;
            switch (binary_cond->getOpcode()) {
                case clang::BO_LT: {
                    if (lhs_cst) {
                        threshold++;
                    } else {
                        threshold--;
                    }
                } break;

                case clang::BO_GT: {
                    if (lhs_cst) {
                        threshold--;
                    } else {
                        threshold++;
                    }
                } break;
                default:
                    break;
            }
            return threshold;
        }
    }
    return std::nullopt;
}

namespace impl {

template < graph G, typename GraphTrait = GraphTrait< G > >
class WtoIterator final : public WtoComponentVisitor< G, GraphTrait > {
  public:
    using WtoFPIterator = WtoBasedFixPointIterator< G, GraphTrait >;
    using GraphRef = typename WtoFPIterator::GraphRef;
    using NodeRef = typename WtoFPIterator::NodeRef;

    using WtoT = Wto< G, GraphTrait >;
    using WtoNestingT = WtoNesting< G, GraphTrait >;
    using WtoVertexT = WtoVertex< G, GraphTrait >;
    using WtoCycleT = WtoCycle< G, GraphTrait >;

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
    void visit(const WtoVertexT& vertex) override;

    void visit(const WtoCycleT& cycle) override;

    const LocationContext* get_location_context(const NodeRef& node) const;

}; // class WtoIterator
template < graph G, typename GraphTrait >
class WtoChecker final : public WtoComponentVisitor< G, GraphTrait > {
  public:
    using WtoFPIterator = WtoBasedFixPointIterator< G, GraphTrait >;
    using GraphRef = typename WtoFPIterator::GraphRef;
    using NodeRef = typename WtoFPIterator::NodeRef;

    using WtoT = Wto< G, GraphTrait >;
    using WtoNestingT = WtoNesting< G, GraphTrait >;
    using WtoVertexT = WtoVertex< G, GraphTrait >;
    using WtoCycleT = WtoCycle< G, GraphTrait >;

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
    void visit(const WtoVertexT& vertex) override;

    void visit(const WtoCycleT& cycle) override;

}; // class WtoChecker

template < graph G, typename GraphTrait >
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void WtoIterator< G, GraphTrait >::visit(const WtoVertexT& vertex) {
    auto node = vertex.get_node();
    ProgramStateRef state_pre = this->m_fp_iterator.get_pre(node);

    knight_log(llvm::outs()
               << "wto visit node: " << node->getBlockID() << "\n");

    for (auto it = GraphTrait::pred_begin(node),
              end = GraphTrait::pred_end(node);
         it != end;
         ++it) {
        auto pred = *it;

        auto joined_val =
            state_pre->join(this->m_fp_iterator
                                .transfer_edge(pred,
                                               node,
                                               this->m_fp_iterator.get_post(
                                                   pred)),
                            get_location_context(node));

        knight_log_nl(llvm::outs()
                      << "join pred `" << node->getBlockID() << "` with"
                      << " node `" << pred->getBlockID()
                      << "`\nstate_pre before join: " << *state_pre
                      << "\npre transfer edge state: " << *joined_val);

        state_pre = joined_val;

        knight_log_nl(llvm::outs()
                      << "state_pre after join: " << *state_pre << "\n");
    }

    this->m_fp_iterator.set_pre(node, state_pre);
    this->m_fp_iterator
        .set_post(node,
                  this->m_fp_iterator.transfer_node(node,
                                                    std::move(state_pre)));
}

template < graph G, typename GraphTrait >
void WtoIterator< G, GraphTrait >::visit(const WtoCycleT& cycle) {
    auto head = cycle.get_head();
    ProgramStateRef state_pre = this->m_fp_iterator.get_bottom();
    auto& wto = this->m_fp_iterator.get_wto();
    const auto& nesting = wto.get_nesting(head);
    const auto* loc_ctx = get_location_context(head);

    this->m_fp_iterator.notify_enter_cycle(head);

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
        knight_log(llvm::outs() << "iteration#" << iter_cnt << "\n");

        this->m_fp_iterator.notify_each_cycle_iteration(head, iter_cnt, kind);
        state_pre = state_pre->normalize();
        knight_log(llvm::outs() << "set for head pre: " << *state_pre << "\n");

        this->m_fp_iterator.set_pre(head, state_pre);

        auto state_post = this->m_fp_iterator.transfer_node(head, state_pre);
        knight_log(llvm::outs()
                   << "set for head post: " << *state_post << "\n");

        this->m_fp_iterator.set_post(head, state_post);

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
            knight_log_nl(llvm::outs()
                          << "in increasing stage, head state_pre: "
                          << *state_pre
                          << "\nnew_state_front: " << *new_state_front << "\n");
            ProgramStateRef increased =
                this->m_fp_iterator
                    .enlarge_at_head_when_increasing(head,
                                                     iter_cnt,
                                                     state_pre,
                                                     new_state_front,
                                                     get_location_context(
                                                         head));
            increased = increased->normalize();

            knight_log_nl(llvm::outs()
                          << "head increased state: " << *increased << "\n"
                          << "state_pre: " << *state_pre << "\n");

            if (this->m_fp_iterator.is_increasing_fixpoint_reached(head,
                                                                   iter_cnt,
                                                                   state_pre,
                                                                   increased)) {
                knight_log(llvm::outs() << "increasing fixpoint reached, turn "
                                           "to decreasing stage\n");
                // Increasing fixpoint is reached
                kind = IterationKind::Decreasing;
                iter_cnt = 1U;
            } else {
                knight_log(llvm::outs() << "increasing fixpoint not reached, "
                                           "continue to increase\n");
                state_pre = std::move(increased);
            }
        }

        if (kind == IterationKind::Decreasing) {
            ProgramStateRef refined =
                this->m_fp_iterator
                    .refine_at_loop_head_when_decreasing(head,
                                                         iter_cnt,
                                                         state_pre,
                                                         new_state_front);
            refined = refined->normalize();
            knight_log_nl(llvm::outs()
                          << "head refined state: " << *refined << "\n"
                          << "state_pre: " << *state_pre << "\n");
            if (this->m_fp_iterator.is_decreasing_fixpoint_reached(head,
                                                                   iter_cnt,
                                                                   state_pre,
                                                                   refined)) {
                knight_log(llvm::outs() << "decreasing fixpoint reached\n");
                // Decreasing fixpoint is reached
                this->m_fp_iterator.set_pre(head, std::move(refined));
                break;
            }
            knight_log(llvm::outs() << "decreasing fixpoint not reached, "
                                       "continue to refine\n");
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
void WtoChecker< G, GraphTrait >::visit(const WtoVertexT& vertex) {
    auto node = vertex.get_node();
    this->m_fp_iterator.check_pre(node, this->m_fp_iterator.get_pre(node));
    this->m_fp_iterator.check_post(node, this->m_fp_iterator.get_post(node));
}

template < graph G, typename GraphTrait >
void WtoChecker< G, GraphTrait >::visit(const WtoCycleT& cycle) {
    auto head = cycle.get_head();
    this->m_fp_iterator.check_pre(head, this->m_fp_iterator.get_pre(head));
    this->m_fp_iterator.check_post(head, this->m_fp_iterator.get_post(head));

    for (auto* component : cycle.components()) {
        component->accept(*this);
    }
}

} // namespace impl

} // namespace knight::dfa

#ifdef DEBUG_TYPE_BACKUP
#    define DEBUG_TYPE DEBUG_TYPE_BACKUP
#    undef DEBUG_TYPE_BACKUP
#else
#    undef DEBUG_TYPE
#endif
