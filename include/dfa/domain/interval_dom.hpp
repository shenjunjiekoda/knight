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
#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"
#include "dfa/domain/interval.hpp"
#include "dfa/domain/map/separate_numerical_domain.hpp"
#include "dfa/domain/numerical/numerical_base.hpp"
#include "llvm/Support/raw_ostream.h"

namespace knight::dfa {

constexpr std::size_t MaxCycles = 10U;
constexpr std::size_t LargeSystemCstThreshold = 3;
constexpr std::size_t LargeSystemOpThreshold = 27;

template < typename Num, typename NumericalDom >
class IntervalSolver {
  public:
    using Var = Variable< Num >;
    using VarSet = llvm::DenseSet< Var >;

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
    /// Warning: the linear constraint system should outlive the solver
    void add(LinearConstraintSystemRef csts) {
        for (const LinearConstraint& cst :
             csts.get().get_linear_constraints()) {
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
        llvm::outs() << "Refining " << x << " to " << itv << "\n";
        Interval old_itv = numerical.to_interval(x);
        Interval new_itv = old_itv;
        new_itv.meet_with(itv);
        if (new_itv.is_bottom()) {
            return true;
        }
        if (old_itv != new_itv) {
            numerical.meet_value(x, new_itv);
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
            if (!var.equals(pivot)) {
                residual -= Interval(coeff) * numerical.to_interval(var);
                ++this->m_op_cnt;
            }
        }
        return residual;
    }

    /// \return true if the value is bottom, false otherwise
    bool propagate(const LinearConstraint& cst, NumericalDom& numerical) {
        for (const auto& [pivot, coeff] : cst.get_variable_terms()) {
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
                    numerical.meet_value(pivot, new_itv);
                    this->m_refined_vars.insert(pivot);
                }
                ++this->m_op_cnt;
            }
        }
        return false;
    }

    void build_trigger_table() {
        for (LinearConstraintRef cst : this->m_csts) {
            for (const auto& [var, _] : cst.get().get_variable_terms()) {
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

        return false;
    }

    bool solve_small_system(NumericalDom& inv) {
        std::size_t cycle = 0;
        do {
            ++cycle;
            VarSet().swap(m_refined_vars);
            for (const LinearConstraint& cst : this->m_csts) {
                if (this->propagate(cst, inv)) {
                    return true;
                }
            }
        } while (!this->m_refined_vars.empty() && cycle <= MaxCycles);

        return false;
    }

}; // class IntervalSolver < Num, NumericalDom >

template < typename Num, DomainKind Kind, DomainKind SepKind >
class IntervalDom
    : public NumericalDom< IntervalDom< Num, Kind, SepKind >, Num > {
  public:
    using Base = NumericalDom< IntervalDom< Num, Kind, SepKind >, Num >;
    using IntervalDomT = IntervalDom< Num, Kind, SepKind >;
    using Var = Variable< Num >;
    using Interval = Interval< Num >;
    using LinearExpr = LinearExpr< Num >;
    using LinearConstraint = LinearConstraint< Num >;
    using LinearConstraintSystem = LinearConstraintSystem< Num >;
    using SeparateNumericalDom = SeparateNumericalDom< Num, Interval, SepKind >;
    using Solver = IntervalSolver< Num, IntervalDomT >;
    using Map = typename SeparateNumericalDom::Map;

  private:
    SeparateNumericalDom m_sep_dom;

  public:
    IntervalDom(bool is_bottom, Map table = {})
        : m_sep_dom(SeparateNumericalDom(is_bottom, table)) {}

  public:
    [[nodiscard]] static DomainKind get_kind() { return Kind; }
    [[nodiscard]] static SharedVal default_val() {
        return std::static_pointer_cast< AbsDomBase >(
            std::make_shared< IntervalDom >(false, Map{}));
    }
    [[nodiscard]] static SharedVal bottom_val() {
        return std::static_pointer_cast< AbsDomBase >(
            std::make_shared< IntervalDom >(true, Map{}));
    }

    [[nodiscard]] AbsDomBase* clone() const override {
        return new IntervalDom(m_sep_dom.is_bottom(), m_sep_dom.clone_table());
    }

    void normalize() override { m_sep_dom.normalize(); }

    [[nodiscard]] bool is_bottom() const override {
        return m_sep_dom.is_bottom();
    }

    [[nodiscard]] bool is_top() const override { return m_sep_dom.is_top(); }

    void set_to_bottom() override { m_sep_dom.set_to_bottom(); }

    void set_to_top() override { m_sep_dom.set_to_top(); }

    void forget(const Var& x) override { m_sep_dom.forget(x); }

    void join_with(const IntervalDomT& other) {
        m_sep_dom.join_with(other.m_sep_dom);
    }

    void join_with_at_loop_head(const IntervalDomT& other) {
        m_sep_dom.join_with_at_loop_head(other.m_sep_dom);
    }

    void join_consecutive_iter_with(const IntervalDomT& other) {
        m_sep_dom.join_consecutive_iter_with(other.m_sep_dom);
    }

    void widen_with(const IntervalDomT& other) {
        m_sep_dom.widen_with(other.m_sep_dom);
    }

    void meet_with(const IntervalDomT& other) {
        m_sep_dom.meet_with(other.m_sep_dom);
    }

    void meet_value(const Var& x, const Interval& itv) {
        m_sep_dom.meet_value(x, itv);
    }

    void narrow_with(const IntervalDomT& other) {
        m_sep_dom.narrow_with(other.m_sep_dom);
    }

    bool leq(const IntervalDomT& other) const {
        return m_sep_dom.leq(other.m_sep_dom);
    }

    bool equals(const IntervalDomT& other) const {
        return m_sep_dom.equals(other.m_sep_dom);
    }

    Interval get_value(const Var& key) const {
        return m_sep_dom.get_value(key);
    }

    void set_value(const Var& key, const Interval& value) {
        return m_sep_dom.set_value(key, value);
    }

    void dump(llvm::raw_ostream& os) const override { m_sep_dom.dump(os); }

  public:
    void widen_with_threshold(const IntervalDomT& other, Num threshold) {
        m_sep_dom.widen_with_threshold(other.m_sep_dom, threshold);
    }

    void narrow_with_threshold(const IntervalDomT& other, Num threshold) {
        m_sep_dom.narrow_with_threshold(other.m_sep_dom, threshold);
    }

    void assign_num(const Var& x, const Num& n) override {
        m_sep_dom.assign_num(x, n);
    }

    void assign_var(const Var& x, const Var& y) override {
        m_sep_dom.assign_var(x, y);
    }

    void assign_linear_expr(const Var& x, const LinearExpr& e) override {
        m_sep_dom.assign_linear_expr(x, e);
    }

    void assign_unary(clang::UnaryOperatorKind op,
                      const Var& x,
                      const Var& y) override {
        m_sep_dom.assign_unary(op, x, y);
    }

    void assign_binary_var_var(clang::BinaryOperatorKind op,
                               const Var& x,
                               const Var& y,
                               const Var& z) override {
        knight_assert(!clang::BinaryOperator::isAssignmentOp(op));
        if (clang::BinaryOperator::isComparisonOp(op)) {
            IntervalDom dom_pos = *this;
            IntervalDom dom_neg = *this;
            LinearConstraint cstr = Base::construct_constraint(op, y, z);
            dom_pos.apply_linear_constraint(cstr);
            dom_neg.apply_linear_constraint(cstr.negate());
            if (dom_pos.is_bottom() && !dom_neg.is_bottom()) {
                this->set_value(x, Interval::true_val());
            } else if (!dom_pos.is_bottom() && dom_neg.is_bottom()) {
                this->set_value(x, Interval::false_val());
            } else {
                this->set_value(x, Interval::unknown_bool());
            }
        } else {
            m_sep_dom.assign_binary_var_var_for_non_assign_rel_op(op, x, y, z);
        }
    }

    void assign_binary_var_num(clang::BinaryOperatorKind op,
                               const Var& x,
                               const Var& y,
                               const Num& z) override {
        knight_assert(!clang::BinaryOperator::isAssignmentOp(op));
        if (clang::BinaryOperator::isComparisonOp(op)) {
            IntervalDom dom_pos = *this;
            IntervalDom dom_neg = *this;
            LinearConstraint cstr = Base::construct_constraint(op, y, z);
            dom_pos.apply_linear_constraint(cstr);
            dom_neg.apply_linear_constraint(cstr.negate());
            if (dom_pos.is_bottom() && !dom_neg.is_bottom()) {
                this->set_value(x, Interval::true_val());
            } else if (!dom_pos.is_bottom() && dom_neg.is_bottom()) {
                this->set_value(x, Interval::false_val());
            } else {
                this->set_value(x, Interval::unknown_bool());
            }
        } else {
            m_sep_dom.assign_binary_var_num_for_non_assign_rel_op(op, x, y, z);
        }
    }

    void assign_cast(clang::QualType dst_type,
                     unsigned dst_bit_width,
                     const Var& x,
                     const Var& y) override {
        m_sep_dom.assign_cast(dst_type, dst_bit_width, x, y);
    }

    Interval to_interval(const Var& x) const override {
        return m_sep_dom.get_value(x);
    }

    void apply_linear_constraint(const LinearConstraint& cst) override {
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
                                           const Interval& itv) const {
        LinearConstraintSystem csts;

        if (itv.is_bottom()) {
            csts.add_linear_constraint(LinearConstraint::contradiction());
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

    LinearConstraintSystem within_interval(const Var& x,
                                           const Interval& itv) const {
        return within_interval(LinearExpr(x), itv);
    }

    [[nodiscard]] LinearConstraintSystem to_linear_constraint_system()
        const override {
        if (m_sep_dom.is_bottom()) {
            return LinearConstraintSystem(LinearConstraint::contradiction());
        }

        LinearConstraintSystem csts;
        for (auto& [var, itv] : this->m_sep_dom.get_table()) {
            csts.merge_linear_constraint_system(within_interval(var, itv));
        }

        return csts;
    }

}; // class IntervalDom

using ZIntervalDom = IntervalDom< ZNum,
                                  DomainKind::ZIntervalDomain,
                                  DomainKind::ZIntervalSeparate >;
using QIntervalDom = IntervalDom< QNum,
                                  DomainKind::QIntervalDomain,
                                  DomainKind::QIntervalSeparate >;

} // namespace knight::dfa