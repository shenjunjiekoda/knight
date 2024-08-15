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

#include "dfa/constraint/linear.hpp"
#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"

namespace knight::dfa {

template < typename Num,
           derived_dom SeparateNumericalValue,
           DomainKind DomKind >
class SeparateNumericalDom
    : public AbsDom<
          SeparateNumericalDom< Num, SeparateNumericalValue, DomKind > > {
  public:
    using Var = Variable< Num >;
    using Map = std::unordered_map< Var, SeparateNumericalValue >;
    using LinearExpr = LinearExpr< Num >;

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

    void forget(const Var& key) {
        if (this->is_bottom() || this->is_top()) {
            return;
        }
        this->m_table.erase(key);
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
        if (other.is_bottom() || this->is_top()) {
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

    /// TODO: For numericals

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

    /// \brief Assign `x = n`
    void transfer_assign_constant(Var x, const Num& n) {
        this->set_value(x, Value(n));
    }

    /// \brief Assign `x = y`
    void transfer_assign_variable(Var x, Var y) {
        this->set_value(x, this->get_value(y));
    }

}; // class SeparateNumericalDom

} // namespace knight::dfa