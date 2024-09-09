//===- discrete_domain.hpp --------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the discrete domain.
//
//===------------------------------------------------------------------===//

#pragma once

#include <optional>

#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"
#include "support/dumpable.hpp"

namespace knight::dfa {
template < typename Key,
           DomainKind domain_kind > // NOLINT(readability-identifier-naming)
class DiscreteDom : public AbsDom< DiscreteDom< Key, domain_kind > > {
  public:
    using Set = std::unordered_set< Key >;

  private:
    struct Top {};
    struct Bottom {};

  private:
    Set m_set;
    bool m_is_top;

  public:
    explicit DiscreteDom(bool is_top = true, Set set = {})
        : m_is_top(is_top), m_set(std::move(set)) {}
    DiscreteDom(std::initializer_list< Key > elements)
        : DiscreteDom(false, elements) {}

    DiscreteDom(const DiscreteDom&) = default;
    DiscreteDom(DiscreteDom&&) = default;
    DiscreteDom& operator=(const DiscreteDom&) = default;
    DiscreteDom& operator=(DiscreteDom&&) = default;
    ~DiscreteDom() override = default;

  public:
    [[nodiscard]] static DiscreteDom top() { return DiscreteDom(true); }

    [[nodiscard]] static DiscreteDom bottom() { return DiscreteDom(false); }

    [[nodiscard]] const Set& get_set() const { return m_set; }

    [[nodiscard]] std::size_t size() const { return m_set.size(); }

    void add(const Key& key) {
        if (this->is_top()) {
            return;
        }
        this->m_set.insert(key);
    }

    void remove(const Key& key) {
        if (this->is_bottom()) {
            return;
        }
        this->m_set.erase(key);
    }

    [[nodiscard]] bool contains(const Key& key) const {
        return this->is_top() || m_set.contains(key);
    }

    [[nodiscard]] DiscreteDom diff(const DiscreteDom& other) const {
        DiscreteDom tmp(*this);
        tmp.diff_with(other);
        return tmp;
    }

    [[nodiscard]] std::optional< Key > get_single_element() const {
        if (this->is_top() || this->size() != 1U) {
            return std::nullopt;
        }
        return *m_set.begin();
    }

  public:
    static DomainKind get_kind() { return domain_kind; }

    static SharedVal default_val() {
        return std::make_shared< DiscreteDom >(true);
    }
    static SharedVal bottom_val() {
        return std::make_shared< DiscreteDom >(false);
    }

    [[nodiscard]] AbsDomBase* clone() const override {
        if constexpr (isa_abs_dom< Key >::value) {
            Set set;
            for (auto& key : m_set) {
                set.insert(*(static_cast< Key* >(key.clone())));
            }
            return new DiscreteDom(m_is_top, set);
        }
        return new DiscreteDom(m_is_top, m_set);
    }

    void normalize() override {}

    [[nodiscard]] bool is_bottom() const override {
        return !m_is_top && m_set.empty();
    }

    [[nodiscard]] bool is_top() const override { return m_is_top; }

    void set_to_bottom() override {
        m_is_top = false;
        Set().swap(m_set);
    }

    void set_to_top() override {
        m_is_top = true;
        Set().swap(m_set);
    }

    void join_with(const DiscreteDom& other) {
        if (this->is_top()) {
            return;
        }
        if (other.is_top()) {
            set_to_top();
            return;
        }
        this->m_set.insert(other.m_set.begin(), other.m_set.end());
    }

    void widen_with(const DiscreteDom& other) { join_with(other); }

    void meet_with(const DiscreteDom& other) {
        if (this->is_bottom() || other.is_top()) {
            return;
        }
        if (other.is_bottom()) {
            set_to_bottom();
            return;
        }
        if (this->is_top()) {
            *this = other;
            return;
        }
        for (auto it = m_set.begin(); it != m_set.end();) {
            if (!other.m_set.contains(*it)) {
                it = this->m_set.erase(it);
            } else {
                it++;
            }
        }
    }

    void narrow_with(const DiscreteDom& other) { meet_with(other); }

    void diff_with(const DiscreteDom& other) {
        if (other.is_top()) {
            set_to_bottom();
            return;
        }
        if (this->is_top()) {
            return;
        }

        for (auto it = m_set.begin(); it != m_set.end();) {
            if (other.m_set.contains(*it)) {
                it = this->m_set.erase(it);
            } else {
                it++;
            }
        }
    }

    [[nodiscard]] bool leq(const DiscreteDom& other) const {
        if (other.is_top()) {
            return true;
        }
        if (this->is_top()) {
            return false;
        }
        return llvm::all_of(this->m_set, [&](const auto& key) {
            return other.m_set.contains(key);
        });
    }

    [[nodiscard]] bool equals(const DiscreteDom& other) const {
        if (this->is_top()) {
            return other.is_top();
        }
        if (other.is_top()) {
            return false;
        }
        if (this->m_set.size() != other.m_set.size()) {
            return false;
        }
        return llvm::all_of(this->m_set, [&](const auto& key) {
            return other.m_set.contains(key);
        });
    }

    void dump(llvm::raw_ostream& os) const override {
        if (is_top()) {
            os << "T";
        } else {
            os << "{";
            bool first = true;
            for (auto& key : m_set) {
                if (!first) {
                    os << ", ";
                }
                DumpableTrait< Key >::dump(os, key);
                first = false;
            }
            os << "}";
        }
    }

}; // class DiscreteDom

} // namespace knight::dfa