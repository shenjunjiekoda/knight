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

#include "dfa/constraint/linear.hpp"
#include "dfa/domain/dom_base.hpp"

#include <clang/AST/OperationKinds.h>

namespace knight::dfa {

/// Base for all numerical domains. (Linear for currently);
/// Except for the `AbsDom` requirements for `Derived` domain,
/// it should also implement the following *required* methods:
/// - `widen_with_threshold(const Derived&, const Num&)`
/// - `narrow_with_threshold(const Derived&, const Num&)`
/// - `transfer_assign_constant(VarRef, const Num &)`
/// - `transfer_assign_variable(VarRef, VarRef)`
/// - `transfer_assign_linear_expr(VarRef, const LinearExpr&)`
/// - `apply(clang::BinaryOperatorKind, VarRef)`
template < typename Derived, typename Num >
class NumericalDom : public AbsDom< Derived > {
  public:
    using LinearExpr = LinearExpr< Num >;
    using LinearConstraint = LinearConstraint< Num >;
    using LinearConstraintSystem = LinearConstraintSystem< Num >;
    using VarRef = typename LinearConstraint::VarRef;

  public:
    NumericalDom< Derived, Num >() : AbsDom< Derived >() {}

    /// \brief Widen with a threshold num
    void widen_with_threshold(const NumericalDom& other, const Num& threshold) {
        static_cast< Derived* >(this)->widen_with_threshold(other, threshold);
    }

    /// \brief Narrow with a threshold num
    void narrow_with_threshold(const NumericalDom& other,
                               const Num& threshold) {
        static_cast< Derived* >(this)->narrow_with_threshold(other, threshold);
    }

    /// \brief Assign `x = n`
    virtual void transfer_assign_constant(VarRef x, const Num& n) = 0;

    /// \brief Assign `x = y`
    virtual void transfer_assign_variable(VarRef x, VarRef y) = 0;

    /// \brief Assign `x = expr`
    virtual void transfer_assign_linear_expr(VarRef x,
                                             const LinearExpr& expr) = 0;

    virtual void apply(clang::UnaryOperatorKind op, VarRef x, VarRef y) = 0;

    /// \brief Apply `x = y op z`
    virtual void apply(clang::BinaryOperatorKind op,
                       VarRef x,
                       VarRef y,
                       VarRef z) = 0;

    virtual void apply(clang::BinaryOperatorKind op,
                       VarRef x,
                       VarRef y,
                       const Num& z) = 0;

    virtual void apply(clang::BinaryOperatorKind op,
                       VarRef x,
                       const Num& y,
                       VarRef z) = 0;

    /// \brief Add a linear constraint
    virtual void add_linear_constraint(const LinearConstraint& cst) = 0;

    /// \brief Add a linear constraint system
    virtual void merge_with_linear_constraint_system(
        const LinearConstraintSystem& csts) = 0;

    /// \brief Forget a numerical variable
    virtual void forget(VarRef x) = 0;

}; // class NumericalDom

} // namespace knight::dfa