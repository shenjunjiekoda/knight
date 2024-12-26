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

#include "analyzer/core/constraint/linear.hpp"

#include <llvm/ADT/FoldingSet.h>

namespace knight::analyzer {

class ConstraintSystem : public llvm::FoldingSetNode {
  public:
    using NonLinearConstraintSet = std::unordered_set< SExprRef >;

  private:
    ZLinearConstraintSystem m_zlinear_constraint_system;
    NonLinearConstraintSet m_non_linear_constraint_set;

  public:
    ConstraintSystem() = default;

    // NOLINTNEXTLINE
    void Profile(llvm::FoldingSetNodeID& id) const {
        m_zlinear_constraint_system.Profile(id);
        for (const auto& constraint : m_non_linear_constraint_set) {
            id.AddPointer(constraint);
        }
    }

  public:
    [[nodiscard]] const ZLinearConstraintSystem& get_zlinear_constraint_system()
        const {
        return m_zlinear_constraint_system;
    }

    [[nodiscard]] const NonLinearConstraintSet& get_non_linear_constraint_set()
        const {
        return m_non_linear_constraint_set;
    }

  public:
    void add_zlinear_constraint(const ZLinearConstraint& constraint) {
        m_zlinear_constraint_system.add_linear_constraint(constraint);
    }

    void merge(const ConstraintSystem& system) {
        merge_zlinear_constraint_system(system.get_zlinear_constraint_system());
        merge_non_linear_constraint_set(system.get_non_linear_constraint_set());
    }

    void retain(const ConstraintSystem& system) {
        retain_zlinear_constraint_system(
            system.get_zlinear_constraint_system());
        retain_non_linear_constraint_set(
            system.get_non_linear_constraint_set());
    }

    void merge_zlinear_constraint_system(
        const ZLinearConstraintSystem& system) {
        m_zlinear_constraint_system.merge_linear_constraint_system(system);
    }

    void retain_zlinear_constraint_system(
        const ZLinearConstraintSystem& system) {
        m_zlinear_constraint_system.retain_common_linear_constraint_system(
            system);
    }

    void add_non_linear_constraint(const SExprRef& constraint) {
        m_non_linear_constraint_set.insert(constraint);
    }

    void merge_non_linear_constraint_set(const NonLinearConstraintSet& set) {
        m_non_linear_constraint_set.insert(set.begin(), set.end());
    }

    void retain_non_linear_constraint_set(const NonLinearConstraintSet& set) {
        auto it = m_non_linear_constraint_set.begin();
        while (it != m_non_linear_constraint_set.end()) {
            if (set.find(*it) == set.end()) {
                it = m_non_linear_constraint_set.erase(it);
            } else {
                ++it;
            }
        }
    }

    void dump(llvm::raw_ostream& os) const {
        os << "Constraint System:{";
        if (!m_zlinear_constraint_system.is_empty()) {
            os << "\nZ:\n";
            m_zlinear_constraint_system.dump(os);
            os << "\n";
        }

        if (!m_non_linear_constraint_set.empty()) {
            os << "N:\n";
            for (const auto& constraint : m_non_linear_constraint_set) {
                os << constraint << "\n";
            }
        }
        os << "}\n";
    }

    bool operator==(const ConstraintSystem& other) const {
        return m_zlinear_constraint_system.equals(
                   other.m_zlinear_constraint_system) &&
               m_non_linear_constraint_set == other.m_non_linear_constraint_set;
    }

}; // class ConstraintSystem

} // namespace knight::analyzer