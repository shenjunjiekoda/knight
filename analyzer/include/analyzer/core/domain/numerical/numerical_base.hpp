//===- numerical_base.hpp ---------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the base for all numerical domains.
//
//===------------------------------------------------------------------===//

#pragma once

#include "analyzer/core/constraint/linear.hpp"
#include "analyzer/core/domain/dom_base.hpp"
#include "analyzer/core/domain/interval.hpp"

#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>

namespace knight::analyzer {

template < typename Num >
struct NumericalDomBase : AbsDomBase {
    using LinearExprT = LinearExpr< Num >;
    using LinearConstraintT = LinearConstraint< Num >;
    using LinearConstraintSystemT = LinearConstraintSystem< Num >;
    using Var = Variable< Num >;

  public:
    explicit NumericalDomBase(DomainKind k) : AbsDomBase(k, true) {}

    ~NumericalDomBase() override = default;

    /// \brief Widen with a threshold num
    virtual void widen_with_threshold(const NumericalDomBase& other,
                                      const Num& threshold) = 0;

    /// \brief Narrow with a threshold num
    virtual void narrow_with_threshold(const NumericalDomBase& other,
                                       const Num& threshold) = 0;

    /// \brief Assign `x = n`
    virtual void assign_num(const Var& x, const Num& n) = 0;

    /// \brief Assign `x = y`
    virtual void assign_var(const Var& x, const Var& y) = 0;

    /// \brief Assign `x = expr`
    ///
    /// This method is often
    virtual void assign_linear_expr(const Var& x, const LinearExprT& expr) = 0;

    /// \brief Apply `x = y op z`
    ///
    /// @{

    /// op is not assignment
    virtual void assign_binary_var_var(clang::BinaryOperatorKind op,
                                       const Var& x,
                                       const Var& y,
                                       const Var& z) = 0;

    /// op is not assignment
    virtual void assign_binary_var_num(clang::BinaryOperatorKind op,
                                       const Var& x,
                                       const Var& y,
                                       const Num& z) = 0;

    /// @}

    /// \brief Apply `x = (T)y`
    virtual void assign_cast(clang::QualType dst_type,
                             unsigned dst_bit_width,
                             const Var& x,
                             const Var& y) = 0;

    LinearConstraintT construct_constraint(clang::BinaryOperatorKind op,
                                           const Var& lhs,
                                           const Num& rhs) {
        knight_assert(clang::BinaryOperator::isComparisonOp(op));
        switch (op) {
            using enum clang::BinaryOperatorKind;
            case BO_EQ:
                return lhs == rhs;
            case BO_NE:
                return lhs != rhs;
            case BO_LT:
                return lhs <= rhs - 1;
            case BO_GT:
                return lhs >= rhs + 1;
            case BO_LE:
                return lhs <= rhs;
            case BO_GE:
                return lhs >= rhs;
            default:
                break;
        }
        knight_unreachable("Unsupported binary operator");
    }

    LinearConstraintT construct_constraint(clang::BinaryOperatorKind op,
                                           const Var& lhs,
                                           const Var& rhs) {
        knight_assert(clang::BinaryOperator::isComparisonOp(op));
        switch (op) {
            using enum clang::BinaryOperatorKind;
            case BO_EQ:
                return lhs == rhs;
            case BO_NE:
                return lhs != rhs;
            case BO_LT:
                return lhs <= rhs - Num(1);
            case BO_GT:
                return lhs >= rhs + Num(1);
            case BO_LE:
                return lhs <= rhs;
            case BO_GE:
                return lhs >= rhs;
            default:
                break;
        }
        knight_unreachable("Unsupported binary operator");
    }

    virtual void assume_predicate_var_num(clang::BinaryOperatorKind op,
                                          const Var& x,
                                          const Num& y) {
        this->apply_linear_constraint(construct_constraint(op, x, y));
    }

    virtual void assume_predicate_var_var(clang::BinaryOperatorKind op,
                                          const Var& x,
                                          const Var& y) {
        this->apply_linear_constraint(construct_constraint(op, x, y));
    }

    /// \brief Add a linear constraint (semantic)
    virtual void apply_linear_constraint(const LinearConstraintT& cst) = 0;

    /// \brief Add a linear constraint system (semantic)
    virtual void merge_with_linear_constraint_system(
        const LinearConstraintSystemT& csts) = 0;

    /// \brief Get the linear constraint system (semantic)
    [[nodiscard]] virtual LinearConstraintSystemT to_linear_constraint_system()
        const = 0;

    /// \brief Forget a numerical variable
    virtual void forget(const Var& x) = 0;

    [[nodiscard]] virtual Interval< Num > to_interval(const Var& x) const = 0;

    static bool classof(const AbsDomBase* dom) { return dom->is_numerical; }

}; // struct NumericalDomBase

/// Base for all numerical domains. (Linear for currently);
/// Except for the `AbsDom` requirements for `Derived` domain,
/// it should also implement the following *required* methods:
/// - `widen_with_threshold(const Derived&, const Num&)`
/// - `narrow_with_threshold(const Derived&, const Num&)`
/// - `assign_num(const Var &, const Num &)`
/// - `assign_var(const Var &, const Var &)`
/// - `assign_linear_expr(const Var &, const LinearExpr&)`
/// - `assign_binary_var_var(clang::BinaryOperatorKind,const Var &,const Var
/// &,const Var &)`
/// - `assign_binary_var_num(clang::BinaryOperatorKind,const Var &,const Var
/// &,const Num&)`
/// - `assign_cast(clang::QualType, unsigned, const Var &, const Var &)`
/// - `apply_linear_constraint(const LinearConstraint&)`
/// - `merge_with_linear_constraint_system(const LinearConstraintSystem&)`
/// - `to_linear_constraint_system() const`
template < typename Derived, typename Num >
class NumericalDom : public NumericalDomBase< Num > {
  public:
    using NumericalDomBaseT = NumericalDomBase< Num >;
    using LinearExprT = LinearExpr< Num >;
    using LinearConstraintT = LinearConstraint< Num >;
    using LinearConstraintSystemT = LinearConstraintSystem< Num >;
    using Var = Variable< Num >;

  public:
    NumericalDom() : NumericalDomBaseT(Derived::get_kind()) {}

    void join_with(const AbsDomBase& other) override {
        static_assert(does_derived_dom_can_join_with< Derived >::value,
                      "derived domain needs to implement `join_with` method");
        static_cast< Derived* >(this)->join_with(
            static_cast< const Derived& >(other));
    }

    void join_with_at_loop_head(const AbsDomBase& other) override {
        if constexpr (does_derived_dom_can_join_with_at_loop_head<
                          Derived >::value) {
            static_cast< Derived* >(this)->join_with_at_loop_head(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::join_with_at_loop_head(other);
        }
    }

    void join_consecutive_iter_with(const AbsDomBase& other) override {
        if constexpr (does_derived_dom_can_join_consecutive_iter_with<
                          Derived >::value) {
            static_cast< Derived* >(this)->join_consecutive_iter_with(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::join_consecutive_iter_with(other);
        }
    }

    void widen_with(const AbsDomBase& other) override {
        if constexpr (does_derived_dom_can_widen_with< Derived >::value) {
            static_cast< Derived* >(this)->widen_with(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::widen_with(other);
        }
    }

    void meet_with(const AbsDomBase& other) override {
        if constexpr (does_derived_dom_can_meet_with< Derived >::value) {
            static_cast< Derived* >(this)->meet_with(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::meet_with(other);
        }
    }

    void narrow_with(const AbsDomBase& other) override {
        if constexpr (does_derived_dom_can_narrow_with< Derived >::value) {
            static_cast< Derived* >(this)->narrow_with(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::narrow_with(other);
        }
    }

    [[nodiscard]] bool leq(const AbsDomBase& other) const override {
        static_assert(does_derived_dom_can_leq< Derived >::value,
                      "derived domain needs to implement `leq` method");
        return static_cast< const Derived& >(*this).leq(
            static_cast< const Derived& >(other));
    }

    [[nodiscard]] bool equals(const AbsDomBase& other) const override {
        if constexpr (does_derived_dom_can_equals< Derived >::value) {
            return static_cast< const Derived& >(*this).equals(
                static_cast< const Derived& >(other));
        } else {
            return AbsDomBase::equals(other);
        }
    }

    /// \brief Widen with a threshold num
    void widen_with_threshold(const NumericalDomBaseT& other,
                              const Num& threshold) override {
        if constexpr (does_derived_numerical_dom_can_widen_with_threshold<
                          Derived,
                          Num >::value) {
            static_cast< Derived* >(this)
                ->widen_with_threshold(static_cast< const Derived& >(other),
                                       threshold);
        } else {
            widen_with(other);
        }
    }

    /// \brief Narrow with a threshold num
    void narrow_with_threshold(const NumericalDomBaseT& other,
                               const Num& threshold) override {
        if constexpr (does_derived_numerical_dom_can_narrow_with_threshold<
                          Derived,
                          Num >::value) {
            static_cast< Derived* >(this)
                ->narrow_with_threshold(static_cast< const Derived& >(other),
                                        threshold);
        } else {
            narrow_with(other);
        }
    }

}; // class NumericalDom

using ZNumericalDomBase = NumericalDomBase< ZNum >;
using ZNumericalValRef = ZNumericalDomBase*;
using SharedZNumericalVal = std::shared_ptr< ZNumericalDomBase >;

} // namespace knight::analyzer