//===- demo_dom.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines some demo domain for testing purpose.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"
#include "dfa/domain/map/map_domain.hpp"
#include "util/assert.hpp"

#include <clang/AST/Decl.h>

#include <climits>

namespace knight::dfa {

class DemoItvDom : public AbsDom< DemoItvDom > {
  public:
    struct Bottom {};

  private:
    int m_lb, m_ub;
    bool m_is_bottom;

  public:
    DemoItvDom() : DemoItvDom(INT_MIN, INT_MAX) {}
    explicit DemoItvDom(int x) : DemoItvDom(x, x) {}
    DemoItvDom(int l, int u) : m_lb(l), m_ub(u), m_is_bottom(false), AbsDom() {}
    explicit DemoItvDom(Bottom)
        : m_lb(0), m_ub(0), m_is_bottom(true), AbsDom() {}

    /// \brief specify the domain kind
    [[nodiscard]] static DomainKind get_kind() {
        return DomainKind::DemoItvDom;
    }

  public:
    [[nodiscard]] static SharedVal default_val() {
        return std::make_shared< DemoItvDom >();
    }

    [[nodiscard]] static SharedVal bottom_val() {
        return std::make_shared< DemoItvDom >(Bottom{});
    }

    [[nodiscard]] AbsDomBase* clone() const override {
        if (is_bottom()) {
            return new DemoItvDom(Bottom{});
        }
        return new DemoItvDom(m_lb, m_ub);
    }

    void normalize() override {
        if (!is_bottom() && m_lb > m_ub) {
            m_is_bottom = true;
        }
    }

    [[nodiscard]] bool is_bottom() const override { return m_is_bottom; }

    [[nodiscard]] bool is_top() const override {
        return !m_is_bottom && m_lb == INT_MIN && m_ub == INT_MAX;
    }

    void set_to_bottom() override { m_is_bottom = true; }

    void set_to_top() override {
        m_is_bottom = false;
        m_lb = INT_MIN;
        m_ub = INT_MAX;
    }

    void join_with(const DemoItvDom& other) {
        if (is_bottom()) {
            *this = other;
            return;
        }
        if (other.is_bottom()) {
            return;
        }
        m_lb = std::min(m_lb, other.m_lb);
        m_ub = std::max(m_ub, other.m_ub);
    }

    void widen_with(const DemoItvDom& other) {
        if (is_bottom()) {
            *this = other;
            return;
        }
        m_lb = other.m_lb < m_lb ? INT_MIN : m_lb;
        m_ub = other.m_ub > m_ub ? INT_MAX : m_ub;
    }

    void meet_with(const DemoItvDom& other) {
        if (is_bottom() || other.is_bottom()) {
            return;
        }
        m_lb = std::max(m_lb, other.m_lb);
        m_ub = std::min(m_ub, other.m_ub);
    }

    [[nodiscard]] bool leq(const DemoItvDom& other) const {
        if (is_bottom() || other.is_top()) {
            return true;
        }
        return m_lb <= other.m_lb && m_ub >= other.m_ub;
    }

    [[nodiscard]] bool equals(const DemoItvDom& other) const {
        if (is_bottom() && other.is_bottom()) {
            return true;
        }
        return m_lb == other.m_lb && m_ub == other.m_ub;
    }

    void dump(llvm::raw_ostream& os) const override {
        if (is_bottom()) {
            os << "_|_";
        } else {
            os << "[" << m_lb << ", " << m_ub << "]";
        }
    }
}; // class DemoItvDom

using DemoMapDomain =
    MapDom< const clang::Decl*, DemoItvDom, DomainKind::DemoMapDom >;

} // namespace knight::dfa
