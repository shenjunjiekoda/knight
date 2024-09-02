//===- interval.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the interval value domain
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/domain/bound.hpp"
#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"
#include "dfa/domain/num/qnum.hpp"
#include "dfa/domain/num/znum.hpp"
#include "util/assert.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
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
    [[nodiscard]] static DomainKind get_kind() { return DomainKind::ZInterval; }
    Interval() : m_lb(Bound::ninf()), m_ub(Bound::pinf()) {}
    Interval(Bound lb, Bound ub) : m_lb(std::move(lb)), m_ub(std::move(ub)) {
        knight_assert(m_lb.is_finite() || m_ub.is_finite() || m_lb != m_ub);
        if (this->m_lb > this->m_ub) {
            this->m_lb = Num(1.);
            this->m_ub = Num(0.);
        }
    }
    Interval(Num lb, Num ub) : Interval(Bound(lb), Bound(ub)) {}

    explicit Interval(Num n) : Interval(Bound(n)) {}
    explicit Interval(Bound n) : Interval(n, n) {}

    Interval(const Interval&) noexcept = default;
    Interval(Interval&&) noexcept = default;
    Interval& operator=(const Interval&) noexcept = default;
    Interval& operator=(Interval&&) noexcept = default;

    ~Interval() override = default;

  public:
    explicit Interval(Top) // NOLINT(readability-named-parameter)
        : Interval(Bound::ninf(), Bound::pinf()) {}

    explicit Interval(Bottom) // NOLINT(readability-named-parameter)
        : Interval(Bound::pinf(), Bound::ninf()) {}

  public:
    [[nodiscard]] static Interval top() { return Interval(Top{}); }
    [[nodiscard]] static Interval bottom() { return Interval(Bottom{}); }
    [[nodiscard]] static Interval unknown_bool() {
        return Interval(Num(0.), Num(1.));
    }
    [[nodiscard]] static Interval true_val() {
        return Interval(Num(1.), Num(1.));
    }
    [[nodiscard]] static Interval false_val() {
        return Interval(Num(0.), Num(0.));
    }
    [[nodiscard]] const Bound& get_lb() const { return m_lb; }
    [[nodiscard]] const Bound& get_ub() const { return m_ub; }
    [[nodiscard]] std::optional< Num > get_singleton_opt() const {
        if (m_lb.is_finite() && m_lb == m_ub) {
            return m_lb.get_num();
        }
        return std::nullopt;
    }

  public:
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

    [[nodiscard]] bool is_bottom() const override { return m_lb > m_ub; }
    [[nodiscard]] bool is_top() const override {
        return m_lb.is_ninf() && m_ub.is_pinf();
    }

    void set_to_bottom() override {
        m_lb = Num(1.);
        m_ub = Num(0.);
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
        Bound one = Bound(Num(1));
        if (m_lb > other.m_lb) {
            if (other.m_lb >= thr) {
                m_lb = thr;
            } else if (other.m_lb == thr - one) {
                m_lb = thr - one;
            } else {
                m_lb = Bound::ninf();
            }
        }
        if (m_ub < other.m_ub) {
            if (other.m_ub <= thr) {
                m_ub = thr;
            } else if (other.m_ub == thr + one) {
                m_ub = thr + one;
            } else {
                m_ub = Bound::pinf();
            }
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

    Interval operator-() const {
        if (this->is_bottom()) {
            return bottom();
        }
        return Interval(-this->m_ub, -this->m_lb);
    }

    void operator+=(const Interval& other) {
        if (this->is_bottom()) {
            return;
        }
        if (other.is_bottom()) {
            this->set_to_bottom();
            return;
        }
        m_lb += other.m_lb;
        m_ub += other.m_ub;
    }

    void operator-=(const Interval& other) {
        if (this->is_bottom()) {
            return;
        }
        if (other.is_bottom()) {
            this->set_to_bottom();
            return;
        }
        m_lb -= other.m_ub;
        m_ub -= other.m_lb;
    }

    /// \brief Return true if the interval contains n
    [[nodiscard]] bool contains(const Num& n) const {
        return !this->is_bottom() && m_lb <= Bound(n) && m_ub >= Bound(n);
    }

    void cast(clang::QualType type, unsigned bit_width) {}

    void dump(llvm::raw_ostream& os) const override {
        if (is_bottom()) {
            os << "⊥";
        } else {
            if (m_lb == m_ub) {
                m_lb.dump(os);
            } else {
                os << "[";
                m_lb.dump(os);
                os << ", ";
                m_ub.dump(os);
                os << "]";
            }
        }
    }

}; // class Interval

template < typename Num >
inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                     const Interval< Num >& dom) {
    dom.dump(os);
    return os;
}

template < typename Num >
inline Interval< Num > operator+(const Interval< Num >& lhs,
                                 const Interval< Num >& rhs) {
    using IntervalT = Interval< Num >;

    if (lhs.is_bottom() || rhs.is_bottom()) {
        return IntervalT::bottom();
    }
    return IntervalT(lhs.get_lb() + rhs.get_lb(), lhs.get_ub() + rhs.get_ub());
}

template < typename Num >
inline Interval< Num > operator-(const Interval< Num >& lhs,
                                 const Interval< Num >& rhs) {
    using IntervalT = Interval< Num >;

    if (lhs.is_bottom() || rhs.is_bottom()) {
        return IntervalT::bottom();
    }
    return IntervalT(lhs.get_lb() - rhs.get_ub(), lhs.get_ub() - rhs.get_lb());
}

template < typename Num >
inline Interval< Num > operator*(const Interval< Num >& lhs,
                                 const Interval< Num >& rhs) {
    using BoundT = Bound< Num >;
    using IntervalT = Interval< Num >;

    if (lhs.is_bottom() || rhs.is_bottom()) {
        return IntervalT::bottom();
    }
    BoundT ll = lhs.get_lb() * rhs.get_lb();
    BoundT lu = lhs.get_lb() * rhs.get_ub();
    BoundT ul = lhs.get_ub() * rhs.get_lb();
    BoundT uu = lhs.get_ub() * rhs.get_ub();
    return IntervalT(min(ll, lu, ul, uu), max(ll, lu, ul, uu));
}

template < typename Num >
inline Interval< Num > operator/(const Interval< Num >& lhs,
                                 const Interval< Num >& rhs) {
    using BoundT = Bound< Num >;
    using IntervalT = Interval< Num >;

    if (lhs.is_bottom() || rhs.is_bottom()) {
        return IntervalT::bottom();
    }
    if (rhs.contains(Num(0))) {
        IntervalT l(rhs.get_lb(), BoundT(Num(-1)));
        IntervalT u(BoundT(Num(1)), rhs.get_ub());
        Interval res(lhs / l);
        res.join_with(lhs / u);
        return res;
    }
    if (lhs.contains(Num(0))) {
        IntervalT l(lhs.get_lb(), BoundT(Num(-1)));
        IntervalT u(BoundT(Num(1)), lhs.get_ub());
        IntervalT res(l / rhs);
        res.join_with(u / rhs);
        res.join_with(IntervalT(Num(0)));
        return res;
    }
    BoundT ll = lhs.get_lb() / rhs.get_lb();
    BoundT lu = lhs.get_lb() / rhs.get_ub();
    BoundT ul = lhs.get_ub() / rhs.get_lb();
    BoundT uu = lhs.get_ub() / rhs.get_ub();
    return IntervalT(min(ll, lu, ul, uu), max(ll, lu, ul, uu));
}

template < typename Num >
inline Interval< Num > operator%(const Interval< Num >& lhs,
                                 const Interval< Num >& rhs) {
    using BoundT = Bound< Num >;
    using IntervalT = Interval< Num >;

    if (lhs.is_bottom() || rhs.is_bottom()) {
        return IntervalT::bottom();
    }
    auto n_opt = lhs.get_singleton_opt();
    auto d_opt = rhs.get_singleton_opt();

    if (d_opt && *d_opt == 0) {
        return IntervalT::bottom();
    }
    if (n_opt && d_opt) {
        return IntervalT(*n_opt % *d_opt);
    }
    BoundT zero(Num(0));
    BoundT n_ub = max(abs(lhs.get_lb()), abs(lhs.get_ub()));
    BoundT d_ub = max(abs(rhs.get_lb()), abs(rhs.get_ub())) - BoundT(Num(1));
    BoundT ub = min(n_ub, d_ub);

    if (lhs.get_lb() < zero) {
        if (lhs.get_ub() > zero) {
            return IntervalT(-ub, ub);
        }
        return IntervalT(-ub, zero);
    }
    return IntervalT(zero, ub);
}

/// \brief Modulo of intervals
template < typename Num >
inline Interval< Num > mod(const Interval< Num >& lhs,
                           const Interval< Num >& rhs) {
    using BoundT = Bound< Num >;
    using IntervalT = Interval< Num >;

    if (lhs.is_bottom() || rhs.is_bottom()) {
        return IntervalT::bottom();
    }
    auto n_opt = lhs.get_singleton_opt();
    auto d_opt = rhs.get_singleton_opt();

    if (d_opt && *d_opt == 0) {
        return IntervalT::bottom();
    }
    if (n_opt && d_opt) {
        return IntervalT(mod(*n_opt, *d_opt));
    }
    if (d_opt && lhs.get_lb().is_finite() && lhs.get_ub().is_finite()) {
        Num lb = lhs.get_lb().get_num();
        Num ub = lhs.get_ub().get_num();

        Num mod_lb = mod(lb, *d_opt);
        Num mod_ub = mod(ub, *d_opt);

        if (mod_ub - mod_lb == ub - lb) {
            return IntervalT(BoundT(mod_lb), BoundT(mod_ub));
        }
        return IntervalT(BoundT(Num(0)), BoundT(abs(*d_opt) - 1));
    }
    BoundT ub = max(abs(rhs.get_lb()), abs(rhs.get_ub())) - BoundT(Num(1));
    return IntervalT(BoundT(Num(0)), ub);
}

template < typename Num >
inline Interval< Num > operator<<(const Interval< Num >& lhs,
                                  const Interval< Num >& rhs) {
    using BoundT = Bound< Num >;
    using IntervalT = Interval< Num >;

    if (lhs.is_bottom() || rhs.is_bottom()) {
        return IntervalT::bottom();
    }

    IntervalT shift = rhs;
    shift.meet_with(IntervalT(BoundT(Num(0)), BoundT::pinf()));

    if (shift.is_bottom()) {
        return IntervalT::bottom();
    }

    IntervalT coeff(BoundT(Num(1) << *shift.get_lb().get_num_opt()),
                    shift.get_ub().is_finite()
                        ? BoundT(Num(1) << *shift.get_ub().get_num_opt())
                        : BoundT::pinf());
    return lhs * coeff;
}

template < typename Num >
inline Interval< Num > operator>>(const Interval< Num >& lhs,
                                  const Interval< Num >& rhs) {
    using BoundT = Bound< Num >;
    using IntervalT = Interval< Num >;

    if (lhs.is_bottom() || rhs.is_bottom()) {
        return IntervalT::bottom();
    }
    IntervalT shift = rhs;
    shift.meet_with(IntervalT(BoundT(Num(0)), BoundT::pinf()));

    if (shift.is_bottom()) {
        return IntervalT::bottom();
    }

    if (lhs.contains(Num(0))) {
        IntervalT l(lhs.get_lb(), BoundT(Num(-1)));
        IntervalT u(BoundT(Num(1)), lhs.get_ub());
        IntervalT res(l >> rhs);
        res.join_with(u >> rhs);
        res.join_with(IntervalT(Num(0)));
        return res;
    }
    BoundT ll = (lhs.get_lb() >> shift.get_lb());
    BoundT lu = (lhs.get_lb() >> shift.get_ub());
    BoundT ul = (lhs.get_ub() >> shift.get_lb());
    BoundT uu = (lhs.get_ub() >> shift.get_ub());
    return IntervalT(min(ll, lu, ul, uu), max(ll, lu, ul, uu));
}

template < typename Num >
inline Interval< Num > operator&(const Interval< Num >& lhs,
                                 const Interval< Num >& rhs) {
    using BoundT = Bound< Num >;
    using IntervalT = Interval< Num >;

    if (lhs.is_bottom() || rhs.is_bottom()) {
        return IntervalT::bottom();
    }
    auto l_opt = lhs.get_singleton_opt();
    auto r_opt = rhs.get_singleton_opt();

    if (l_opt && r_opt) {
        return IntervalT(*l_opt & *r_opt);
    }
    if ((l_opt && *l_opt == 0) || (r_opt && *r_opt == 0)) {
        return IntervalT(Num(0));
    }
    if (l_opt && *l_opt == -1) {
        return rhs;
    }
    if (r_opt && *r_opt == -1) {
        return lhs;
    }
    if (lhs.get_lb() >= BoundT(Num(0)) && rhs.get_lb() >= BoundT(Num(0))) {
        return IntervalT(BoundT(Num(0)), min(lhs.get_ub(), rhs.get_ub()));
    }
    if (lhs.get_lb() >= BoundT(Num(0))) {
        return IntervalT(BoundT(Num(0)), lhs.get_ub());
    }
    if (rhs.get_lb() >= BoundT(Num(0))) {
        return IntervalT(BoundT(Num(0)), rhs.get_ub());
    }
    return IntervalT::top();
}

template < typename Num >
inline Interval< Num > operator|(const Interval< Num >& lhs,
                                 const Interval< Num >& rhs) {
    using BoundT = Bound< Num >;
    using IntervalT = Interval< Num >;

    if (lhs.is_bottom() || rhs.is_bottom()) {
        return IntervalT::bottom();
    }
    auto l = lhs.get_singleton_opt();
    auto r = rhs.get_singleton_opt();

    if (l && r) {
        return IntervalT(*l | *r);
    }
    if ((l && *l == -1) || (r && *r == -1)) {
        return IntervalT(Num(-1));
    }
    if (l && *l == 0) {
        return rhs;
    }
    if (r && *r == 0) {
        return lhs;
    }
    if (lhs.get_lb() >= BoundT(Num(0)) && lhs.get_ub().is_finite() &&
        rhs.get_lb() >= BoundT(Num(0)) && rhs.get_ub().is_finite()) {
        Num m = max(lhs.get_ub().get_num(), rhs.get_ub().get_num());
        Num ub = ((m + 1 <= 1) ? Num(1) : (Num(1) << m)) - 1;
        return IntervalT(BoundT(Num(0)), BoundT(ub));
    }
    return IntervalT::top();
}

template < typename Num >
inline Interval< Num > operator^(const Interval< Num >& lhs,
                                 const Interval< Num >& rhs) {
    using BoundT = Bound< Num >;
    using IntervalT = Interval< Num >;

    if (lhs.is_bottom() || rhs.is_bottom()) {
        return IntervalT::bottom();
    }
    auto l = lhs.get_singleton_opt();
    auto r = rhs.get_singleton_opt();

    if (l && r) {
        return IntervalT(*l ^ *r);
    }
    if (l && *l == 0) {
        return rhs;
    }
    if (r && *r == 0) {
        return lhs;
    }
    if (lhs.get_lb() >= BoundT(Num(0)) && lhs.get_ub().is_finite() &&
        rhs.get_lb() >= BoundT(Num(0)) && rhs.get_ub().is_finite()) {
        Num m = max(lhs.get_ub().get_num(), rhs.get_ub().get_num());
        Num ub = ((m + 1 <= 1) ? Num(1) : (Num(1) << m)) - 1;
        return IntervalT(BoundT(Num(0)), BoundT(ub));
    }
    return IntervalT::top();
}

using ZInterval = Interval< ZNum >;
using QInterval = Interval< QNum >;

inline ZInterval trim_bound(const ZInterval& itv, const ZBound& b) {
    knight_assert(!itv.is_bottom());
    if (itv.get_lb() == b) {
        return {b + ZBound(ZNum(1)), itv.get_ub()};
    }
    if (itv.get_ub() == b) {
        return {itv.get_lb(), b - ZBound(ZNum(1))};
    }
    return itv;
}

inline QInterval trim_bound(const QInterval& itv, const QBound&) { // NOLINT
    return itv;
}

} // namespace knight::dfa
