//===- interval_dom.hpp -----------------------------------------------===//
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

#include "dfa/domain/bound.hpp"
#include "dfa/domain/domains.hpp"
#include "dfa/domain/interval.hpp"
#include "dfa/domain/map/separate_numerical_domain.hpp"
#include "dfa/domain/numerical/numerical_base.hpp"

namespace knight::dfa {

constexpr std::size_t MaxCycles = 10U;
constexpr std::size_t LargeSystemCstThreshold = 3;
constexpr std::size_t LargeSystemOpThreshold = 27;

template < typename Num, typename NumericalDom >
class IntervalSolver {
  public:
    using Var = Variable< Num >;
    using VarSet = std::unordered_set< Var >;

    using Bound = Bound< Num >;
    using Interval = Interval< Num >;

    using LinearExpr = LinearExpr< Num >;
    using LinearConstraint = LinearConstraint< Num >;
    using LinearConstraintSystem = LinearConstraintSystem< Num >;
    using LinearConstraintRef =
        std::reference_wrapper< const LinearConstraint >;
    using LinearConstraintSystemRef =
        std::reference_wrapper< const LinearConstraintSystem >;

    using LinearConstraintSet = std::vector< LinearConstraintRef >;
    using TriggerTable = std::unordered_map< Var, LinearConstraintSet >;

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
    void add(LinearConstraintRef cst) {
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

    /// \brief Add a system of constraint
    ///
    /// Warning: the linear constrainst system should outlive the solver
    void add(LinearConstraintSystemRef csts) {
        for (const LinearConstraint& cst : csts.get()) {
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
            this->m_op_per_cycle += cst.num_terms() * cst.num_terms();
        }
    }

    [[nodiscard]] std::size_t num_constraints() const {
        return this->m_csts.size();
    }

    [[nodiscard]] bool empty() const { return this->m_csts.empty(); }

    void run(NumericalDom& numerical) {
        if (this->m_is_contradiction) {
            numerical.set_to_bottom();
            return;
        }

        if (this->empty()) {
            return;
        }

        this->m_max_op = this->m_op_per_cycle * MaxCycles;

        this->m_is_large_system =
            this->m_csts.size() > LargeSystemCstThreshold ||
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

  private:
    /// \return true if the value is bottom, false otherwise
    bool refine_to_numerical(Var x,
                             const Interval& itv,
                             NumericalDom& numerical) {
        Interval old_itv = numerical.to_interval(x);
        Interval new_itv = old_itv.meet(itv);
        if (new_itv.is_bottom()) {
            return true;
        }
        if (old_itv != new_itv) {
            numerical.refine(x, new_itv);
            this->m_refined_vars.insert(x);
            ++this->m_op_cnt;
        }
        return false;
    }

    [[nodiscard]] Interval compute_residual(const LinearConstraint& cst,
                                            Var pivot,
                                            const NumericalDom& numerical) {
        Interval residual(cst.get_constant_term());
        for (const auto& [var, coeff] : cst.get_variable_terms()) {
            if (var != pivot) {
                residual -= Interval(coeff) * numerical.to_interval(var);
                ++this->m_op_count;
            }
        }
        return residual;
    }

    /// \return true if the value is bottom, false otherwise
    bool propagate(const LinearConstraint& cst, NumericalDom& numerical) {
        for (const auto& [pivot, coeff] : cst) {
            Interval rhs =
                compute_residual(cst, pivot, numerical) / Interval(coeff);
            if (cst.is_equality()) {
                this->refine_to_numerical(pivot, rhs, numerical);
                return false;
            }

            if (cst.is_inequality()) {
                if (coeff > 0) {
                    return this->refine_to_numerical(pivot,
                                                     Interval(Bound::ninf(),
                                                              rhs.get_ub()),
                                                     numerical);
                }
                return this->refine_to_numerical(pivot,
                                                 Interval(rhs.get_lb(),
                                                          Bound::pinf()),
                                                 numerical);
            }
            // cst as disequation
            auto k = rhs.get_singleton_opt();
            if (k) {
                Interval old_itv = numerical.to_interval(pivot);
                Interval new_itv = trim_bound(old_itv, Bound(*k));
                if (new_itv.is_bottom()) {
                    return true;
                }
                if (old_itv != new_itv) {
                    numerical.refine_to_numerical(pivot, new_itv);
                    this->m_refined_vars.insert(pivot);
                }
                ++this->m_op_cnt;
            }
        }
    }

    void build_trigger_table() {
        for (LinearConstraintRef cst : this->m_csts) {
            for (const auto& [var, _] : cst.get()) {
                this->m_trigger_table[var].push_back(cst);
            }
        }
    }

    /// \return true if the value is bottom, false otherwise

    bool solve_large_system(NumericalDom& numerical) {
        this->m_op_cnt = 0;
        VarSet().swap(m_refined_vars);
        for (const LinearConstraint& cst : this->m_csts) {
            if (this->propagate(cst, numerical)) {
                return true;
            }
        }
        do {
            VarSet vars_to_process(this->m_refined_vars);
            VarSet().swap(m_refined_vars);
            for (Var var : vars_to_process) {
                for (const LinearConstraint& cst : this->m_trigger_table[var]) {
                    if (this->propagate(cst, numerical)) {
                        return true;
                    }
                }
            }
        } while (!this->m_refined_vars.empty() &&
                 this->m_op_cnt <= this->m_max_op);
    }

    void solve_small_system(NumericalDom& inv) {
        std::size_t cycle = 0;
        do {
            ++cycle;
            VarSet().swap(m_refined_vars);
            for (const LinearConstraint& cst : this->m_csts) {
                this->propagate(cst, inv);
            }
        } while (!this->m_refined_vars.empty() && cycle <= this->MaxCycles);
    }

}; // class IntervalSolver < Num, NumericalDom >

template < typename Num >
class IntervalDom : public SeparateNumericalDom< Num,
                                                 Interval< Num >,
                                                 DomainKind::IntervalDomain > {
  public:
    using IntervalDomT = IntervalDom< Num >;
    using Var = Variable< Num >;
    using Interval = Interval< Num >;
    using LinearExpr = LinearExpr< Num >;
    using LinearConstraint = LinearConstraint< Num >;
    using LinearConstraintSystem = LinearConstraintSystem< Num >;
    using Solver = IntervalSolver< Num, IntervalDomT >;

  public:
    Interval to_interval(const Var& x) const override {
        return this->get_value(x);
    }

    void add_linear_constraint(const LinearConstraint& cst) override {
        Solver solver;
        solver.add(cst);
        solver.run(*this);
    }

    void merge_with_linear_constraint_system(
        const LinearConstraintSystem& csts) override {
        Solver solver;
        solver.add(csts);
        solver.run(*this);
    }

    LinearConstraintSystem within_interval(const LinearExpr& expr,
                                           const Interval& itv) {
        LinearConstraintSystem csts;

        if (itv.is_bottom()) {
            csts.add(LinearConstraint::contradiction());
        } else {
            auto lb = itv.get_lb().get_num_opt();
            auto ub = itv.get_ub().get_num_opt();

            if (lb && ub && *lb == *ub) {
                csts.add(expr == *ub);
            } else {
                if (lb) {
                    csts.add(expr >= *lb);
                }
                if (ub) {
                    csts.add(expr <= *ub);
                }
            }
        }

        return csts;
    }

    LinearConstraintSystem within_interval(const Var& x, const Interval& itv) {
        return within_interval(LinearExpr(x), itv);
    }

    [[nodiscard]] LinearConstraintSystem to_linear_constraint_system()
        const override {
        if (this->is_bottom()) {
            return LinearConstraintSystemT(LinearConstraint::contradiction());
        }

        LinearConstraintSystem csts;
        for (auto& [var, itv] : this->get_table()) {
            csts.add(within_interval(var, itv));
        }

        return csts;
    }

}; // class IntervalDom

using ZIntervalDom = IntervalDom< llvm::APSInt >;
using QIntervalDom = IntervalDom< llvm::APFloat >;

} // namespace knight::dfa