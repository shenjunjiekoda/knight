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

#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

template < typename Num, typename NumericalDom >
class IntervalSolver;

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
    explicit IntervalDom(bool is_bottom, Map table = {})
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
                               const Var& z) override;

    void assign_binary_var_num(clang::BinaryOperatorKind op,
                               const Var& x,
                               const Var& y,
                               const Num& z) override;

    void assign_cast(clang::QualType dst_type,
                     unsigned dst_bit_width,
                     const Var& x,
                     const Var& y) override {
        m_sep_dom.assign_cast(dst_type, dst_bit_width, x, y);
    }

    Interval to_interval(const Var& x) const override {
        return m_sep_dom.get_value(x);
    }

    void apply_linear_constraint(const LinearConstraint& cst) override;

    void merge_with_linear_constraint_system(
        const LinearConstraintSystem& csts) override;

    LinearConstraintSystem within_interval(const LinearExpr& expr,
                                           const Interval& itv) const;

    LinearConstraintSystem within_interval(const Var& x,
                                           const Interval& itv) const {
        return within_interval(LinearExpr(x), itv);
    }

    [[nodiscard]] LinearConstraintSystem to_linear_constraint_system()
        const override;

}; // class IntervalDom

using ZIntervalDom = IntervalDom< ZNum,
                                  DomainKind::ZIntervalDomain,
                                  DomainKind::ZIntervalSeparate >;
using QIntervalDom = IntervalDom< QNum,
                                  DomainKind::QIntervalDomain,
                                  DomainKind::QIntervalSeparate >;

template < typename Num, DomainKind Kind, DomainKind SepKind >
inline llvm::raw_ostream& operator<<(
    llvm::raw_ostream& os, const IntervalDom< Num, Kind, SepKind >& dom) {
    dom.dump(os);
    return os;
}

} // namespace knight::dfa

#include "interval_dom.tpp"