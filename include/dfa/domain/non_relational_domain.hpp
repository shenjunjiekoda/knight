//===- non_relational_domain.hpp --------------------------------------------===//
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

#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"

namespace knight::dfa {

template < typename Key, derived_dom Value, DomainKind domain_kind >
class NonRelationalDomain
    : public AbsDom< NonRelationalDomain< Key, Value, domain_kind > > {
  public:
    using Map = std::unordered_map< Key, Value >;

  private:
    Map m_table;
    bool m_is_bottom = false;
    bool m_is_top = false;

  public:
    NonRelationalDomain(bool is_bottom, bool is_top, Map table = {})
        : m_is_bottom(is_bottom), m_is_top(is_top), m_table(std::move(table)) {}

    NonRelationalDomain(const NonRelationalDomain&) = default;
    NonRelationalDomain(NonRelationalDomain&&) = default;
    NonRelationalDomain& operator=(const NonRelationalDomain&) = default;
    NonRelationalDomain& operator=(NonRelationalDomain&&) = default;
    ~NonRelationalDomain() override = default;

  public:
    static NonRelationalDomain top() {
        return NonRelationalDomain(false, true);
    }

    static NonRelationalDomain bottom() {
        return NonRelationalDomain(false, true);
    }

    const Map& get_table() const { return m_table; }

    void forget(const Key& key) {
        if (this->is_bottom() || this->is_top()) {
            return;
        }
        this->m_table.erase(key);
    }

    Value get_value(const Key& key) const {
        if (this->is_bottom()) {
            return *(static_cast< Value* >(Value::bottom_val().get()));
        }
        if (this->is_top()) {
            return *(static_cast< Value* >(Value::default_val().get()));
        }

        auto it = m_table.find(key);
        if (it != m_table.end()) {
            return it->second;
        }
        return *(static_cast< Value* >(Value::default_val().get()));
    }

    void set_value(const Key& key, const Value& value) {
        if (this->is_bottom()) {
            return;
        } else if (value.is_bottom()) {
            this->set_to_bottom();
        } else if (value.is_top()) {
            this->forget(key);
        } else {
            this->m_table.insert_or_assign(key, value);
        }
    }

    void meet_value(const Key& key, const Value& value) {
        if (this->is_bottom()) {
            return;
        } else if (value.is_bottom()) {
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
    static DomainKind get_kind() { return domain_kind; }

    static UniqueVal default_val() {
        return std::make_unique< NonRelationalDomain >(true, false);
    }
    static UniqueVal bottom_val() {
        return std::make_unique< NonRelationalDomain >(false, true);
    }

    UniqueVal clone() const override {
        Map table;
        for (auto& [key, value] : m_table) {
            table[key] = *(static_cast< Value* >(value.clone().get()));
        }
        return std::make_unique< NonRelationalDomain >(m_is_bottom,
                                                       m_is_top,
                                                       table);
    }

    void normalize() override {
        for (auto& [_, value] : m_table) {
            value.normalize();
        }
    }

    bool is_bottom() const override { return m_is_bottom && !m_is_top; }

    bool is_top() const override { return !m_is_bottom && m_is_top; }

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

    void join_with(const NonRelationalDomain& other) {
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

    void join_with_at_loop_head(const NonRelationalDomain& other) {
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

    void join_consecutive_iter_with(const NonRelationalDomain& other) {
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

    void widen_with(const NonRelationalDomain& other) {
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

    void meet_with(const NonRelationalDomain& other) {
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

    void narrow_with(const NonRelationalDomain& other) {
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

    bool leq(const NonRelationalDomain& other) const {
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

    bool equals(const NonRelationalDomain& other) const {
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

}; // class NonRelationalDomain

} // namespace knight::dfa