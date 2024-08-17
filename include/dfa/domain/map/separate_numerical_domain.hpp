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

#include "clang/AST/Expr.h"
#include "dfa/constraint/linear.hpp"
#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"
#include "dfa/domain/interval.hpp"
#include "dfa/domain/numerical/numerical_base.hpp"

#include "util/assert.hpp"

namespace knight::dfa {

template < typename Num,
           derived_dom SeparateNumericalValue,
           DomainKind DomKind >
class SeparateNumericalDom
    : public NumericalDom<
          SeparateNumericalDom< Num, SeparateNumericalValue, DomKind >,
          Num > {
  public:
    using Var = Variable< Num >;
    using Map = std::unordered_map< Var, SeparateNumericalValue >;
    using LinearExpr = LinearExpr< Num >;
    using LinearConstraint = LinearConstraint< Num >;
    using LinearConstraintSystem = LinearConstraintSystem< Num >;

  private:
    Map m_table;
    bool m_is_bottom = false;
    bool m_is_top = false;

  public:
    SeparateNumericalDom(bool is_bottom, bool is_top, Map table = {})
        : m_is_bottom(is_bottom), m_is_top(is_top), m_table(std::move(table)) {}

    SeparateNumericalDom(const SeparateNumericalDom&) = default;
    SeparateNumericalDom(SeparateNumericalDom&&) = default;
    SeparateNumericalDom& operator=(const SeparateNumericalDom&) = default;
    SeparateNumericalDom& operator=(SeparateNumericalDom&&) = default;
    ~SeparateNumericalDom() override = default;

  public:
    static SeparateNumericalDom top() {
        return SeparateNumericalDom(false, true);
    }

    static SeparateNumericalDom bottom() {
        return SeparateNumericalDom(false, true);
    }

    const Map& get_table() const { return m_table; }

    void forget(const Var& x) override {
        if (this->is_bottom() || this->is_top()) {
            return;
        }
        this->m_table.erase(x);
    }

    SeparateNumericalValue get_value(const Var& key) const {
        if (this->is_bottom()) {
            return *(static_cast< SeparateNumericalValue* >(
                SeparateNumericalValue::bottom_val().get()));
        }
        if (this->is_top()) {
            return *(static_cast< SeparateNumericalValue* >(
                SeparateNumericalValue::default_val().get()));
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
            this->m_table.insert_or_assign(key, value);
        }
    }

    void meet_value(const Var& key, const SeparateNumericalValue& value) {
        if (this->is_bottom()) {
            return;
        }
        if (value.is_bottom()) {
            this->set_to_bottom();
        } else if (value.is_top()) {
            return;
        } else {
            auto it = m_table.find(key);
            if (it == m_table.end()) {
                return;
            }
            it->second.meet_with(value);
            if (it->second.is_bottom()) {
                this->set_to_bottom();
            }
        }
    }

  public:
    static DomainKind get_kind() { return DomKind; }

    static SharedVal default_val() {
        return std::make_shared< SeparateNumericalDom >(true, false);
    }
    static SharedVal bottom_val() {
        return std::make_shared< SeparateNumericalDom >(false, true);
    }

    [[nodiscard]] SeparateNumericalValue* clone() const override {
        Map table;
        for (auto& [key, value] : m_table) {
            table[key] =
                *(static_cast< SeparateNumericalValue* >(value.clone().get()));
        }
        return new SeparateNumericalDom(m_is_bottom, m_is_top, table);
    }

    void normalize() override {
        for (auto& [_, value] : m_table) {
            value.normalize();
        }
    }

    [[nodiscard]] bool is_bottom() const override {
        return m_is_bottom && !m_is_top;
    }

    [[nodiscard]] bool is_top() const override {
        return !m_is_bottom && m_is_top;
    }

    void set_to_bottom() override {
        m_is_bottom = true;
        m_is_top = false;
        Map().swap(m_table);
    }

    void set_to_top() override {
        m_is_bottom = false;
        m_is_top = true;
        Map().swap(m_table);
    }

    void join_with(const SeparateNumericalDom& other) {
        if (other.is_bottom()) {
            return;
        }
        if (other.is_top()) {
            this->set_to_top();
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
        if (other.is_top()) {
            this->set_to_top();
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
        if (other.is_top()) {
            this->set_to_top();
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
        if (other.is_top()) {
            this->set_to_top();
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
        if (this->is_bottom() || other.is_top()) {
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
        if (this->is_bottom() || other.is_top()) {
            return;
        }
        if (other.is_bottom() || this->is_top()) {
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
        if (this->is_bottom() || other.is_top()) {
            return true;
        }
        if (other.is_bottom() || this->is_top()) {
            return false;
        }
        if (this->m_table.size() > other.m_table.size()) {
            return false;
        }
        for (auto& [key, value] : this->m_table) {
            auto it = other.m_table.find(key);
            if (it == other.m_table.end()) {
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
        if (this->is_top()) {
            return other.is_top();
        }
        if (other.is_bottom()) {
            return false;
        }
        if (other.is_top()) {
            return false;
        }
        if (this->m_table.size() != other.m_table.size()) {
            return false;
        }
        for (auto& [key, value] : this->m_table) {
            auto it = other.m_table.find(key);
            if (it == other.m_table.end()) {
                return false;
            }
            if (!value.equals(it->second)) {
                return false;
            }
        }
        return true;
    }

    void dump(llvm::raw_ostream& os) const override {
        if (is_bottom()) {
            os << "_|_";
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
                value.dump(os);
                first = false;
            }
            os << "}";
        }
    }

    void widen_with_threshold(const SeparateNumericalDom& other,
                              const Num& threshold) {
        if (other.is_bottom() || this->is_top()) {
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
                    it->second.widening_threshold(sep_num_value, threshold);
                }
            }
        }
    }

    void narrow_with_threshold(const SeparateNumericalDom& other,
                               const Num& threshold) {
        if (this->is_bottom() || other.is_top()) {
            return;
        }
        if (other.is_bottom() || this->is_top()) {
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

    [[nodiscard]] SeparateNumericalDom project(
        const LinearExpr& linear_expr) const {
        if (is_bottom()) {
            return SeparateNumericalDom::bottom();
        }

        SeparateNumericalValue val(linear_expr.get_constant_term());
        for (auto& [var, coeff] : linear_expr.get_terms()) {
            val += coeff * this->get_value(var);
        }
        return val;
    }

    /// \brief Assign `x = n`
    void assign_num(Var x, const Num& n) override {
        this->set_value(x, Value(n));
    }

    /// \brief Assign `x = y`
    void assign_var(Var x, Var y) override {
        this->set_value(x, this->get_value(y));
    }

    /// \brief Assign `x = linear_expr`
    void assign_linear_expr(Var x, const LinearExpr& linear_expr) override {
        this->set_value(x, this->project(linear_expr));
    }

    /// \brief Assign `x = op y`
    void assign_unary(clang::UnaryOperatorKind op, Var x, Var y) override {
        switch (op) {
            using enum clang::UnaryOperatorKind;
            case clang::UO_Minus:
                this->set_value(x, -this->get_value(y));
                break;
            default:
                break;
        }

        knight_unreachable("Unsupported unary operator");
    }

    /// \brief Assign `x = y op z`
    [[nodiscard]] SeparateNumericalValue compute_binary(
        clang::BinaryOperatorKind op,
        SeparateNumericalValue y,
        SeparateNumericalValue z) {
        knight_assert(!clang::BinaryOperator::isAssignmentOp(op));

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

    /// op is not assignment
    void assign_binary_var_var_impl(clang::BinaryOperatorKind op,
                                    Var x,
                                    Var y,
                                    Var z) override {
        this->set_value(x,
                        compute_binary(op,
                                       this->get_value(y),
                                       this->get_value(z)));
    }

    /// op is not assignment
    void assign_binary_var_num_impl(clang::BinaryOperatorKind op,
                                    Var x,
                                    Var y,
                                    Num z) override {
        this->set_value(x, compute_binary(op, this->get_value(y), z));
    }
    /// x = (T)y
    void assign_cast(clang::QualType dst_type,
                     unsigned dst_bit_width,
                     Var x,
                     Var y) override {
        this->set_Value(x, this->get_value(y).cast(dst_type, dst_bit_width));
    }

}; // class SeparateNumericalDom

} // namespace knight::dfa