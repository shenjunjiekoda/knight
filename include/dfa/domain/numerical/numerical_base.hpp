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
#include "dfa/domain/interval.hpp"

#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>

namespace knight::dfa {

/// Base for all numerical domains. (Linear for currently);
/// Except for the `AbsDom` requirements for `Derived` domain,
/// it should also implement the following *required* methods:
/// - `widen_with_threshold(const Derived&, const Num&)`
/// - `narrow_with_threshold(const Derived&, const Num&)`
/// - `assign_num(Var, const Num &)`
/// - `assign_variable(Var, Var)`
/// - `assign_linear_expr(Var, const LinearExpr&)`
/// - `assign_unary(clang::UnaryOperatorKind, Var, Var)`
/// - `assign_binary_var_var_impl(clang::BinaryOperatorKind,Var,Var,Var)`
/// - `assign_binary_var_num_impl(clang::BinaryOperatorKind,Var,Var,const Num&)`
/// - `assign_cast(clang::QualType, unsigned, Var, Var)`
/// - `add_linear_constraint(const LinearConstraint&)`
/// - `merge_with_linear_constraint_system(const LinearConstraintSystem&)`
/// - `to_linear_constraint_system() const`
template < typename Derived, typename Num >
class NumericalDom : public AbsDom< Derived > {
  public:
    using LinearExpr = LinearExpr< Num >;
    using LinearConstraint = LinearConstraint< Num >;
    using LinearConstraintSystem = LinearConstraintSystem< Num >;
    using Var = Variable< Num >;

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
    virtual void assign_num(const Var& x, const Num& n) = 0;

    /// \brief Assign `x = y`
    virtual void assign_var(const Var& x, const Var& y) = 0;

    /// \brief Assign `x = expr`
    ///
    /// This method is often
    virtual void assign_linear_expr(const Var& x, const LinearExpr& expr) = 0;

    /// \brief Assign `x = op y`
    virtual void assign_unary(clang::UnaryOperatorKind op,
                              const Var& x,
                              const Var& y) = 0;

    /// \brief Apply `x = y op z`
    ///
    /// @{

    /// Split assignment and normal binary operation
    void assign_binary_var_var(clang::BinaryOperatorKind op,
                               const Var& x,
                               const Var& y,
                               const Var& z) {
        bool is_assign_op = clang::BinaryOperator::isAssignmentOp(op);
        bool is_compound_assign_op =
            clang::BinaryOperator::isCompoundAssignmentOp(op);
        if (!is_assign_op) {
            this->apply_binary_var_var_impl(op, x, y, z);
            return;
        }

        if (is_compound_assign_op) {
            op = clang::BinaryOperator::getOpForCompoundAssignment(op);
            this->apply_binary_var_var_impl(op, y, y, z);
        }

        if (is_assign_op) {
            this->assign_var(x, y);
        }
    }

    /// op is not assignment
    virtual void assign_binary_var_var_impl(clang::BinaryOperatorKind op,
                                            const Var& x,
                                            const Var& y,
                                            const Var& z) = 0;

    /// Split assignment and normal binary operation
    void assign_binary_var_num(clang::BinaryOperatorKind op,
                               const Var& x,
                               const Var& y,
                               const Num& z) {
        bool is_assign_op = clang::BinaryOperator::isAssignmentOp(op);
        bool is_compound_assign_op =
            clang::BinaryOperator::isCompoundAssignmentOp(op);
        if (!is_assign_op) {
            this->apply_binary_var_num_impl(op, x, y, z);
            return;
        }

        if (is_compound_assign_op) {
            op = clang::BinaryOperator::getOpForCompoundAssignment(op);
            this->apply_binary_var_num_impl(op, y, y, z);
        }

        if (is_assign_op) {
            this->assign(x, y);
        }
    }

    /// op is not assignment
    virtual void assign_binary_var_num_impl(clang::BinaryOperatorKind op,
                                            const Var& x,
                                            const Var& y,
                                            const Num& z) = 0;

    /// @}

    /// \brief Apply `x = (T)y`
    virtual void assign_cast(clang::QualType dst_type,
                             unsigned dst_bit_width,
                             const Var& x,
                             const Var& y) = 0;

    /// \brief Add a linear constraint
    virtual void add_linear_constraint(const LinearConstraint& cst) = 0;

    /// \brief Add a linear constraint system
    virtual void merge_with_linear_constraint_system(
        const LinearConstraintSystem& csts) = 0;

    /// \brief Get the linear constraint system
    [[nodiscard]] virtual LinearConstraintSystem to_linear_constraint_system()
        const = 0;

    /// \brief Forget a numerical variable
    virtual void forget(const Var& x) = 0;

    [[nodiscard]] virtual Interval< Num > to_interval(const Var& x) const = 0;

}; // class NumericalDom

} // namespace knight::dfa