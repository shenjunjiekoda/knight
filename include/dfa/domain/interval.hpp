//===- interval.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the interval domain
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/domain/bound.hpp"
#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"
#include "dfa/domain/map/map_domain.hpp"
#include "dfa/region/region.hpp"
#include "util/assert.hpp"

#include <clang/AST/Decl.h>

#include <climits>

namespace knight::dfa {

template < typename Num >
class Interval : public AbsDom< Interval< Num > > {
  public:
    using Bound = Bound< Num >;

    struct Bottom {};
    struct Top {};

  private:
    /// \brief lower bound of the interval
    Bound m_lb;

    /// \brief upper bound of the interval
    Bound m_ub;

  public:
    /// \brief specify the domain kind
    [[nodiscard]] static DomainKind get_kind() { return DomainKind::Interval; }

    Interval(Bound lb, Bound ub) : m_lb(lb), m_ub(ub) {
        knight_assert(m_lb.is_finite() || m_ub.is_finite() || m_lb != m_ub);
        if (this->m_lb > this->m_ub) {
            this->m_lb = 1;
            this->m_ub = 0;
        }
    }

    explicit Interval(Num n) : Interval(Bound(n)) {}
    explicit Interval(Bound n) : Interval(n, n) {}

    Interval(const Interval&) noexcept = default;
    Interval(Interval&&) noexcept = default;
    Interval& operator=(const Interval&) noexcept = default;
    Interval& operator=(Interval&&) noexcept = default;

    ~Interval() = default;

  private:
    explicit Interval(Top) // NOLINT(readability-named-parameter)
        : Interval(Bound::ninf(), Bound::pinf()) {}

    explicit Interval(Bottom) // NOLINT(readability-named-parameter)
        : Interval(Bound::pinf(), Bound::ninf()) {}

  public:
    [[nodiscard]] static Interval top() { return Interval(Top{}); }
    [[nodiscard]] static Interval bottom() { return Interval(Bottom{}); }

    [[nodiscard]] static SharedVal default_val() {
        return std::make_shared< Interval >(Top{});
    }

    [[nodiscard]] static SharedVal bottom_val() {
        return std::make_shared< Interval >(Bottom{});
    }

    [[nodiscard]] AbsDomBase* clone() const override {
        if (is_bottom()) {
            return new Interval(Bottom{});
        }
        return new Interval(m_lb, m_ub);
    }

    [[nodiscard]] const Bound& get_lb() const { return m_lb; }
    [[nodiscard]] const Bound& get_ub() const { return m_ub; }

    [[nodiscard]] bool is_bottom() const override { return m_lb > m_ub; }
    [[nodiscard]] bool is_top() const override {
        return m_lb.is_ninf() && m_ub.is_pinf();
    }

    void set_to_bottom() override {
        m_lb = 1;
        m_ub = 0;
    }

    void set_to_top() override {
        m_lb = Bound::ninf();
        m_ub = Bound::pinf();
    }

    void normalize() override {}

    [[nodiscard]] bool leq(const Interval& other) const {
        if (is_bottom()) {
            return true;
        }
        if (other.is_bottom()) {
            return false;
        }
        return m_lb >= other.m_lb && m_ub <= other.m_ub;
    }

  public:
    void join_with(const Interval& other) {
        if (is_bottom()) {
            *this = other;
            return;
        }
        if (other.is_bottom()) {
            return;
        }
        m_lb = min(m_lb, other.m_lb);
        m_ub = max(m_ub, other.m_ub);
    }

    void widen_with(const Interval& other) {
        if (is_bottom()) {
            *this = other;
            return;
        }
        if (other.is_bottom()) {
            return;
        }
        m_lb = other.m_lb < m_lb ? Bound::ninf() : m_lb;
        m_ub = other.m_ub > m_ub ? Bound::pinf() : m_ub;
    }

    void widen_with_threshold(const Interval& other, const Num& threshold) {
        if (is_bottom()) {
            *this = other;
            return;
        }
        if (other.is_bottom()) {
            return;
        }
        Bound thr = Bound(threshold);
        if (m_lb > other.m_lb) {
            m_lb = other.m_lb >= thr ? thr : Bound::ninf();
        }
        if (m_ub < other.m_ub) {
            m_ub = other.m_ub <= thr ? thr : Bound::pinf();
        }
    }

    void meet_with(const Interval& other) {
        if (is_bottom()) {
            return;
        }
        if (other.is_bottom()) {
            set_to_bottom();
            return;
        }
        m_lb = max(m_lb, other.m_lb);
        m_ub = min(m_ub, other.m_ub);
    }

    void narrow_with(const Interval& other) {
        if (is_bottom()) {
            return;
        }
        if (other.is_bottom()) {
            set_to_bottom();
            return;
        }
        m_lb = m_lb.is_inf() ? other.m_lb : m_lb;
        m_ub = m_ub.is_inf() ? other.m_ub : m_ub;
    }

    void narrow_with_threshold(const Interval& other, const Num& threshold) {
        if (is_bottom()) {
            return;
        }
        if (other.is_bottom()) {
            set_to_bottom();
            return;
        }
        Bound thr = Bound(threshold);
        m_lb = m_lb.is_inf() ? other.m_lb : (m_lb == thr ? other.m_lb : m_lb);
        m_ub = m_ub.is_inf() ? other.m_ub : (m_ub == thr ? other.m_ub : m_ub);
    }

    [[nodiscard]] bool equals(const Interval& other) const {
        if (is_bottom() && other.is_bottom()) {
            return true;
        }
        return m_lb == other.m_lb && m_ub == other.m_ub;
    }

    void dump(llvm::raw_ostream& os) const override {
        if (is_bottom()) {
            os << "_|_";
        } else {
            if (m_lb == m_ub) {
                os << m_lb;
            } else {
                os << "[" << m_lb << ", " << m_ub << "]";
            }
        }
    }
}; // class Interval

} // namespace knight::dfa
