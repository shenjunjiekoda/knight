//===- non_relational_numerical_domain.hpp ----------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the non-relational domain.
//
//===------------------------------------------------------------------===//

#pragma once

#include "analyzer/core/constraint/linear.hpp"
#include "analyzer/core/domain/dom_base.hpp"
#include "analyzer/core/domain/domains.hpp"
#include "analyzer/core/domain/interval.hpp"
#include "analyzer/core/domain/numerical/numerical_base.hpp"
#include "common/util/assert.hpp"

#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/Support/raw_ostream.h>

namespace knight::analyzer {

template < typename Num,
           derived_dom SeparateNumericalValue,
           DomainKind DomKind >
class SeparateNumericalDom
    : public AbsDom<
          SeparateNumericalDom< Num, SeparateNumericalValue, DomKind > > {
  public:
    using Var = Variable< Num >;
    using Map = llvm::DenseMap< Var, SeparateNumericalValue >;
    using LinearExprT = LinearExpr< Num >;
    using LinearConstraintT = LinearConstraint< Num >;
    using LinearConstraintSystemT = LinearConstraintSystem< Num >;

  private:
    Map m_table;
    bool m_is_bottom = false;

  public:
    SeparateNumericalDom(bool is_bottom, Map table = {})
        : m_is_bottom(is_bottom), m_table(std::move(table)) {}

    SeparateNumericalDom(const SeparateNumericalDom&) = default;
    SeparateNumericalDom(SeparateNumericalDom&&) = default;
    SeparateNumericalDom& operator=(const SeparateNumericalDom&) = default;
    SeparateNumericalDom& operator=(SeparateNumericalDom&&) = default;
    ~SeparateNumericalDom() override = default;

  public:
    static SeparateNumericalDom top() { return SeparateNumericalDom(false); }

    static SeparateNumericalDom bottom() { return SeparateNumericalDom(true); }

    const Map& get_table() const { return m_table; }

    void forget(const Var& x) {
        if (this->is_bottom()) {
            return;
        }
        this->m_table.erase(x);
    }

    SeparateNumericalValue get_value(const Var& key) const {
        if (this->is_bottom()) {
            return *(static_cast< SeparateNumericalValue* >(
                SeparateNumericalValue::bottom_val().get()));
        }

        auto it = m_table.find(key);
        if (it != m_table.end()) {
            return it->second;
        }
        return *(static_cast< SeparateNumericalValue* >(
            SeparateNumericalValue::default_val().get()));
    }

    void set_value(const Var& key, const SeparateNumericalValue& value) {
        if (this->is_bottom()) {
            return;
        }
        if (value.is_bottom()) {
            this->set_to_bottom();
        } else if (value.is_top()) {
            this->forget(key);
        } else {
            this->m_table[key] = value;
        }
    }

    void meet_value(const Var& key, const SeparateNumericalValue& value) {
        if (this->is_bottom()) {
            return;
        }

        if (value.is_bottom()) {
            this->set_to_bottom();
        }

        if (value.is_top()) {
            return;
        }

        auto it = m_table.find(key);
        if (it == m_table.end()) {
            set_value(key, value);
            return;
        }
        it->second.meet_with(value);
        if (it->second.is_bottom()) {
            this->set_to_bottom();
        }
    }

  public:
    [[nodiscard]] static DomainKind get_kind() { return DomKind; }

    [[nodiscard]] static SharedVal default_val() {
        return std::make_shared< SeparateNumericalDom >(false);
    }
    [[nodiscard]] static SharedVal bottom_val() {
        return std::make_shared< SeparateNumericalDom >(true);
    }
    [[nodiscard]] Map clone_table() const {
        Map table;
        for (auto& [key, value] : m_table) {
            table[key] =
                *(static_cast< SeparateNumericalValue* >(value.clone()));
        }
        return table;
    }
    [[nodiscard]] AbsDomBase* clone() const override {
        return new SeparateNumericalDom(m_is_bottom, clone_table());
    }

    void normalize() override {
        for (auto& [_, value] : m_table) {
            value.normalize();
        }
    }

    [[nodiscard]] bool is_bottom() const override { return m_is_bottom; }

    [[nodiscard]] bool is_top() const override {
        return !m_is_bottom && m_table.empty();
    }

    void set_to_bottom() override {
        m_is_bottom = true;
        Map().swap(m_table);
    }

    void set_to_top() override {
        m_is_bottom = false;
        Map().swap(m_table);
    }

    void join_with(const SeparateNumericalDom& other) {
        if (other.is_bottom()) {
            return;
        }
        if (this->is_bottom()) {
            *this = other;
            return;
        }

        for (auto& [key, value] : other.m_table) {
            auto it = m_table.find(key);
            if (it == m_table.end()) {
                m_table[key] = value;
            } else {
                it->second.join_with(value);
            }
        }
    }

    void join_with_at_loop_head(const SeparateNumericalDom& other) {
        if (other.is_bottom()) {
            return;
        }

        if (this->is_bottom()) {
            *this = other;
            return;
        }

        for (auto& [key, value] : other.m_table) {
            auto it = m_table.find(key);
            if (it == m_table.end()) {
                m_table[key] = value;
            } else {
                it->second.join_with_at_loop_head(value);
            }
        }
    }

    void join_consecutive_iter_with(const SeparateNumericalDom& other) {
        if (other.is_bottom()) {
            return;
        }

        if (this->is_bottom()) {
            *this = other;
            return;
        }

        for (auto& [key, value] : other.m_table) {
            auto it = m_table.find(key);
            if (it == m_table.end()) {
                m_table[key] = value;
            } else {
                it->second.join_consecutive_iter_with(value);
            }
        }
    }

    void widen_with(const SeparateNumericalDom& other) {
        if (other.is_bottom()) {
            return;
        }

        if (this->is_bottom()) {
            *this = other;
            return;
        }

        for (auto& [key, value] : other.m_table) {
            auto it = m_table.find(key);
            if (it == m_table.end()) {
                m_table[key] = value;
            } else {
                it->second.widen_with(value);
            }
        }
    }

    void meet_with(const SeparateNumericalDom& other) {
        if (this->is_bottom()) {
            return;
        }
        if (other.is_bottom()) {
            *this = other;
            return;
        }
        for (auto& [key, value] : other.m_table) {
            auto it = m_table.find(key);
            if (it == m_table.end()) {
                m_table[key] = value;
            } else {
                it->second.meet_with(value);
                if (it->second.is_bottom()) {
                    this->set_to_bottom();
                    return;
                }
            }
        }
    }

    void refine(Var x, const SeparateNumericalValue& value) {
        if (this->is_bottom() || value.is_top()) {
            return;
        }

        if (value.is_bottom()) {
            this->set_to_bottom();
        }

        auto it = m_table.find(x);
        if (it == m_table.end()) {
            m_table[x] = value;
        } else {
            it->second.meet_with(value);
            if (it->second.is_bottom()) {
                this->set_to_bottom();
                return;
            }
        }
    }

    void narrow_with(const SeparateNumericalDom& other) {
        if (this->is_bottom()) {
            return;
        }
        if (other.is_bottom()) {
            *this = other;
            return;
        }
        for (auto& [key, value] : other.m_table) {
            auto it = m_table.find(key);
            if (it == m_table.end()) {
                m_table[key] = value;
            } else {
                it->second.narrow_with(value);
                if (it->second.is_bottom()) {
                    this->set_to_bottom();
                    return;
                }
            }
        }
    }

    bool leq(const SeparateNumericalDom& other) const {
        if (this->is_bottom()) {
            return true;
        }
        if (other.is_bottom()) {
            return false;
        }
        // llvm::DenseSet< Var > this_keys;
        bool need_to_check_other = this->m_table.size() != other.m_table.size();
        for (auto& [key, value] : this->m_table) {
            auto it = other.m_table.find(key);
            if (it == other.m_table.end()) {
                if (value.is_bottom()) {
                    continue;
                }
                return false;
            }

            if (!value.leq(it->second)) {
                return false;
            }
        }
        return true;
    }

    bool equals(const SeparateNumericalDom& other) const {
        if (this->is_bottom()) {
            return other.is_bottom();
        }
        if (other.is_bottom()) {
            return false;
        }
        llvm::DenseSet< Var > this_keys;
        for (auto& [key, value] : this->m_table) {
            this_keys.insert(key);
            auto it = other.m_table.find(key);
            if (it == other.m_table.end()) {
                if (value.is_bottom()) {
                    continue;
                }
                return false;
            }
            if (!value.equals(it->second)) {
                return false;
            }
        }
        for (auto& [key, value] : other.m_table) {
            if (!this_keys.contains(key) && !value.is_bottom()) {
                return false;
            }
        }
        return true;
    }

    void dump(llvm::raw_ostream& os) const override {
        if (is_bottom()) {
            os << "⊥";
        } else if (is_top()) {
            os << "T";
        } else {
            os << "{";
            bool first = true;
            for (auto& [key, value] : m_table) {
                if (!first) {
                    os << ", ";
                }
                os << key << ": ";
                DumpableTrait< SeparateNumericalValue >::dump(os, value);
                first = false;
            }
            os << "}";
        }
    }

    void widen_with_threshold(const SeparateNumericalDom& other,
                              const Num& threshold) {
        if (other.is_bottom()) {
            return;
        }
        if (this->is_bottom()) {
            *this = other;
        } else {
            for (auto& [key, sep_num_value] : other.m_table) {
                auto it = m_table.find(key);
                if (it == m_table.end()) {
                    m_table[key] = sep_num_value;
                } else {
                    it->second.widen_with_threshold(sep_num_value, threshold);
                }
            }
        }
    }

    void narrow_with_threshold(const SeparateNumericalDom& other,
                               const Num& threshold) {
        if (this->is_bottom()) {
            return;
        }
        if (other.is_bottom()) {
            *this = other;
            return;
        }
        for (auto& [key, value] : other.m_table) {
            auto it = m_table.find(key);
            if (it == m_table.end()) {
                m_table[key] = value;
            } else {
                it->second.narrow_with_threshold(value, threshold);
                if (it->second.is_bottom()) {
                    this->set_to_bottom();
                    return;
                }
            }
        }
    }

    [[nodiscard]] SeparateNumericalValue project(
        const LinearExprT& linear_expr) const {
        if (is_bottom()) {
            return SeparateNumericalValue::bottom();
        }

        SeparateNumericalValue val(linear_expr.get_constant_term());
        for (auto& [var, coeff] : linear_expr.get_variable_terms()) {
            val += SeparateNumericalValue(coeff) * this->get_value(var);
        }
        return val;
    }

    /// \brief Assign `x = n`
    void assign_num(Var x, const Num& n) {
        this->set_value(x, SeparateNumericalValue(n));
    }

    /// \brief Assign `x = y`
    void assign_var(Var x, Var y) { this->set_value(x, this->get_value(y)); }

    /// \brief Assign `x = linear_expr`
    void assign_linear_expr(Var x, const LinearExprT& linear_expr) {
        this->set_value(x, this->project(linear_expr));
    }

    /// \brief Assign `x = y op z`
    ///
    /// \note `op` shall not be assignment or comparison operator.
    [[nodiscard]] SeparateNumericalValue compute_binary(
        clang::BinaryOperatorKind op,
        SeparateNumericalValue y,
        SeparateNumericalValue z) {
        knight_assert(!clang::BinaryOperator::isAssignmentOp(op));
        knight_assert(!clang::BinaryOperator::isComparisonOp(op));

        switch (op) {
            using enum clang::BinaryOperatorKind;
            case BO_Add:
                return y + z;
            case BO_Sub:
                return y - z;
            case BO_Mul:
                return y * z;
            case BO_Div:
                return y / z;
            case BO_Rem:
                return y % z;
            case BO_Shl:
                return y << z;
            case BO_Shr:
                return y >> z;
            case BO_And:
                return y & z;
            case BO_Or:
                return y | z;
            case BO_Xor:
                return y ^ z;
            default:
                break;
        }
        knight_unreachable("Unsupported binary operator");
    }

    /// op is not assignment or comparison operator.
    void assign_binary_var_var_for_non_assign_rel_op(
        clang::BinaryOperatorKind op, Var x, Var y, Var z) {
        this->set_value(x,
                        compute_binary(op,
                                       this->get_value(y),
                                       this->get_value(z)));
    }

    /// op is not assignment or comparison operator.
    void assign_binary_var_num_for_non_assign_rel_op(
        clang::BinaryOperatorKind op, Var x, Var y, const Num& z) {
        this->set_value(x,
                        compute_binary(op,
                                       this->get_value(y),
                                       SeparateNumericalValue(z)));
    }

    /// x = (T)y
    void assign_cast(clang::QualType dst_type,
                     unsigned dst_bit_width,
                     Var x,
                     Var y) {
        auto val = this->get_value(y);
        val.cast(dst_type, dst_bit_width);
        this->set_value(x, val);
    }

}; // class SeparateNumericalDom

} // namespace knight::analyzer