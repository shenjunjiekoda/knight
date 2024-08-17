//===- constraint.hpp -------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the constraint.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/constraint/linear.hpp"
#include "llvm/ADT/FoldingSet.h"

namespace knight::dfa {

class ConstraintSystem : public llvm::FoldingSetNode {
  public:
    using NonLinearConstraintSet = std::unordered_set< SExprRef >;

  private:
    ZLinearConstraintSystem m_zlinear_constraint_system;
    QLinearConstraintSystem m_qlinear_constraint_system;
    NonLinearConstraintSet m_non_linear_constraint_set;

  public:
    ConstraintSystem() = default;

    // NOLINTNEXTLINE
    void Profile(llvm::FoldingSetNodeID& id) const {
        m_zlinear_constraint_system.Profile(id);
        m_qlinear_constraint_system.Profile(id);
        for (const auto& constraint : m_non_linear_constraint_set) {
            id.AddPointer(constraint);
        }
    }

  public:
    [[nodiscard]] const ZLinearConstraintSystem& get_zlinear_constraint_system()
        const {
        return m_zlinear_constraint_system;
    }

    [[nodiscard]] const QLinearConstraintSystem& get_qlinear_constraint_system()
        const {
        return m_qlinear_constraint_system;
    }

    [[nodiscard]] const NonLinearConstraintSet& get_non_linear_constraint_set()
        const {
        return m_non_linear_constraint_set;
    }

  public:
    void add_zlinear_constraint(const ZLinearConstraint& constraint) {
        m_zlinear_constraint_system.add_linear_constraint(constraint);
    }

    void merge_zlinear_constraint_system(
        const ZLinearConstraintSystem& system) {
        m_zlinear_constraint_system.merge_linear_constraint_system(system);
    }

    void add_qlinear_constraint(const QLinearConstraint& constraint) {
        m_qlinear_constraint_system.add_linear_constraint(constraint);
    }

    void merge_qlinear_constraint_system(
        const QLinearConstraintSystem& system) {
        m_qlinear_constraint_system.merge_linear_constraint_system(system);
    }

    void add_non_linear_constraint(const SExprRef& constraint) {
        m_non_linear_constraint_set.insert(constraint);
    }

    void merge_non_linear_constraint_set(const NonLinearConstraintSet& set) {
        m_non_linear_constraint_set.insert(set.begin(), set.end());
    }

}; // class ConstraintSystem

} // namespace knight::dfa