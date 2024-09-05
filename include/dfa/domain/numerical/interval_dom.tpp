//===- interval_dom.tpp -----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the interval separate domain
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/domain/numerical/interval_dom.hpp"
#include "util/log.hpp"

#ifdef DEBUG_TYPE
#    define DEBUG_TYPE_BACKUP DEBUG_TYPE
#    undef DEBUG_TYPE
#endif

#define DEBUG_TYPE "interval-dom"

namespace knight::dfa {

template < typename Num, DomainKind Kind, DomainKind SepKind >
void IntervalDom< Num, Kind, SepKind >::assign_binary_var_var(
    clang::BinaryOperatorKind op, const Var& x, const Var& y, const Var& z) {
    knight_assert(!clang::BinaryOperator::isAssignmentOp(op));
    if (clang::BinaryOperator::isComparisonOp(op)) {
        IntervalDom dom_pos = *this;
        IntervalDom dom_neg = *this;
        LinearConstraintT cstr = Base::construct_constraint(op, y, z);
        dom_pos.apply_linear_constraint(cstr);
        dom_neg.apply_linear_constraint(cstr.negate());
        if (dom_pos.is_bottom() && !dom_neg.is_bottom()) {
            this->set_value(x, IntervalT::true_val());
        } else if (!dom_pos.is_bottom() && dom_neg.is_bottom()) {
            this->set_value(x, IntervalT::false_val());
        } else {
            this->set_value(x, IntervalT::unknown_bool());
        }
    } else {
        m_sep_dom.assign_binary_var_var_for_non_assign_rel_op(op, x, y, z);
    }
}

template < typename Num, DomainKind Kind, DomainKind SepKind >
void IntervalDom< Num, Kind, SepKind >::assign_binary_var_num(
    clang::BinaryOperatorKind op, const Var& x, const Var& y, const Num& z) {
    knight_assert(!clang::BinaryOperator::isAssignmentOp(op));
    if (clang::BinaryOperator::isComparisonOp(op)) {
        IntervalDom dom_pos = *this;
        IntervalDom dom_neg = *this;
        LinearConstraintT cstr = Base::construct_constraint(op, y, z);
        dom_pos.apply_linear_constraint(cstr);
        dom_neg.apply_linear_constraint(cstr.negate());

        knight_log_nl(llvm::outs() << "dom_pos: " << dom_pos << "\n";
                      llvm::outs() << "dom_neg: " << dom_neg << "\n";);

        if (dom_pos.is_bottom() && !dom_neg.is_bottom()) {
            this->set_value(x, IntervalT::false_val());
        } else if (!dom_pos.is_bottom() && dom_neg.is_bottom()) {
            this->set_value(x, IntervalT::true_val());
        } else {
            this->set_value(x, IntervalT::unknown_bool());
        }
    } else {
        m_sep_dom.assign_binary_var_num_for_non_assign_rel_op(op, x, y, z);
    }
}

template < typename Num, DomainKind Kind, DomainKind SepKind >
void IntervalDom< Num, Kind, SepKind >::apply_linear_constraint(
    const LinearConstraintT& cst) {
    Solver solver;
    solver.add(cst);
    solver.run(*this);
}

template < typename Num, DomainKind Kind, DomainKind SepKind >
void IntervalDom< Num, Kind, SepKind >::merge_with_linear_constraint_system(
    const LinearConstraintSystemT& csts) {
    Solver solver;
    solver.add(csts);
    solver.run(*this);
}

template < typename Num, DomainKind Kind, DomainKind SepKind >
IntervalDom< Num, Kind, SepKind >::LinearConstraintSystemT
IntervalDom< Num, Kind, SepKind >::within_interval(const LinearExprT& expr,
                                                   const IntervalT& itv) const {
    LinearConstraintSystemT csts;

    if (itv.is_bottom()) {
        csts.add_linear_constraint(LinearConstraintT::contradiction());
    } else {
        auto lb = itv.get_lb().get_num_opt();
        auto ub = itv.get_ub().get_num_opt();

        if (lb && ub && *lb == *ub) {
            csts.add_linear_constraint(expr == *ub);
        } else {
            if (lb) {
                csts.add_linear_constraint(expr >= *lb);
            }
            if (ub) {
                csts.add_linear_constraint(expr <= *ub);
            }
        }
    }

    return csts;
}

template < typename Num, DomainKind Kind, DomainKind SepKind >
IntervalDom< Num, Kind, SepKind >::LinearConstraintSystemT
IntervalDom< Num, Kind, SepKind >::to_linear_constraint_system() const {
    if (m_sep_dom.is_bottom()) {
        return LinearConstraintSystemT(LinearConstraintT::contradiction());
    }

    LinearConstraintSystemT csts;
    for (auto& [var, itv] : this->m_sep_dom.get_table()) {
        csts.merge_linear_constraint_system(within_interval(var, itv));
    }

    return csts;
}

namespace impl {

constexpr std::size_t MaxCycles = 10U;
constexpr std::size_t LargeSystemCstThreshold = 3;
constexpr std::size_t LargeSystemOpThreshold = 27;

template < typename Num, typename NumericalDom >
class IntervalSolver {
  public:
    using Var = Variable< Num >;
    using VarSet = llvm::DenseSet< Var >;

    using BoundT = Bound< Num >;
    using IntervalT = Interval< Num >;

    using LinearExprT = LinearExpr< Num >;
    using LinearConstraintT = LinearConstraint< Num >;
    using LinearConstraintSystemT = LinearConstraintSystem< Num >;
    using LinearConstraintRef =
        std::reference_wrapper< const LinearConstraintT >;
    using LinearConstraintSystemRef =
        std::reference_wrapper< const LinearConstraintSystemT >;

    using LinearConstraintSet = std::vector< LinearConstraintRef >;
    using TriggerTable = llvm::DenseMap< Var, LinearConstraintSet >;

  private:
    VarSet m_refined_vars;
    LinearConstraintSet m_csts;
    TriggerTable m_trigger_table;

    std::size_t m_op_cnt = 0U;
    std::size_t m_op_per_cycle = 0U;
    std::size_t m_max_op = 0U;

    bool m_is_contradiction = false;
    bool m_is_large_system = false;

  public:
    IntervalSolver() = default;

  public:
    /// \brief Add a constraint
    void add(LinearConstraintRef cst);

    /// \brief Add a constraint system
    void add(LinearConstraintSystemRef csts);

    [[nodiscard]] std::size_t num_constraints() const {
        return this->m_csts.size();
    }

    [[nodiscard]] bool empty() const { return this->m_csts.empty(); }

    /// \brief Solve the constraint system of interval domain
    void run(NumericalDom& numerical);

  private:
    /// \return true if the value is bottom, false otherwise
    [[nodiscard]] bool refine_to_numerical(Var x,
                                           const IntervalT& itv,
                                           NumericalDom& numerical);

    [[nodiscard]] IntervalT compute_residual(const LinearConstraintT& cst,
                                             Var pivot,
                                             const NumericalDom& numerical);

    /// \return true if the value is bottom, false otherwise
    [[nodiscard]] bool propagate(const LinearConstraintT& cst,
                                 NumericalDom& numerical);

    void build_trigger_table();

    /// \return true if the value is bottom, false otherwise
    [[nodiscard]] bool solve_large_system(NumericalDom& numerical);

    /// \return true if the value is bottom, false otherwise
    [[nodiscard]] bool solve_small_system(NumericalDom& inv);

}; // class IntervalSolver < Num, NumericalDom >

template < typename Num, typename NumericalDom >
void IntervalSolver< Num, NumericalDom >::add(LinearConstraintRef cst) {
    if (cst.get().is_contradiction()) {
        this->m_is_contradiction = true;
        return;
    }
    if (cst.get().is_tautology()) {
        return;
    }
    this->m_csts.push_back(cst);
    this->m_op_per_cycle +=
        cst.get().num_variable_terms() * cst.get().num_variable_terms();
}

template < typename Num, typename NumericalDom >
void IntervalSolver< Num, NumericalDom >::add(LinearConstraintSystemRef csts) {
    for (const LinearConstraintT& cst : csts.get().get_linear_constraints()) {
        if (cst.is_contradiction()) {
            this->m_is_contradiction = true;
            return;
        }
        if (cst.is_tautology()) {
            continue;
        }
        this->m_csts.push_back(cst);

        // cost of one reduction step on the constraint in terms
        // of accesses to the interval collection
        this->m_op_per_cycle +=
            cst.num_variable_terms() * cst.num_variable_terms();
    }
}

template < typename Num, typename NumericalDom >
void IntervalSolver< Num, NumericalDom >::run(NumericalDom& numerical) {
    if (this->m_is_contradiction) {
        numerical.set_to_bottom();
        return;
    }

    if (this->empty()) {
        return;
    }

    this->m_max_op = this->m_op_per_cycle * MaxCycles;

    this->m_is_large_system = this->m_csts.size() > LargeSystemCstThreshold ||
                              this->m_op_per_cycle > LargeSystemOpThreshold;

    if (this->m_is_large_system) {
        this->build_trigger_table();
    }

    if (this->m_is_large_system) {
        if (this->solve_large_system(numerical)) {
            numerical.set_to_bottom();
        }
    } else {
        if (this->solve_small_system(numerical)) {
            numerical.set_to_bottom();
        }
    }
}

template < typename Num, typename NumericalDom >
bool IntervalSolver< Num, NumericalDom >::refine_to_numerical(
    Var x, const IntervalT& itv, NumericalDom& numerical) {
    IntervalT old_itv = numerical.to_interval(x);
    IntervalT new_itv = old_itv;
    new_itv.meet_with(itv);

    knight_log_nl(llvm::outs() << "Refining " << x << " to " << itv << "\n";
                  llvm::outs() << "old interval: " << old_itv << "\n";
                  llvm::outs() << "new interval: " << new_itv << "\n";);

    if (new_itv.is_bottom()) {
        return true;
    }
    if (old_itv != new_itv) {
        numerical.meet_value(x, new_itv);

        knight_log_nl(llvm::outs() << "numerical after meet: ";
                      numerical.dump(llvm::outs());
                      llvm::outs() << "\n";);

        this->m_refined_vars.insert(x);
        ++this->m_op_cnt;
    }
    return false;
}

template < typename Num, typename NumericalDom >
IntervalSolver< Num, NumericalDom >::IntervalT
IntervalSolver< Num, NumericalDom >::compute_residual(
    const LinearConstraintT& cst, Var pivot, const NumericalDom& numerical) {
    IntervalT residual(cst.get_constant_term());
    for (const auto& [var, coeff] : cst.get_variable_terms()) {
        if (!var.equals(pivot)) {
            residual -= IntervalT(coeff) * numerical.to_interval(var);
            ++this->m_op_cnt;
        }
    }
    return residual;
}

template < typename Num, typename NumericalDom >
bool IntervalSolver< Num, NumericalDom >::propagate(
    const LinearConstraintT& cst, NumericalDom& numerical) {
    for (const auto& [pivot, coeff] : cst.get_variable_terms()) {
        IntervalT rhs =
            compute_residual(cst, pivot, numerical) / IntervalT(coeff);
        if (cst.is_equality()) {
            return this->refine_to_numerical(pivot, rhs, numerical);
        }

        if (cst.is_inequality()) {
            if (coeff > 0) {
                return this->refine_to_numerical(pivot,
                                                 IntervalT(BoundT::ninf(),
                                                           rhs.get_ub()),
                                                 numerical);
            }
            return this->refine_to_numerical(pivot,
                                             IntervalT(rhs.get_lb(),
                                                       BoundT::pinf()),
                                             numerical);
        }
        // cst as disequation
        auto k = rhs.get_singleton_opt();
        if (k) {
            IntervalT old_itv = numerical.to_interval(pivot);
            IntervalT new_itv = trim_bound(old_itv, BoundT(*k));
            if (new_itv.is_bottom()) {
                return true;
            }
            if (old_itv != new_itv) {
                numerical.meet_value(pivot, new_itv);
                this->m_refined_vars.insert(pivot);
            }
            ++this->m_op_cnt;
        }
    }
    return false;
}

template < typename Num, typename NumericalDom >
void IntervalSolver< Num, NumericalDom >::build_trigger_table() {
    for (LinearConstraintRef cst : this->m_csts) {
        for (const auto& [var, _] : cst.get().get_variable_terms()) {
            this->m_trigger_table[var].push_back(cst);
        }
    }
}

template < typename Num, typename NumericalDom >
bool IntervalSolver< Num, NumericalDom >::solve_large_system(
    NumericalDom& numerical) {
    this->m_op_cnt = 0;
    VarSet().swap(m_refined_vars);
    for (const LinearConstraintT& cst : this->m_csts) {
        if (this->propagate(cst, numerical)) {
            return true;
        }
    }
    do {
        VarSet vars_to_process(this->m_refined_vars);
        VarSet().swap(m_refined_vars);
        for (Var var : vars_to_process) {
            for (const LinearConstraintT& cst : this->m_trigger_table[var]) {
                if (this->propagate(cst, numerical)) {
                    return true;
                }
            }
        }
    } while (!this->m_refined_vars.empty() && this->m_op_cnt <= this->m_max_op);

    return false;
}

template < typename Num, typename NumericalDom >
bool IntervalSolver< Num, NumericalDom >::solve_small_system(
    NumericalDom& inv) {
    std::size_t cycle = 0;
    do {
        ++cycle;
        VarSet().swap(m_refined_vars);
        for (const LinearConstraintT& cst : this->m_csts) {
            if (this->propagate(cst, inv)) {
                return true;
            }
        }
    } while (!this->m_refined_vars.empty() && cycle <= MaxCycles);

    return false;
}

} // namespace impl

} // namespace knight::dfa

#ifdef DEBUG_TYPE_BACKUP
#    define DEBUG_TYPE DEBUG_TYPE_BACKUP
#    undef DEBUG_TYPE_BACKUP
#else
#    undef DEBUG_TYPE
#endif