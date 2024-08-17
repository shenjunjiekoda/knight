//===- bound.hpp ------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the bound
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APSInt.h>

#include <llvm/Support/raw_ostream.h>

#include "util/assert.hpp"

#include <optional>

namespace knight::dfa {

template < typename Num >
class Bound {
  private:
    bool m_is_inf;
    Num m_val;

  private:
    Bound(bool is_inf, Num val) : m_is_inf(is_inf), m_val(val) { normalize(); }

    void normalize() {
        if (!m_is_inf) {
            return;
        }
        if (m_val >= 0) {
            m_val = 1;
        } else {
            m_val = -1;
        }
    }

  public:
    Bound() = delete;

    explicit Bound(Num val) : Bound(false, val) {}
    Bound(const Bound&) noexcept = default;
    Bound(Bound&&) noexcept = default;
    Bound& operator=(const Bound&) noexcept = default;
    Bound& operator=(Bound&&) noexcept = default;

    Bound& operator=(Num n) {
        this->m_is_inf = false;
        this->m_val = n;
        return *this;
    }
    ~Bound() = default;

  public:
    /// \brief get the positive infinity bound
    [[nodiscard]] static Bound pinf() { return Bound(true, 1); }

    /// \brief get the negative infinity bound
    [[nodiscard]] static Bound ninf() { return Bound(true, -1); }

  public:
    [[nodiscard]] bool is_inf() const { return m_is_inf; }
    [[nodiscard]] bool is_finite() const { return !m_is_inf; }
    [[nodiscard]] bool is_pinf() const { return m_is_inf && m_val == 1; }
    [[nodiscard]] bool is_ninf() const { return m_is_inf && m_val == -1; }

    [[nodiscard]] bool is_num(Num n) { return !m_is_inf && m_val == n; }
    [[nodiscard]] bool is_zero() { return is_num(Num(0.)); }
    [[nodiscard]] bool is_one() { return is_num(Num(1.)); }

    [[nodiscard]] std::optional< Num > get_num_opt() const {
        return m_is_inf ? std::nullopt : std::make_optional(m_val);
    }

    [[nodiscard]] Num get_num() const {
        knight_assert_msg(is_finite(), "Bound is inf");
        return m_val;
    }

    /// \brief -b
    Bound operator-() const { return Bound(m_is_inf, -m_val); }

    void operator+=(const Bound& b) {
        if (this->is_inf()) {
            if (b.is_inf() && this->m_val != b.m_val) {
                knight_unreachable("undefined op on pinf + ninf");
            }
            return;
        }
        if (b.is_finite()) {
            this->m_val += b.m_val;
            return;
        }

        *this = b;
    }

    void operator-=(const Bound& b) {
        if (this->is_inf()) {
            if (b.is_inf() && this->m_val == b.m_val) {
                knight_unreachable("undefined op on inf - inf");
            }
            return;
        }
        if (b.is_finite()) {
            this->m_val -= b.m_val;
            return;
        }

        *this = -b;
    }

    void operator*=(const Bound& b) {
        if (this->is_zero()) {
            return;
        }
        if (b.is_zero()) {
            *this = b;
            return;
        }
        m_val *= b.m_val;
        m_is_inf = (m_is_inf || b.m_is_inf);
        normalize();
    }

    bool operator<=(const Bound& b) const {
        if (m_is_inf && !b.m_is_inf) {
            return m_val == -1;
        }
        if (!m_is_inf && b.m_is_inf) {
            return b.m_val == 1;
        }
        return m_val <= b.m_val;
    }

    bool operator>=(const Bound& b) const {
        if (m_is_inf && !b.m_is_inf) {
            return m_val == 1;
        }
        if (!m_is_inf && b.m_is_inf) {
            return b.m_val == -1;
        }
        return m_val >= b.m_val;
    }

    bool operator<(const Bound& b) const { return !(*this >= b); }

    bool operator>(const Bound& b) const { return !(*this <= b); }

    bool operator==(const Bound& b) const {
        return this->m_is_inf == b.m_is_inf && this->m_val == b.m_val;
    }

    bool operator!=(const Bound& b) const { return !(*this == b); }

    void dump(llvm::raw_ostream& os) const {
        if (is_pinf()) {
            os << "+oo";
        } else if (is_ninf()) {
            os << "-oo";
        } else {
            os << m_val;
        }
    }

    template < typename T >
    friend Bound< T > operator+(const Bound< T >& lhs, const Bound< T >& rhs);

    template < typename T >
    friend Bound< T > operator-(const Bound< T >& lhs, const Bound< T >& rhs);

    template < typename T >
    friend Bound< T > operator*(const Bound< T >& lhs, const Bound< T >& rhs);

    template < typename T >
    friend Bound< T > operator/(const Bound< T >& lhs, const Bound< T >& rhs);

    template < typename T >
    friend const Bound< T >& min(const Bound< T >& lhs, const Bound< T >& rhs);

    template < typename T >
    friend const Bound< T >& min(const Bound< T >& a,
                                 const Bound< T >& b,
                                 const Bound< T >& c);

    template < typename T >
    friend const Bound< T >& min(const Bound< T >& a,
                                 const Bound< T >& b,
                                 const Bound< T >& c,
                                 const Bound< T >& d);

    template < typename T >
    friend const Bound< T >& max(const Bound< T >& lhs, const Bound< T >& rhs);

    template < typename T >
    friend const Bound< T >& max(const Bound< T >& a,
                                 const Bound< T >& b,
                                 const Bound< T >& c);

    template < typename T >
    friend const Bound< T >& max(const Bound< T >& a,
                                 const Bound< T >& b,
                                 const Bound< T >& c,
                                 const Bound< T >& d);

    template < typename T >
    inline Bound< T > abs(const Bound< T >& b);

}; // class Bound<Num>

template < typename Num >
inline Bound< Num > operator+(const Bound< Num >& lhs,
                              const Bound< Num >& rhs) {
    using BoundT = Bound< Num >;
    if (lhs.is_finite() && rhs.is_finite()) {
        return BoundT(lhs.m_val + rhs.m_val);
    }
    if (lhs.is_finite() && rhs.is_inf()) {
        return rhs;
    }
    if (lhs.is_inf() && rhs.is_finite()) {
        return lhs;
    }
    if (lhs.m_val == rhs.m_val) {
        return lhs;
    }
    knight_unreachable("undefined op on pinf + ninf");
}

template < typename Num >
inline Bound< Num > operator-(const Bound< Num >& lhs,
                              const Bound< Num >& rhs) {
    using BoundT = Bound< Num >;
    if (lhs.is_finite() && rhs.is_finite()) {
        return BoundT(lhs.m_val - rhs.m_val);
    }
    if (lhs.is_finite() && rhs.is_inf()) {
        return -rhs;
    }
    if (lhs.is_inf() && rhs.is_finite()) {
        return lhs;
    }
    if (lhs.m_val != rhs.m_val) {
        return lhs;
    }
    knight_unreachable("undefined op on inf - inf");
}

template < typename Num >
inline Bound< Num > operator*(const Bound< Num >& lhs,
                              const Bound< Num >& rhs) {
    using BoundT = Bound< Num >;
    if (lhs.is_zero() || rhs.is_zero()) {
        return BoundT(0);
    }
    return BoundT(lhs.m_is_inf || rhs.m_is_inf, lhs.m_val * rhs.m_val);
}

template < typename Num >
inline Bound< Num > operator/(const Bound< Num >& lhs,
                              const Bound< Num >& rhs) {
    using BoundT = Bound< Num >;
    if (rhs.is_zero()) {
        knight_unreachable("division by zero");
    }
    if (lhs.is_finite() && rhs.is_finite()) {
        return BoundT(lhs.m_val / rhs.m_val);
    }
    if (lhs.is_finite() && rhs.is_inf()) {
        return BoundT(0);
    }
    if (lhs.is_inf() && rhs.is_finite()) {
        return rhs.m_val >= 0 ? lhs : -lhs;
    }
    return Bound(true, lhs.m_val / rhs.m_val);
}

template < typename Num >
inline const Bound< Num >& min(const Bound< Num >& a, const Bound< Num >& b) {
    return (a < b) ? a : b;
}

template < typename Num >
inline const Bound< Num >& min(const Bound< Num >& a,
                               const Bound< Num >& b,
                               const Bound< Num >& c) {
    return min(min(a, b), c);
}

template < typename Num >
inline const Bound< Num >& min(const Bound< Num >& a,
                               const Bound< Num >& b,
                               const Bound< Num >& c,
                               const Bound< Num >& d) {
    return min(min(a, b, c), d);
}

template < typename Num >
inline const Bound< Num >& max(const Bound< Num >& a, const Bound< Num >& b) {
    return (a > b) ? a : b;
}

template < typename Num >
inline const Bound< Num >& max(const Bound< Num >& a,
                               const Bound< Num >& b,
                               const Bound< Num >& c) {
    return max(max(a, b), c);
}

template < typename Num >
inline const Bound< Num >& max(const Bound< Num >& a,
                               const Bound< Num >& b,
                               const Bound< Num >& c,
                               const Bound< Num >& d) {
    return max(max(a, b, c), d);
}

template < typename Num >
inline Bound< Num > abs(const Bound< Num >& b) {
    if (b >= Bound< Num >(0)) {
        return b;
    }
    return -b;
}

using ZBound = Bound< llvm::APSInt >;
using QBound = Bound< llvm::APFloat >;

} // namespace knight::dfa