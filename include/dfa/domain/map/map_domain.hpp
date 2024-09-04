//===- map_domain.hpp -------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the map domain.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"
#include "support/dumpable.hpp"

namespace knight::dfa {

template < typename Key, derived_dom SeparateValue, DomainKind domain_kind >
class MapDom : public AbsDom< MapDom< Key, SeparateValue, domain_kind > > {
  public:
    using Map = std::unordered_map< Key, SeparateValue >;

  private:
    Map m_table;
    bool m_is_bottom = false;

  public:
    explicit MapDom(bool is_bottom, Map table = {})
        : m_is_bottom(is_bottom), m_table(std::move(table)) {}

    MapDom(const MapDom&) = default;
    MapDom(MapDom&&) = default;
    MapDom& operator=(const MapDom&) = default;
    MapDom& operator=(MapDom&&) = default;
    ~MapDom() override = default;

  public:
    [[nodiscard]] static MapDom top() { return MapDom(false); }

    [[nodiscard]] static MapDom bottom() { return MapDom(true); }

    [[nodiscard]] const Map& get_table() const { return m_table; }

    void forget(const Key& key) {
        if (this->is_bottom()) {
            return;
        }
        this->m_table.erase(key);
    }

    [[nodiscard]] SeparateValue get_value(const Key& key) const {
        if (this->is_bottom()) {
            return *(static_cast< const SeparateValue* >(
                SeparateValue::bottom_val().get()));
        }

        auto it = m_table.find(key);
        if (it != m_table.end()) {
            return it->second;
        }
        return *(static_cast< const SeparateValue* >(
            SeparateValue::default_val().get()));
    }

    void set_value(const Key& key, const SeparateValue& value) {
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

    void meet_value(const Key& key, const SeparateValue& value) {
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
        if (it != m_table.end()) {
            it->second.meet_with(value);
            return;
        }
        it->second.meet_with(value);
        if (it->second.is_bottom()) {
            this->set_to_bottom();
        }
    }

  public:
    static DomainKind get_kind() { return domain_kind; }

    static SharedVal default_val() { return std::make_shared< MapDom >(false); }
    static SharedVal bottom_val() { return std::make_shared< MapDom >(true); }

    [[nodiscard]] AbsDomBase* clone() const override {
        Map table;
        for (auto& [key, value] : m_table) {
            table[key] = *(static_cast< SeparateValue* >(value.clone()));
        }
        return new MapDom(m_is_bottom, table);
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

    void join_with(const MapDom& other) {
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

    void join_with_at_loop_head(const MapDom& other) {
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

    void join_consecutive_iter_with(const MapDom& other) {
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

    void widen_with(const MapDom& other) {
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

    void meet_with(const MapDom& other) {
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

    void narrow_with(const MapDom& other) {
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

    [[nodiscard]] bool leq(const MapDom& other) const {
        if (this->is_bottom()) {
            return true;
        }
        if (other.is_bottom()) {
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

    [[nodiscard]] bool equals(const MapDom& other) const {
        if (this->is_bottom()) {
            return other.is_bottom();
        }
        if (other.is_bottom()) {
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
            os << "⊥";
        } else {
            os << "{";
            bool first = true;
            for (auto& [key, value] : m_table) {
                if (!first) {
                    os << ", ";
                }
                DumpableTrait< Key >::dump(os, key);
                os << ": ";
                DumpableTrait< SeparateValue >::dump(os, value);
                first = false;
            }
            os << "}";
        }
    }

}; // class MapDom

} // namespace knight::dfa