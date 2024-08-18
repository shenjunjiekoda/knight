//===- znum.hpp -------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the ZNum class, which is a wrapper around GMP's
//  mpz_class
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/ADT/FoldingSet.h>
#include <llvm/Support/raw_ostream.h>

#include <functional>
#include <string>
#include <type_traits>
#include <utility>

#include <gmpxx.h>

#include "util/assert.hpp"

namespace knight::dfa {

namespace internal {

using UL = unsigned long;       // NOLINT
using ULL = unsigned long long; // NOLINT
using UI = unsigned int;        // NOLINT
using LL = long long;           // NOLINT
using L = long;                 // NOLINT
using ULI = unsigned long int;  // NOLINT

constexpr UI K32UMask = 0xFFFFFFFFU;
constexpr UI K4Bits = 4U;
constexpr UI K8Bits = 8U;
constexpr UI K32UBits = 32U;
constexpr int K32Bits = 32;
constexpr int K64Bits = 64;
constexpr int K65Bits = 65;
constexpr int K63Bits = 63;
constexpr int K10Base = 10;

template < typename T >
struct MpzTransformer {
    T operator()(T n) { return n; }
};

template < bool >
struct MpzTransformerULL;

template <>
struct MpzTransformerULL< true > {
    UL operator()(ULL n) { return static_cast< UL >(n); }
};

template <>
struct MpzTransformerULL< false > {
    mpz_class operator()(ULL n) {
        static_assert(sizeof(ULL) == K8Bits, "expected 8 bit");
        static_assert(sizeof(UI) == K4Bits, "expected 4 bit");
        mpz_class r(static_cast< UI >(n >> K32UBits));
        r <<= K32Bits;
        r += static_cast< UI >(n & K32UMask);
        return r;
    }
};

template <>
struct MpzTransformer< ULL >
    : public MpzTransformerULL< sizeof(ULL) == sizeof(UL) > {};

template < bool >
struct MpzTransformerLL;

template <>
struct MpzTransformerLL< true > {
    L operator()(LL n) { return static_cast< L >(n); }
};

template <>
struct MpzTransformerLL< false > {
    mpz_class operator()(LL n) {
        static_assert(sizeof(LL) == K8Bits, "expected 8 bit");
        static_assert(sizeof(int) == K4Bits, "expected 4 bit");
        mpz_class r(static_cast< int >(n >> K32Bits)); // NOLINT
        r <<= K32Bits;
        r += static_cast< UI >(n & K32UMask); // NOLINT
        return r;
    }
};

template <>
struct MpzTransformer< LL >
    : public MpzTransformerLL< sizeof(LL) == sizeof(L) > {};

template < typename T >
struct MpzFits;

template <>
struct MpzFits< UI > {
    bool operator()(const mpz_class& n) { return n.fits_uint_p(); }
};

template <>
struct MpzFits< int > {
    bool operator()(const mpz_class& n) { return n.fits_sint_p(); }
};

template <>
struct MpzFits< UL > {
    bool operator()(const mpz_class& n) { return n.fits_ulong_p(); }
};

template <>
struct MpzFits< L > {
    bool operator()(const mpz_class& n) { return n.fits_slong_p(); }
};

/// \brief Helper to handle ULL
template < bool >
struct MpzFitsULL;

template <>
struct MpzFitsULL< true > {
    bool operator()(const mpz_class& n) { return n.fits_ulong_p(); }
};

template <>
struct MpzFitsULL< false > {
    bool operator()(const mpz_class& n) {
        static_assert(sizeof(ULL) == K8Bits, "expected 8 bits");
        static_assert(sizeof(UI) == K4Bits, "expected 4 bits");
        return mpz_sgn(n.get_mpz_t()) >= 0 &&
               mpz_sizeinbase(n.get_mpz_t(), 2) <= K64Bits;
    }
};

template <>
struct MpzFits< ULL > : public MpzFitsULL< sizeof(ULL) == sizeof(UL) > {};

template < bool >
struct MpzFitsLL;

template <>
struct MpzFitsLL< true > {
    bool operator()(const mpz_class& n) { return n.fits_slong_p(); }
};

template <>
struct MpzFitsLL< false > {
    bool operator()(const mpz_class& n) {
        static_assert(sizeof(LL) == K8Bits, "expected 8 bits");
        static_assert(sizeof(int) == K4Bits, "expected 4 bits");
        if (mpz_sgn(n.get_mpz_t()) >= 0) {
            return mpz_sizeinbase(n.get_mpz_t(), 2) <= K63Bits;
        }

        mpz_class m(-n - 1);
        return mpz_sizeinbase(m.get_mpz_t(), 2) <= K63Bits;
    }
};

template <>
struct MpzFits< LL > : public MpzFitsLL< sizeof(LL) == sizeof(L) > {};

template < typename T >
struct MpzTo;

template <>
struct MpzTo< UI > {
    UI operator()(const mpz_class& n) { return static_cast< UI >(n.get_ui()); }
};

template <>
struct MpzTo< int > {
    int operator()(const mpz_class& n) {
        return static_cast< int >(n.get_si());
    }
};

template <>
struct MpzTo< UL > {
    UL operator()(const mpz_class& n) { return n.get_ui(); }
};

template <>
struct MpzTo< L > {
    L operator()(const mpz_class& n) { return n.get_si(); }
};

/// \brief Helper to handle ULL
template < bool >
struct MpzToULL;

template <>
struct MpzToULL< true > {
    ULL operator()(const mpz_class& n) {
        return static_cast< ULL >(n.get_ui());
    }
};

template <>
struct MpzToULL< false > {
    ULL operator()(const mpz_class& n) {
        static_assert(sizeof(ULL) == K8Bits, "expected 8 bits");
        static_assert(sizeof(UI) == K4Bits, "expected 4 bits");
        auto hi = static_cast< ULL >(mpz_class(n >> K32Bits).get_ui());
        auto lo = static_cast< ULL >(mpz_class(n & K32UMask).get_ui());
        return (hi << 32U) + lo; // NOLINT
    }
};

template <>
struct MpzTo< ULL > : public MpzToULL< sizeof(ULL) == sizeof(UL) > {};

/// \brief Helper to handle LL
template < bool >
struct MpzToLL;

template <>
struct MpzToLL< true > {
    LL operator()(const mpz_class& n) { return static_cast< LL >(n.get_si()); }
};

template <>
struct MpzToLL< false > {
    LL operator()(const mpz_class& n) {
        static_assert(sizeof(LL) == K8Bits, "expected 8 bits");
        static_assert(sizeof(int) == K4Bits, "expected 4 bits");
        auto hi = static_cast< LL >(mpz_class(n >> K32Bits).get_si());
        auto lo = static_cast< LL >(mpz_class(n & K32UMask).get_ui());
        return (hi << 32) + lo; // NOLINT
    }
};

template <>
struct MpzTo< LL > : public MpzToLL< sizeof(LL) == sizeof(L) > {};

} // namespace internal

/// \brief Unlimited precision integer
class ZNum : public llvm::FoldingSetNode {
    friend class QNum;

  private:
    mpz_class m_mpz;

  public:
    /// \brief Create a ZNum from a string representation
    ///
    /// Interpret the string `str` in the given base.
    ///
    /// The base may vary from 2 to 36.
    static std::optional< ZNum > from_string(const std::string& str,
                                             int base = internal::K10Base) {
        try {
            return ZNum(mpz_class(str, base));
        } catch (std::invalid_argument&) {
            llvm::errs() << "ZNum: invalid input string `" << str << "`\n";
            return std::nullopt;
        }
    }

    [[nodiscard]] const mpz_class& get_mpz() const { return this->m_mpz; }

    ZNum() = default;
    explicit ZNum(const mpz_class& n) : m_mpz(n) {}
    explicit ZNum(mpz_class&& n) : m_mpz(std::move(n)) {}
    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    explicit ZNum(T n) : m_mpz(internal::MpzTransformer< T >()(n)) {}

    ZNum(const ZNum&) = default;
    ZNum(ZNum&&) = default;
    ZNum& operator=(const ZNum&) = default;
    ZNum& operator=(ZNum&&) noexcept = default;
    ~ZNum() = default;

    // NOLINTNEXTLINE
    void Profile(llvm::FoldingSetNodeID& ID) const {
        const mpz_class& m = this->m_mpz;

        // Add the size of the mpz_t
        ID.AddInteger(m.get_mpz_t()[0]._mp_size);

        // Add each limb of the mpz_t
        for (int i = 0, e = std::abs(m.get_mpz_t()[0]._mp_size); i < e; ++i) {
            ID.AddInteger(m.get_mpz_t()[0]._mp_d[i]); // NOLINT
        }
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    ZNum& operator=(T n) {
        this->m_mpz = internal::MpzTransformer< T >()(n);
        return *this;
    }

    ZNum& operator+=(const ZNum& x) {
        this->m_mpz += x.m_mpz;
        return *this;
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    ZNum& operator+=(T x) {
        this->m_mpz += internal::MpzTransformer< T >()(x);
        return *this;
    }

    ZNum& operator-=(const ZNum& x) {
        this->m_mpz -= x.m_mpz;
        return *this;
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    ZNum& operator-=(T x) {
        this->m_mpz -= internal::MpzTransformer< T >()(x);
        return *this;
    }

    ZNum& operator*=(const ZNum& x) {
        this->m_mpz *= x.m_mpz;
        return *this;
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    ZNum& operator*=(T x) {
        this->m_mpz *= internal::MpzTransformer< T >()(x);
        return *this;
    }

    ZNum& operator/=(const ZNum& x) {
        knight_assert_msg(x.m_mpz != 0, "divided by zero");
        this->m_mpz /= x.m_mpz;
        return *this;
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    ZNum& operator/=(T x) {
        knight_assert_msg(x != 0, "divided by zero");
        this->m_mpz /= internal::MpzTransformer< T >()(x);
        return *this;
    }

    /// 0 <= abs(r) < abs(x)
    ZNum& operator%=(const ZNum& x) {
        knight_assert_msg(x.m_mpz != 0, "divided by zero");
        this->m_mpz %= x.m_mpz;
        return *this;
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    ZNum& operator%=(T x) {
        knight_assert_msg(x != 0, "divided by zero");
        this->m_mpz %= internal::MpzTransformer< T >()(x);
        return *this;
    }

    ZNum& operator&=(const ZNum& x) {
        this->m_mpz &= x.m_mpz;
        return *this;
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    ZNum& operator&=(T x) {
        this->m_mpz &= internal::MpzTransformer< T >()(x);
        return *this;
    }

    /// \brief Bitwise OR assignment
    ZNum& operator|=(const ZNum& x) {
        this->m_mpz |= x.m_mpz;
        return *this;
    }

    /// \brief Bitwise OR assignment with integral types
    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    ZNum& operator|=(T x) {
        this->m_mpz |= internal::MpzTransformer< T >()(x);
        return *this;
    }

    ZNum& operator^=(const ZNum& x) {
        this->m_mpz ^= x.m_mpz;
        return *this;
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    ZNum& operator^=(T x) {
        this->m_mpz ^= internal::MpzTransformer< T >()(x);
        return *this;
    }

    ZNum& operator<<=(const ZNum& x) {
        knight_assert_msg(x.m_mpz >= 0, "shift count is negative");
        knight_assert_msg(x.m_mpz.fits_ulong_p(), "shift count is too big");
        this->m_mpz <<= x.m_mpz.get_ui();
        return *this;
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    ZNum& operator<<=(T x) {
        knight_assert_msg(x >= 0, "shift count is negative");
        this->m_mpz <<= static_cast< internal::ULI >(x);
        return *this;
    }

    ZNum& operator>>=(const ZNum& x) {
        knight_assert_msg(x.m_mpz >= 0, "shift count is negative");
        knight_assert_msg(x.m_mpz.fits_ulong_p(), "shift count is too big");
        this->m_mpz >>= x.m_mpz.get_ui();
        return *this;
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    ZNum& operator>>=(T x) {
        knight_assert_msg(x >= 0, "shift count is negative");
        this->m_mpz >>= static_cast< internal::ULI >(x);
        return *this;
    }

    /// \brief Unary plus
    const ZNum& operator+() const { return *this; }

    /// \brief Prefix increment
    ZNum& operator++() {
        ++this->m_mpz;
        return *this;
    }

    /// \brief Postfix increment
    const ZNum operator++(int) { // NOLINT
        ZNum r(*this);
        ++this->m_mpz;
        return r;
    }

    /// \brief Unary minus
    const ZNum operator-() const { // NOLINT
        return ZNum(-this->m_mpz);
    }

    /// \brief Prefix decrement
    ZNum& operator--() {
        --this->m_mpz;
        return *this;
    }

    /// \brief Postfix decrement
    const ZNum operator--(int) { // NOLINT
        ZNum r(*this);
        --this->m_mpz;
        return r;
    }

    /// \brief Return the next power of 2 greater or equal to this number
    [[nodiscard]] ZNum next_power_of_2() const {
        knight_assert(this->m_mpz >= 0);

        if (this->m_mpz <= 1) {
            return ZNum(1);
        }

        ZNum n(this->m_mpz - 1);
        return ZNum(mpz_class(1) << n.size_in_bits());
    }

    [[nodiscard]] uint64_t trailing_zeros() const {
        knight_assert(this->m_mpz != 0);
        return mpz_scan1(this->m_mpz.get_mpz_t(), 0);
    }

    [[nodiscard]] uint64_t trailing_ones() const {
        knight_assert(this->m_mpz != -1);
        return mpz_scan0(this->m_mpz.get_mpz_t(), 0);
    }

    [[nodiscard]] uint64_t size_in_bits() const {
        return mpz_sizeinbase(this->m_mpz.get_mpz_t(), 2);
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    [[nodiscard]] bool fits() const {
        return internal::MpzFits< T >()(this->m_mpz);
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    T to() const {
        knight_assert_msg(internal::MpzFits< T >()(this->m_mpz),
                          "does not fit");
        return internal::MpzTo< T >()(this->m_mpz);
    }

    [[nodiscard]] std::string str(int base = internal::K10Base) const {
        return this->m_mpz.get_str(base);
    }

    friend ZNum mod(const ZNum&, const ZNum&); // NOLINT

    friend ZNum gcd(const ZNum&, const ZNum&); // NOLINT

    friend ZNum lcm(const ZNum&, const ZNum&); // NOLINT

    friend void gcd_extended(
        const ZNum&, const ZNum&, ZNum&, ZNum&, ZNum&); // NOLINT

    friend ZNum single_mask(const ZNum& size);

    friend ZNum double_mask(const ZNum& low, const ZNum& high);

    friend ZNum make_clipped_mask(const ZNum& low,
                                  const ZNum& size,
                                  const ZNum& lower_clip,
                                  const ZNum& size_clip);

}; // end class ZNum

inline ZNum operator+(const ZNum& lhs, const ZNum& rhs) {
    return ZNum(lhs.get_mpz() + rhs.get_mpz());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator+(const ZNum& lhs, T rhs) {
    return ZNum(lhs.get_mpz() + internal::MpzTransformer< T >()(rhs));
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator+(T lhs, const ZNum& rhs) {
    return ZNum(internal::MpzTransformer< T >()(lhs) + rhs.get_mpz());
}

inline ZNum operator-(const ZNum& lhs, const ZNum& rhs) {
    return ZNum(lhs.get_mpz() - rhs.get_mpz());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator-(const ZNum& lhs, T rhs) {
    return ZNum(lhs.get_mpz() - internal::MpzTransformer< T >()(rhs));
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator-(T lhs, const ZNum& rhs) {
    return ZNum(internal::MpzTransformer< T >()(lhs) - rhs.get_mpz());
}

inline ZNum operator*(const ZNum& lhs, const ZNum& rhs) {
    return ZNum(lhs.get_mpz() * rhs.get_mpz());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator*(const ZNum& lhs, T rhs) {
    return ZNum(lhs.get_mpz() * internal::MpzTransformer< T >()(rhs));
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator*(T lhs, const ZNum& rhs) {
    return ZNum(internal::MpzTransformer< T >()(lhs) * rhs.get_mpz());
}

inline ZNum operator/(const ZNum& lhs, const ZNum& rhs) {
    knight_assert_msg(rhs.get_mpz() != 0, "divided by zero");
    return ZNum(lhs.get_mpz() / rhs.get_mpz());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator/(const ZNum& lhs, T rhs) {
    knight_assert_msg(rhs != 0, "divided by zero");
    return ZNum(lhs.get_mpz() / internal::MpzTransformer< T >()(rhs));
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator/(T lhs, const ZNum& rhs) {
    knight_assert_msg(rhs.get_mpz() != 0, "divided by zero");
    return ZNum(internal::MpzTransformer< T >()(lhs) / rhs.get_mpz());
}

inline ZNum operator%(const ZNum& lhs, const ZNum& rhs) {
    knight_assert_msg(rhs.get_mpz() != 0, "divided by zero");
    return ZNum(lhs.get_mpz() % rhs.get_mpz());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator%(const ZNum& lhs, T rhs) {
    knight_assert_msg(rhs != 0, "divided by zero");
    return ZNum(lhs.get_mpz() % internal::MpzTransformer< T >()(rhs));
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator%(T lhs, const ZNum& rhs) {
    knight_assert_msg(rhs.get_mpz() != 0, "divided by zero");
    return ZNum(internal::MpzTransformer< T >()(lhs) % rhs.get_mpz());
}

inline ZNum operator&(const ZNum& lhs, const ZNum& rhs) {
    return ZNum(lhs.get_mpz() & rhs.get_mpz());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator&(const ZNum& lhs, T rhs) {
    return ZNum(lhs.get_mpz() & internal::MpzTransformer< T >()(rhs));
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator&(T lhs, const ZNum& rhs) {
    return ZNum(internal::MpzTransformer< T >()(lhs) & rhs.get_mpz());
}

inline ZNum operator|(const ZNum& lhs, const ZNum& rhs) {
    return ZNum(lhs.get_mpz() | rhs.get_mpz());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator|(const ZNum& lhs, T rhs) {
    return ZNum(lhs.get_mpz() | internal::MpzTransformer< T >()(rhs));
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator|(T lhs, const ZNum& rhs) {
    return ZNum(internal::MpzTransformer< T >()(lhs) | rhs.get_mpz());
}

inline ZNum operator^(const ZNum& lhs, const ZNum& rhs) {
    return ZNum(lhs.get_mpz() ^ rhs.get_mpz());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator^(const ZNum& lhs, T rhs) {
    return ZNum(lhs.get_mpz() ^ internal::MpzTransformer< T >()(rhs));
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator^(T lhs, const ZNum& rhs) {
    return ZNum(internal::MpzTransformer< T >()(lhs) ^ rhs.get_mpz());
}

inline ZNum operator<<(const ZNum& lhs, const ZNum& rhs) {
    knight_assert_msg(rhs.get_mpz() >= 0, "shift count is negative");
    knight_assert_msg(rhs.get_mpz().fits_ulong_p(), "shift count is too big");
    return ZNum(lhs.get_mpz() << rhs.get_mpz().get_ui());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator<<(const ZNum& lhs, T rhs) {
    knight_assert_msg(rhs >= 0, "shift count is negative");
    return ZNum(lhs.get_mpz() << static_cast< internal::ULI >(rhs));
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator<<(T lhs, const ZNum& rhs) {
    ZNum r(lhs);
    return r <<= rhs;
}

inline ZNum operator>>(const ZNum& lhs, const ZNum& rhs) {
    knight_assert_msg(rhs.get_mpz() >= 0, "shift count is negative");
    knight_assert_msg(rhs.get_mpz().fits_ulong_p(), "shift count is too big");
    return ZNum(lhs.get_mpz() >> rhs.get_mpz().get_ui());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator>>(const ZNum& lhs, T rhs) {
    knight_assert_msg(rhs >= 0, "shift count is negative");
    return ZNum(lhs.get_mpz() >> static_cast< internal::ULI >(rhs));
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline ZNum operator>>(T lhs, const ZNum& rhs) {
    ZNum r(lhs);
    r >>= rhs;
    return r;
}

inline bool operator==(const ZNum& lhs, const ZNum& rhs) {
    return lhs.get_mpz() == rhs.get_mpz();
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator==(const ZNum& lhs, T rhs) {
    return lhs.get_mpz() == internal::MpzTransformer< T >()(rhs);
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator==(T lhs, const ZNum& rhs) {
    return internal::MpzTransformer< T >()(lhs) == rhs.get_mpz();
}

inline bool operator!=(const ZNum& lhs, const ZNum& rhs) {
    return lhs.get_mpz() != rhs.get_mpz();
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator!=(const ZNum& lhs, T rhs) {
    return lhs.get_mpz() != internal::MpzTransformer< T >()(rhs);
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator!=(T lhs, const ZNum& rhs) {
    return internal::MpzTransformer< T >()(lhs) != rhs.get_mpz();
}

inline bool operator<(const ZNum& lhs, const ZNum& rhs) {
    return lhs.get_mpz() < rhs.get_mpz();
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator<(const ZNum& lhs, T rhs) {
    return lhs.get_mpz() < internal::MpzTransformer< T >()(rhs);
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator<(T lhs, const ZNum& rhs) {
    return internal::MpzTransformer< T >()(lhs) < rhs.get_mpz();
}

inline bool operator<=(const ZNum& lhs, const ZNum& rhs) {
    return lhs.get_mpz() <= rhs.get_mpz();
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator<=(const ZNum& lhs, T rhs) {
    return lhs.get_mpz() <= internal::MpzTransformer< T >()(rhs);
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator<=(T lhs, const ZNum& rhs) {
    return internal::MpzTransformer< T >()(lhs) <= rhs.get_mpz();
}

inline bool operator>(const ZNum& lhs, const ZNum& rhs) {
    return lhs.get_mpz() > rhs.get_mpz();
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator>(const ZNum& lhs, T rhs) {
    return lhs.get_mpz() > internal::MpzTransformer< T >()(rhs);
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator>(T lhs, const ZNum& rhs) {
    return internal::MpzTransformer< T >()(lhs) > rhs.get_mpz();
}

inline bool operator>=(const ZNum& lhs, const ZNum& rhs) {
    return lhs.get_mpz() >= rhs.get_mpz();
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator>=(const ZNum& lhs, T rhs) {
    return lhs.get_mpz() >= internal::MpzTransformer< T >()(rhs);
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator>=(T lhs, const ZNum& rhs) {
    return internal::MpzTransformer< T >()(lhs) >= rhs.get_mpz();
}

inline const ZNum& min(const ZNum& a, const ZNum& b) {
    return (a < b) ? a : b;
}

inline const ZNum& min(const ZNum& a, const ZNum& b, const ZNum& c) {
    return min(min(a, b), c);
}

inline const ZNum& min(const ZNum& a,
                       const ZNum& b,
                       const ZNum& c,
                       const ZNum& d) {
    return min(min(min(a, b), c), d);
}

inline const ZNum& max(const ZNum& a, const ZNum& b) {
    return (a < b) ? b : a;
}

inline const ZNum& max(const ZNum& a, const ZNum& b, const ZNum& c) {
    return max(max(a, b), c);
}

inline const ZNum& max(const ZNum& a,
                       const ZNum& b,
                       const ZNum& c,
                       const ZNum& d) {
    return max(max(max(a, b), c), d);
}

inline ZNum mod(const ZNum& n, const ZNum& d) {
    knight_assert_msg(d.get_mpz() != 0, "divided by zero");
    ZNum r;
    mpz_mod(r.m_mpz.get_mpz_t(), n.m_mpz.get_mpz_t(), d.m_mpz.get_mpz_t());
    return r;
}

inline ZNum abs(const ZNum& n) {
    return ZNum(abs(n.get_mpz()));
}

inline ZNum gcd(const ZNum& a, const ZNum& b) {
    ZNum r;
    mpz_gcd(r.m_mpz.get_mpz_t(), a.m_mpz.get_mpz_t(), b.m_mpz.get_mpz_t());
    return r;
}

inline ZNum gcd(const ZNum& a, const ZNum& b, const ZNum& c) {
    return gcd(gcd(a, b), c);
}

inline ZNum lcm(const ZNum& a, const ZNum& b) {
    ZNum r;
    mpz_lcm(r.m_mpz.get_mpz_t(), a.m_mpz.get_mpz_t(), b.m_mpz.get_mpz_t());
    return r;
}

inline void gcd_extended(
    const ZNum& a, const ZNum& b, ZNum& g, ZNum& u, ZNum& v) {
    mpz_gcdext(g.m_mpz.get_mpz_t(),
               u.m_mpz.get_mpz_t(),
               v.m_mpz.get_mpz_t(),
               a.m_mpz.get_mpz_t(),
               b.m_mpz.get_mpz_t());
}

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const ZNum& n) {
    return os << n.get_mpz().get_str();
}

inline void hash_combine(std::size_t& seed, std::size_t value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2); // NOLINT
}

inline std::size_t hash_value(const ZNum& n) {
    const mpz_class& m = n.get_mpz();
    std::size_t result = 0;
    hash_combine(result, std::hash< int >()(m.get_mpz_t()[0]._mp_size));
    for (int i = 0, e = std::abs(m.get_mpz_t()[0]._mp_size); i < e; ++i) {
        hash_combine(result,
                     std::hash< mp_limb_t >()(
                         m.get_mpz_t()[0]._mp_d[i])); // NOLINT
    }
    return result;
}

} // end namespace knight::dfa

namespace std {

/// \brief std::hash implementation for ZNum
template <>
struct hash< knight::dfa::ZNum > {
    std::size_t operator()(const knight::dfa::ZNum& n) const {
        return knight::dfa::hash_value(n);
    }
};

} // namespace std
