//===- machine_znum.hpp -----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the MachineZNum class.
//
//===------------------------------------------------------------------===//

#pragma once

#include <cstdint>

#include "analyzer/core/domain/num/znum.hpp"

namespace knight::analyzer {

constexpr uint64_t MaxU64 = ~static_cast< uint64_t >(0);

enum Signedness { Signed = 0, Unsigned = 1 };

template < typename X, typename Y >
inline void knight_assert_compatiblem_num(const X& x, const Y& y) {
    knight_assert_msg(x.get_bit_width() == y.get_bit_width(),
                      "parameters have a different bit-width");
    knight_assert_msg(x.get_sign() == y.get_sign(),
                      "parameters have a different signedness");
}

/// \brief Arbitrary precision machine ZNum
class MachineZNum {
  private:
    struct NormalizedTag {};

  private:
    /// If bit-width <= 64, store directly the integer,
    /// Otherwise use a pointer on a ZNum.
    union {
        uint64_t i; /// Used to store the <= 64 bits integer value.
        ZNum* p;    /// Used to store the >64 bits integer value.
    } m_ap;
    uint64_t m_bit_width;
    Signedness m_sign;

  public:
    MachineZNum() = delete;
    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    // NOLINTNEXTLINE(hicpp-member-init,cppcoreguidelines-pro-type-member-init)
    MachineZNum(T n, uint64_t bit_width, Signedness sign)
        : m_bit_width(bit_width), m_sign(sign) {
        knight_assert_msg(bit_width > 0, "invalid bit width");

        if (this->is_small_num()) {
            this->m_ap.i = static_cast< uint64_t >(n);
        } else {
            this->m_ap.p = new ZNum(n);
        }

        this->normalize();
    }

    // NOLINTNEXTLINE(hicpp-member-init,cppcoreguidelines-pro-type-member-init)
    MachineZNum(const ZNum& n, uint64_t bit_width, Signedness sign)
        : m_bit_width(bit_width), m_sign(sign) {
        knight_assert_msg(bit_width > 0, "invalid bit width");

        if (this->is_small_num()) {
            ZNum m = mod(n, power_2(this->m_bit_width));
            this->m_ap.i = m.to< uint64_t >();
        } else {
            this->m_ap.p = new ZNum(n);
        }

        this->normalize();
    }

    // NOLINTNEXTLINE(hicpp-member-init,cppcoreguidelines-pro-type-member-init)
    MachineZNum(const MachineZNum& o)
        : m_bit_width(o.m_bit_width), m_sign(o.m_sign) {
        if (o.is_small_num()) {
            this->m_ap.i = o.m_ap.i;
        } else {
            this->m_ap.p = new ZNum(*o.m_ap.p);
        }
    }

    MachineZNum(MachineZNum&& o) noexcept
        : m_ap(o.m_ap), m_bit_width(o.m_bit_width), m_sign(o.m_sign) {
        o.m_bit_width = 0;
    }

    ~MachineZNum() {
        if (this->is_large_num()) {
            delete this->m_ap.p;
        }
    }

  private:
    // NOLINTNEXTLINE
    MachineZNum(uint64_t n, uint64_t bit_width, Signedness sign, NormalizedTag)
        : m_bit_width(bit_width), m_sign(sign) {
        knight_assert_msg(bit_width > 0, "invalid bit width");

        if (this->is_small_num()) {
            this->m_ap.i = n;
        } else {
            this->m_ap.p = new ZNum(n);
        }
    }

    // NOLINTNEXTLINE(hicpp-member-init,cppcoreguidelines-pro-type-member-init)
    MachineZNum(const ZNum& n,
                uint64_t bit_width,
                Signedness sign,
                NormalizedTag) // NOLINT
        : m_bit_width(bit_width), m_sign(sign) {
        knight_assert_msg(bit_width > 0, "invalid bit width");

        if (this->is_small_num()) {
            this->m_ap.i = n.to< uint64_t >();
        } else {
            this->m_ap.p = new ZNum(n);
        }
    }

  public:
    [[nodiscard]] bool is_small_num() const {
        return this->m_bit_width <= internal::K64Bits;
    }
    [[nodiscard]] bool is_large_num() const { return !this->is_small_num(); }

    [[nodiscard]] static ZNum power_2(uint64_t n) { return ZNum(1) << n; }
    [[nodiscard]] static ZNum power_2(const ZNum& n) { return ZNum(1) << n; }

    void normalize() {
        if (this->is_small_num()) {
            uint64_t mask = MaxU64 >> (internal::K64Bits - this->m_bit_width);
            this->m_ap.i &= mask;
        }
        if (*this->m_ap.p == 0) {
            return;
        }
        if (this->is_signed()) {
            *this->m_ap.p = mod(*this->m_ap.p + power_2(this->m_bit_width - 1),
                                power_2(this->m_bit_width)) -
                            power_2(this->m_bit_width - 1);
            return;
        }
        *this->m_ap.p = mod(*this->m_ap.p, power_2(this->m_bit_width));
    }

  public:
    [[nodiscard]] static MachineZNum min(uint64_t bit_width, Signedness sign) {
        knight_assert_msg(bit_width > 0, "invalid bit width");

        if (sign == Signed) {
            if (bit_width <= internal::K64Bits) {
                return MachineZNum(static_cast< uint64_t >(1)
                                       << (bit_width - 1),
                                   bit_width,
                                   sign,
                                   NormalizedTag{});
            }

            return MachineZNum(-power_2(bit_width - 1),
                               bit_width,
                               sign,
                               NormalizedTag{});
        }

        return MachineZNum(0, bit_width, sign, NormalizedTag{});
    }

    static MachineZNum max(uint64_t bit_width, Signedness sign) {
        knight_assert_msg(bit_width > 0, "invalid bit width");

        if (sign == Signed) {
            if (bit_width == 1) {
                return MachineZNum(0, bit_width, sign, NormalizedTag{});
            }

            if (bit_width <= internal::K64Bits) {
                return MachineZNum(MaxU64 >> (internal::K65Bits - bit_width),
                                   bit_width,
                                   sign,
                                   NormalizedTag{});
            }

            return MachineZNum(power_2(bit_width - 1) - 1,
                               bit_width,
                               sign,
                               NormalizedTag{});
        }

        if (bit_width <= internal::K64Bits) {
            return MachineZNum(MaxU64 >> (internal::K64Bits - bit_width),
                               bit_width,
                               sign,
                               NormalizedTag{});
        }

        return MachineZNum(power_2(bit_width) - 1,
                           bit_width,
                           sign,
                           NormalizedTag{});
    }

    [[nodiscard]] static MachineZNum zero(uint64_t bit_width, Signedness sign) {
        knight_assert_msg(bit_width > 0, "invalid bit width");
        return MachineZNum(0, bit_width, sign, NormalizedTag{});
    }

    [[nodiscard]] static MachineZNum ones(uint64_t bit_width, Signedness sign) {
        knight_assert_msg(bit_width > 0, "invalid bit width");

        if (bit_width <= internal::K64Bits) {
            return MachineZNum(MaxU64 >> (internal::K64Bits - bit_width),
                               bit_width,
                               sign,
                               NormalizedTag{});
        }

        if (sign == Signed) {
            return MachineZNum(ZNum(-1), bit_width, sign, NormalizedTag{});
        }

        return MachineZNum(power_2(bit_width) - 1,
                           bit_width,
                           sign,
                           NormalizedTag{});
    }

    MachineZNum& operator=(const MachineZNum& o) {
        if (this == &o) {
            return *this;
        }

        if (this->is_small_num()) {
            if (o.is_small_num()) {
                this->m_ap.i = o.m_ap.i;
            } else {
                this->m_ap.p = new ZNum(*o.m_ap.p);
            }
        } else {
            if (o.is_small_num()) {
                delete this->m_ap.p;
                this->m_ap.i = o.m_ap.i;
            } else {
                *this->m_ap.p = *o.m_ap.p;
            }
        }

        this->m_bit_width = o.m_bit_width;
        this->m_sign = o.m_sign;
        return *this;
    }

    MachineZNum& operator=(MachineZNum&& o) noexcept {
        if (this == &o) {
            return *this;
        }

        if (this->is_large_num()) {
            delete this->m_ap.p;
        }

        this->m_ap = o.m_ap;
        this->m_bit_width = o.m_bit_width;
        this->m_sign = o.m_sign;
        o.m_bit_width = 0;
        return *this;
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    MachineZNum& operator=(T n) {
        if (this->is_small_num()) {
            this->m_ap.i = static_cast< uint64_t >(n);
        } else {
            *this->m_ap.p = n;
        }
        this->normalize();
        return *this;
    }

    MachineZNum& operator+=(const MachineZNum& x) {
        knight_assert_compatiblem_num(*this, x);
        if (this->is_small_num()) {
            this->m_ap.i += x.m_ap.i;
        } else {
            *this->m_ap.p += *x.m_ap.p;
        }
        this->normalize();
        return *this;
    }

    MachineZNum& operator-=(const MachineZNum& x) {
        knight_assert_compatiblem_num(*this, x);
        if (this->is_small_num()) {
            this->m_ap.i -= x.m_ap.i;
        } else {
            *this->m_ap.p -= *x.m_ap.p;
        }
        this->normalize();
        return *this;
    }

    MachineZNum& operator*=(const MachineZNum& x) {
        knight_assert_compatiblem_num(*this, x);
        if (this->is_small_num()) {
            this->m_ap.i *= x.m_ap.i;
        } else {
            *this->m_ap.p *= *x.m_ap.p;
        }
        this->normalize();
        return *this;
    }

    MachineZNum& operator/=(const MachineZNum& x) {
        *this = div(*this, x);
        return *this;
    }

    MachineZNum& operator%=(const MachineZNum& x) {
        *this = rem(*this, x);
        return *this;
    }

  public:
    [[nodiscard]] uint64_t get_bit_width() const { return this->m_bit_width; }

    [[nodiscard]] Signedness get_sign() const { return this->m_sign; }

    [[nodiscard]] bool is_signed() const { return this->m_sign == Signed; }
    [[nodiscard]] bool is_unsigned() const { return this->m_sign == Unsigned; }

    [[nodiscard]] bool is_minimum() const {
        if (this->is_signed()) {
            if (this->is_small_num()) {
                return this->m_ap.i ==
                       (static_cast< uint64_t >(1) << (this->m_bit_width - 1));
            }

            return *this->m_ap.p == -power_2(this->m_bit_width - 1);
        }

        return this->is_zero();
    }

    [[nodiscard]] bool is_maximum() const {
        if (this->is_signed()) {
            if (this->get_bit_width() == 1) {
                return this->m_ap.i == 0;
            }

            if (this->is_small_num()) {
                return this->m_ap.i ==
                       (MaxU64 >> (internal::K65Bits - this->m_bit_width));
            }

            return *this->m_ap.p == (power_2(this->m_bit_width - 1) - 1);
        }

        if (this->is_small_num()) {
            return this->m_ap.i ==
                   (MaxU64 >> (internal::K64Bits - this->m_bit_width));
        }

        return *this->m_ap.p == (power_2(this->m_bit_width) - 1);
    }

    [[nodiscard]] bool is_zero() const {
        if (this->is_small_num()) {
            return this->m_ap.i == 0;
        }

        return *this->m_ap.p == 0;
    }

    [[nodiscard]] bool high_bit() const {
        if (this->is_small_num()) {
            return (this->m_ap.i >> (this->m_bit_width - 1)) != 0;
        }

        if (this->is_signed()) {
            return *this->m_ap.p < 0;
        }

        return *this->m_ap.p >= power_2(this->m_bit_width - 1);
    }

    [[nodiscard]] bool is_negative() const {
        return this->is_signed() && this->high_bit();
    }

    [[nodiscard]] bool is_non_negative() const { return !this->is_negative(); }

    [[nodiscard]] bool is_positive() const {
        if (this->is_small_num()) {
            if (this->is_signed()) {
                return !this->high_bit() && this->m_ap.i != 0;
            }
            return this->m_ap.i != 0;
        }

        return *this->m_ap.p > 0;
    }

    [[nodiscard]] bool all_ones() const {
        if (this->is_small_num()) {
            return this->m_ap.i ==
                   (MaxU64 >> (internal::K64Bits - this->m_bit_width));
        }

        if (this->is_signed()) {
            return *this->m_ap.p == -1;
        }

        return *this->m_ap.p == (power_2(this->m_bit_width) - 1);
    }

  private:
    [[nodiscard]] static uint64_t leading_zeros(uint64_t n) {
        if (n == 0) {
            return internal::K64Bits;
        }

#if __has_builtin(__builtin_clzll)
        if (std::is_same< uint64_t, internal::ULL >::value) {
            return static_cast< uint64_t >(__builtin_clzll(n));
        }
#endif

        uint64_t zeros = 0;
        for (uint64_t shift = 64U >> 1U; shift != 0U; shift >>= 1U) { // NOLINT
            uint64_t tmp = n >> shift;
            if (tmp != 0) {
                n = tmp;
            } else {
                zeros |= shift;
            }
        }
        return zeros;
    }

    static uint64_t leading_zeros(ZNum n, uint64_t bit_width) {
        if (n == 0) {
            return bit_width;
        }

        if (n < 0) {
            return 0;
        }

        uint64_t zeros = 0;
        for (uint64_t shift = bit_width >> 1U; shift != 0U; shift >>= 1U) {
            ZNum tmp = n >> shift;
            if (tmp != 0) {
                n = tmp;
            } else {
                zeros |= shift;
            }
        }
        return zeros;
    }

    [[nodiscard]] static uint64_t leading_ones(uint64_t n) {
        return leading_zeros(~n);
    }

    [[nodiscard]] static uint64_t leading_ones(ZNum n, uint64_t bit_width) {
        n = mod(n, power_2(bit_width));
        n = n ^ (power_2(bit_width) - 1);
        return leading_zeros(n, bit_width);
    }

  public:
    [[nodiscard]] uint64_t leading_zeros() const {
        if (this->is_small_num()) {
            uint64_t unused_bits = internal::K64Bits - this->m_bit_width;
            return leading_zeros(this->m_ap.i) - unused_bits;
        }

        return leading_zeros(*this->m_ap.p, this->m_bit_width);
    }

    [[nodiscard]] uint64_t leading_ones() const {
        if (this->is_small_num()) {
            return leading_ones(this->m_ap.i
                                << (internal::K64Bits - this->m_bit_width));
        }

        return leading_ones(*this->m_ap.p, this->m_bit_width);
    }

  private:
    [[nodiscard]] static int64_t sign_extend_64(uint64_t n,
                                                uint64_t bit_width) {
        knight_assert_msg(bit_width > 0 && bit_width <= 64,
                          "invalid bit-width");
        // NOLINTNEXTLINE
        return static_cast< int64_t >(n << (64U - bit_width)) >>
               (64U - bit_width); // NOLINT
    }

  public:
    [[nodiscard]] ZNum to_z_number() const {
        if (this->is_small_num()) {
            if (this->is_signed()) {
                return ZNum(sign_extend_64(this->m_ap.i, this->m_bit_width));
            }

            return ZNum(this->m_ap.i);
        }

        return *this->m_ap.p;
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    [[nodiscard]] bool fits() const {
        if (this->is_small_num()) {
            if (this->is_signed()) {
                int64_t n = sign_extend_64(this->m_ap.i, this->m_bit_width);

                if (std::numeric_limits< T >::is_signed) {
                    return int64_t(std::numeric_limits< T >::min()) <= n &&
                           n <= int64_t(std::numeric_limits< T >::max());
                }
                return static_cast< int64_t >(0) <= n &&
                       static_cast< uint64_t >(n) <=
                           uint64_t(std::numeric_limits< T >::max());
            }
            uint64_t n = this->m_ap.i;

            if (std::numeric_limits< T >::is_signed) {
                return n <= uint64_t(std::numeric_limits< T >::max());
            }
            return uint64_t(std::numeric_limits< T >::min()) <= n &&
                   n <= uint64_t(std::numeric_limits< T >::max());
        }
        return this->m_ap.p->fits< T >();
    }

    template < typename T,
               class = std::enable_if_t< std::is_integral< T >::value > >
    [[nodiscard]] T to() const {
        knight_assert_msg(this->fits< T >(), "does not fit");

        if (this->is_small_num()) {
            if (this->is_signed()) {
                return static_cast< T >(
                    sign_extend_64(this->m_ap.i, this->m_bit_width));
            }
            return static_cast< T >(this->m_ap.i);
        }
        return this->m_ap.p->to< T >();
    }

    [[nodiscard]] std::string str(int base = internal::K10Base) const {
        if (this->is_small_num()) {
            if (this->is_signed()) {
                return std::to_string(
                    sign_extend_64(this->m_ap.i, this->m_bit_width));
            }

            return std::to_string(this->m_ap.i);
        }

        return this->m_ap.p->str(base);
    }

    void set_minimum() {
        if (this->is_signed()) {
            if (this->is_small_num()) {
                this->m_ap.i = static_cast< uint64_t >(1)
                               << (this->m_bit_width - 1);
            } else {
                *this->m_ap.p = -power_2(this->m_bit_width - 1);
            }
        } else {
            this->set_zero();
        }
    }

    // \brief Set the machine integer to the maximum integer
    void set_maximum() {
        if (this->is_signed()) {
            if (this->m_bit_width == 1) {
                this->m_ap.i = 0;
            } else if (this->is_small_num()) {
                this->m_ap.i =
                    MaxU64 >> (internal::K65Bits - this->m_bit_width);
            } else {
                *this->m_ap.p = power_2(this->m_bit_width - 1) - 1;
            }
        } else {
            if (this->is_small_num()) {
                this->m_ap.i =
                    MaxU64 >> (internal::K64Bits - this->m_bit_width);
            } else {
                *this->m_ap.p = power_2(this->m_bit_width) - 1;
            }
        }
    }

    // \brief Set the machine integer to zero
    void set_zero() {
        if (this->is_small_num()) {
            this->m_ap.i = 0;
        } else {
            *this->m_ap.p = 0;
        }
    }

    const MachineZNum& operator+() const { return *this; }

    MachineZNum& operator++() {
        if (this->is_small_num()) {
            ++this->m_ap.i;
        } else {
            ++(*this->m_ap.p);
        }
        this->normalize();
        return *this;
    }

    const MachineZNum operator++(int) { // NOLINT
        MachineZNum r(*this);
        if (this->is_small_num()) {
            ++this->m_ap.i;
        } else {
            ++(*this->m_ap.p);
        }
        this->normalize();
        return r;
    }

    const MachineZNum operator-() const { // NOLINT
        if (this->is_small_num()) {
            return {(~this->m_ap.i) + 1, this->m_bit_width, this->m_sign};
        }

        return {-(*this->m_ap.p), this->m_bit_width, this->m_sign};
    }

    void negate() {
        if (this->is_small_num()) {
            this->m_ap.i = (~this->m_ap.i) + 1;
        } else {
            *this->m_ap.p = -(*this->m_ap.p);
        }
        this->normalize();
    }

    const MachineZNum operator~() const { // NOLINT
        if (this->is_small_num()) {
            return {~this->m_ap.i, this->m_bit_width, this->m_sign};
        }

        return {-(*this->m_ap.p) - 1, this->m_bit_width, this->m_sign};
    }

    void flip_bits() {
        if (this->is_small_num()) {
            this->m_ap.i = ~this->m_ap.i;
        } else {
            *this->m_ap.p = -(*this->m_ap.p) - 1;
        }
        this->normalize();
    }

    MachineZNum& operator--() {
        if (this->is_small_num()) {
            --this->m_ap.i;
        } else {
            --(*this->m_ap.p);
        }

        this->normalize();
        return *this;
    }

    const MachineZNum operator--(int) { // NOLINT
        MachineZNum r(*this);
        if (this->is_small_num()) {
            --this->m_ap.i;
        } else {
            --(*this->m_ap.p);
        }

        this->normalize();
        return r;
    }

    [[nodiscard]] MachineZNum trunc_to_bit_width(uint64_t bit_width) const {
        knight_assert(this->m_bit_width > bit_width);

        if (this->is_small_num()) {
            return {this->m_ap.i, bit_width, this->m_sign};
        }
        return {*this->m_ap.p, bit_width, this->m_sign};
    }

    [[nodiscard]] MachineZNum ext_to_bit_width(uint64_t bit_width) const {
        knight_assert(this->m_bit_width < bit_width);

        if (this->is_small_num()) {
            if (this->is_signed()) {
                return {sign_extend_64(this->m_ap.i, this->m_bit_width),
                        bit_width,
                        this->m_sign};
            }
            return {this->m_ap.i, bit_width, this->m_sign, NormalizedTag{}};
        }
        return {*this->m_ap.p, bit_width, this->m_sign, NormalizedTag{}};
    }

    [[nodiscard]] MachineZNum sign_cast(Signedness sign) const {
        knight_assert(this->m_sign != sign);

        if (this->is_small_num()) {
            return {this->m_ap.i, this->m_bit_width, sign, NormalizedTag{}};
        }

        return {*this->m_ap.p, this->m_bit_width, sign};
    }

    [[nodiscard]] MachineZNum cast(uint64_t bit_width, Signedness sign) const {
        if (this->is_small_num()) {
            if (this->is_signed()) {
                return {sign_extend_64(this->m_ap.i, this->m_bit_width),
                        bit_width,
                        sign};
            }

            return {this->m_ap.i, bit_width, sign};
        }

        return {*this->m_ap.p, bit_width, sign};
    }

  private:
    [[nodiscard]] static bool occurred_overflow(const ZNum& n,
                                                uint64_t bit_width,
                                                Signedness sign) {
        if (sign == Signed) {
            return n >= power_2(bit_width - 1);
        }

        return n >= power_2(bit_width);
    }

    [[nodiscard]] static bool occurred_underflow(const ZNum& n,
                                                 uint64_t bit_width,
                                                 Signedness sign) {
        if (sign == Signed) {
            return n < -power_2(bit_width - 1);
        }

        return n < 0;
    }

  public:
    friend MachineZNum add(const MachineZNum& lhs,
                           const MachineZNum& rhs,
                           bool& overflow);

    friend MachineZNum add(const MachineZNum& lhs, const MachineZNum& rhs);

    friend MachineZNum operator+(const MachineZNum& lhs,
                                 const MachineZNum& rhs);

    friend MachineZNum sub(const MachineZNum& lhs,
                           const MachineZNum& rhs,
                           bool& overflow);

    friend MachineZNum sub(const MachineZNum& lhs, const MachineZNum& rhs);

    friend MachineZNum operator-(const MachineZNum& lhs,
                                 const MachineZNum& rhs);

    friend MachineZNum mul(const MachineZNum& lhs,
                           const MachineZNum& rhs,
                           bool& overflow);

    friend MachineZNum mul(const MachineZNum& lhs, const MachineZNum& rhs);

    friend MachineZNum operator*(const MachineZNum& lhs,
                                 const MachineZNum& rhs);

    friend MachineZNum div(const MachineZNum& lhs,
                           const MachineZNum& rhs,
                           bool& overflow,
                           bool& exact);

    friend MachineZNum div(const MachineZNum& lhs, const MachineZNum& rhs);

    friend MachineZNum operator/(const MachineZNum& lhs,
                                 const MachineZNum& rhs);

    friend MachineZNum rem(const MachineZNum& lhs, const MachineZNum& rhs);

    friend MachineZNum operator%(const MachineZNum& lhs,
                                 const MachineZNum& rhs);

    friend MachineZNum mod(const MachineZNum& lhs, const MachineZNum& rhs);

    friend MachineZNum shl(const MachineZNum& lhs,
                           const MachineZNum& rhs,
                           bool& overflow);

    friend MachineZNum shl(const MachineZNum& lhs, const MachineZNum& rhs);

    friend MachineZNum lshr(const MachineZNum& lhs,
                            const MachineZNum& rhs,
                            bool& exact);

    friend MachineZNum lshr(const MachineZNum& lhs, const MachineZNum& rhs);

    friend MachineZNum ashr(const MachineZNum& lhs,
                            const MachineZNum& rhs,
                            bool& exact);

    friend MachineZNum ashr(const MachineZNum& lhs, const MachineZNum& rhs);

    friend MachineZNum bit_and(const MachineZNum& lhs, const MachineZNum& rhs);

    friend MachineZNum operator&(const MachineZNum& lhs,
                                 const MachineZNum& rhs);

    friend MachineZNum bit_or(const MachineZNum& lhs, const MachineZNum& rhs);

    friend MachineZNum operator|(const MachineZNum& lhs,
                                 const MachineZNum& rhs);

    friend MachineZNum bit_xor(const MachineZNum& lhs, const MachineZNum& rhs);

    friend MachineZNum operator^(const MachineZNum& lhs,
                                 const MachineZNum& rhs);

    friend MachineZNum gcd(const MachineZNum& a, const MachineZNum& b);

    friend bool operator==(const MachineZNum& lhs, const MachineZNum& rhs);

    friend bool operator!=(const MachineZNum& lhs, const MachineZNum& rhs);

    friend bool operator<(const MachineZNum& lhs, const MachineZNum& rhs);

    friend bool operator<=(const MachineZNum& lhs, const MachineZNum& rhs);

    friend bool operator>(const MachineZNum& lhs, const MachineZNum& rhs);

    friend bool operator>=(const MachineZNum& lhs, const MachineZNum& rhs);

    friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                         const MachineZNum& n);

    friend std::size_t hash_value(const MachineZNum&); // NOLINT

}; // end class MachineZNum

inline void knight_assert_shift(const MachineZNum& n, uint64_t bit_width) {
    knight_assert_msg(n.is_non_negative(), "shift count is negative");
    knight_assert_msg(n.to_z_number() < bit_width, "shift count is too big");
}

inline MachineZNum add(const MachineZNum& lhs,
                       const MachineZNum& rhs,
                       bool& overflow) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        MachineZNum result(lhs.m_ap.i + rhs.m_ap.i,
                           lhs.m_bit_width,
                           lhs.m_sign);
        if (lhs.is_signed()) {
            overflow = lhs.is_non_negative() == rhs.is_non_negative() &&
                       result.is_non_negative() != lhs.is_non_negative();
        } else {
            overflow = result < rhs;
        }
        return result;
    }

    ZNum n = (*lhs.m_ap.p) + (*rhs.m_ap.p);
    overflow = MachineZNum::occurred_overflow(n, lhs.m_bit_width, lhs.m_sign) ||
               MachineZNum::occurred_underflow(n, lhs.m_bit_width, lhs.m_sign);
    return {n, lhs.m_bit_width, lhs.m_sign};
}

inline MachineZNum add(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        return {lhs.m_ap.i + rhs.m_ap.i, lhs.m_bit_width, lhs.m_sign};
    }

    return {(*lhs.m_ap.p) + (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

/// Add with wrapping.
inline MachineZNum operator+(const MachineZNum& lhs, const MachineZNum& rhs) {
    return add(lhs, rhs);
}

/// Sub with wrapping.
inline MachineZNum sub(const MachineZNum& lhs,
                       const MachineZNum& rhs,
                       bool& overflow) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        MachineZNum result(lhs.m_ap.i - rhs.m_ap.i,
                           lhs.m_bit_width,
                           lhs.m_sign);
        if (lhs.is_signed()) {
            overflow = lhs.is_non_negative() != rhs.is_non_negative() &&
                       result.is_non_negative() != lhs.is_non_negative();
        } else {
            overflow = result > lhs;
        }

        return result;
    }
    ZNum n = (*lhs.m_ap.p) - (*rhs.m_ap.p);
    overflow = MachineZNum::occurred_overflow(n, lhs.m_bit_width, lhs.m_sign) ||
               MachineZNum::occurred_underflow(n, lhs.m_bit_width, lhs.m_sign);

    return {n, lhs.m_bit_width, lhs.m_sign};
}

/// Sub with wrapping.
inline MachineZNum sub(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        return {lhs.m_ap.i - rhs.m_ap.i, lhs.m_bit_width, lhs.m_sign};
    }

    return {(*lhs.m_ap.p) - (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

/// Sub with wrapping.
inline MachineZNum operator-(const MachineZNum& lhs, const MachineZNum& rhs) {
    return sub(lhs, rhs);
}

/// Mul with wrapping.
inline MachineZNum mul(const MachineZNum& lhs,
                       const MachineZNum& rhs,
                       bool& overflow) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        MachineZNum result(lhs.m_ap.i * rhs.m_ap.i,
                           lhs.m_bit_width,
                           lhs.m_sign);
        if (lhs.get_bit_width() == 1) {
            if (lhs.is_signed()) {
                overflow = (lhs.is_minimum() && rhs.is_minimum());
            } else {
                overflow = false;
            }
        } else {
            overflow = !lhs.is_zero() && !rhs.is_zero() &&
                       (div(result, rhs) != lhs || div(result, lhs) != rhs);
        }
        return result;
    }

    ZNum n = (*lhs.m_ap.p) * (*rhs.m_ap.p);
    overflow = MachineZNum::occurred_overflow(n, lhs.m_bit_width, lhs.m_sign) ||
               MachineZNum::occurred_underflow(n, lhs.m_bit_width, lhs.m_sign);
    return {n, lhs.m_bit_width, lhs.m_sign};
}

/// Mul with wrapping.
inline MachineZNum mul(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        return {lhs.m_ap.i * rhs.m_ap.i, lhs.m_bit_width, lhs.m_sign};
    }

    return {(*lhs.m_ap.p) * (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

/// Mul with wrapping.
inline MachineZNum operator*(const MachineZNum& lhs, const MachineZNum& rhs) {
    return mul(lhs, rhs);
}

/// Div with wrapping.
inline MachineZNum div(const MachineZNum& lhs,
                       const MachineZNum& rhs,
                       bool& overflow,
                       bool& exact) {
    knight_assert_compatiblem_num(lhs, rhs);
    knight_assert_msg(!rhs.is_zero(), "division by zero");
    if (lhs.is_small_num()) {
        if (lhs.is_signed()) {
            MachineZNum abs_lhs = lhs.is_negative() ? -lhs : lhs;
            MachineZNum abs_rhs = rhs.is_negative() ? -rhs : rhs;
            MachineZNum result(abs_lhs.m_ap.i / abs_rhs.m_ap.i,
                               lhs.m_bit_width,
                               lhs.m_sign,
                               MachineZNum::NormalizedTag{});
            if (lhs.is_negative() ^ rhs.is_negative()) {
                result.negate();
            }
            overflow = lhs.is_minimum() && rhs.all_ones();
            exact = (abs_lhs.m_ap.i % abs_rhs.m_ap.i) == 0;
            return result;
        }

        MachineZNum result(lhs.m_ap.i / rhs.m_ap.i,
                           lhs.m_bit_width,
                           lhs.m_sign,
                           MachineZNum::NormalizedTag{});
        overflow = false;
        exact = (lhs.m_ap.i % rhs.m_ap.i) == 0;
        return result;
    }

    ZNum n = (*lhs.m_ap.p) / (*rhs.m_ap.p);
    overflow = MachineZNum::occurred_overflow(n, lhs.m_bit_width, lhs.m_sign) ||
               MachineZNum::occurred_underflow(n, lhs.m_bit_width, lhs.m_sign);
    exact = mod(*lhs.m_ap.p, *rhs.m_ap.p) == 0;
    return {n, lhs.m_bit_width, lhs.m_sign};
}

inline MachineZNum div(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    knight_assert_msg(!rhs.is_zero(), "division by zero");
    if (lhs.is_small_num()) {
        if (lhs.is_signed()) {
            MachineZNum abs_lhs = lhs.is_negative() ? -lhs : lhs;
            MachineZNum abs_rhs = rhs.is_negative() ? -rhs : rhs;
            MachineZNum result(abs_lhs.m_ap.i / abs_rhs.m_ap.i,
                               lhs.m_bit_width,
                               lhs.m_sign,
                               MachineZNum::NormalizedTag{});
            if (lhs.is_negative() ^ rhs.is_negative()) {
                result.negate();
            }
            return result;
        }

        return {lhs.m_ap.i / rhs.m_ap.i,
                lhs.m_bit_width,
                lhs.m_sign,
                MachineZNum::NormalizedTag{}};
    }

    return {(*lhs.m_ap.p) / (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

inline MachineZNum operator/(const MachineZNum& lhs, const MachineZNum& rhs) {
    return div(lhs, rhs);
}

inline MachineZNum rem(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    knight_assert_msg(!rhs.is_zero(), "division by zero");
    if (lhs.is_small_num()) {
        if (lhs.is_signed()) {
            // Signed remainder
            MachineZNum abs_lhs = lhs.is_negative() ? -lhs : lhs;
            MachineZNum abs_rhs = rhs.is_negative() ? -rhs : rhs;
            MachineZNum result(abs_lhs.m_ap.i % abs_rhs.m_ap.i,
                               lhs.m_bit_width,
                               lhs.m_sign,
                               MachineZNum::NormalizedTag{});
            if (lhs.is_negative()) {
                result.negate();
            }

            return result;
        }

        return {lhs.m_ap.i % rhs.m_ap.i,
                lhs.m_bit_width,
                lhs.m_sign,
                MachineZNum::NormalizedTag{}};
    }

    return {(*lhs.m_ap.p) % (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

inline MachineZNum operator%(const MachineZNum& lhs, const MachineZNum& rhs) {
    return rem(lhs, rhs);
}

inline MachineZNum mod(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    knight_assert_msg(!rhs.is_zero(), "division by zero");
    if (lhs.is_small_num()) {
        if (lhs.is_signed()) {
            // Signed modulo
            MachineZNum abs_lhs = lhs.is_negative() ? -lhs : lhs;
            MachineZNum abs_rhs = rhs.is_negative() ? -rhs : rhs;
            MachineZNum result(abs_lhs.m_ap.i % abs_rhs.m_ap.i,
                               lhs.m_bit_width,
                               lhs.m_sign,
                               MachineZNum::NormalizedTag{});
            if (lhs.is_negative() && !result.is_zero()) {
                result = abs_rhs - result;
            }
            return result;
        }

        return {lhs.m_ap.i % rhs.m_ap.i,
                lhs.m_bit_width,
                lhs.m_sign,
                MachineZNum::NormalizedTag{}};
    }

    return {mod(*lhs.m_ap.p, *rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

inline MachineZNum shl(const MachineZNum& lhs,
                       const MachineZNum& rhs,
                       bool& overflow) {
    knight_assert_compatiblem_num(lhs, rhs);
    knight_assert_shift(rhs, lhs.m_bit_width);
    if (lhs.is_small_num()) {
        MachineZNum result(lhs.m_ap.i << rhs.m_ap.i,
                           lhs.m_bit_width,
                           lhs.m_sign);
        if (lhs.is_signed()) {
            if (lhs.is_negative()) {
                overflow = (rhs.m_ap.i >= lhs.leading_ones());
            } else {
                overflow = (rhs.m_ap.i >= lhs.leading_zeros());
            }

        } else {
            overflow = (rhs.m_ap.i > lhs.leading_zeros());
        }

        return result;
    }

    MachineZNum result((*lhs.m_ap.p) << (*rhs.m_ap.p),
                       lhs.m_bit_width,
                       lhs.m_sign);
    if (lhs.is_signed()) {
        ZNum u = mod(*lhs.m_ap.p, MachineZNum::power_2(lhs.m_bit_width));
        ZNum shifted = u >> (lhs.m_bit_width - (*rhs.m_ap.p));
        if ((*result.m_ap.p) >= 0) { // resultant sign bit is 0
            overflow = (shifted != 0);
        } else { // resultant sign bit is 1
            overflow = (shifted != MachineZNum::power_2(*rhs.m_ap.p) - 1);
        }

    } else {
        overflow = ((*lhs.m_ap.p) >> (lhs.m_bit_width - (*rhs.m_ap.p))) != 0;
    }

    return result;
}

inline MachineZNum shl(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    knight_assert_shift(rhs, lhs.m_bit_width);
    if (lhs.is_small_num()) {
        return {lhs.m_ap.i << rhs.m_ap.i, lhs.m_bit_width, lhs.m_sign};
    }

    return {(*lhs.m_ap.p) << (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

inline MachineZNum lshr(const MachineZNum& lhs,
                        const MachineZNum& rhs,
                        bool& exact) {
    knight_assert_compatiblem_num(lhs, rhs);
    knight_assert_shift(rhs, lhs.m_bit_width);
    if (lhs.is_small_num()) {
        exact = (rhs.m_ap.i == 0) ||
                (lhs.m_ap.i & (MaxU64 >> (64 - rhs.m_ap.i))) == 0;
        return {lhs.m_ap.i >> rhs.m_ap.i,
                lhs.m_bit_width,
                lhs.m_sign,
                MachineZNum::NormalizedTag{}};
    }

    exact = ((*lhs.m_ap.p) & (MachineZNum::power_2(*rhs.m_ap.p) - 1)) == 0;
    if (lhs.is_signed()) {
        ZNum u = mod(*lhs.m_ap.p, MachineZNum::power_2(lhs.m_bit_width));
        return {u >> (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
    }

    return {(*lhs.m_ap.p) >> (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

inline MachineZNum lshr(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    knight_assert_shift(rhs, lhs.m_bit_width);
    if (lhs.is_small_num()) {
        return MachineZNum(lhs.m_ap.i >> rhs.m_ap.i,
                           lhs.m_bit_width,
                           lhs.m_sign,
                           MachineZNum::NormalizedTag{});
    }
    if (lhs.is_signed()) {
        ZNum u = mod(*lhs.m_ap.p, MachineZNum::power_2(lhs.m_bit_width));
        return {u >> (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
    }

    return {(*lhs.m_ap.p) >> (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

inline MachineZNum ashr(const MachineZNum& lhs,
                        const MachineZNum& rhs,
                        bool& exact) {
    knight_assert_compatiblem_num(lhs, rhs);
    knight_assert_shift(rhs, lhs.m_bit_width);

    if (lhs.is_small_num()) {
        exact =
            (rhs.m_ap.i == 0) ||
            (lhs.m_ap.i & (MaxU64 >> (internal::K64Bits - rhs.m_ap.i))) == 0;
        int64_t n = MachineZNum::sign_extend_64(lhs.m_ap.i, lhs.m_bit_width);

        // NOLINTNEXTLINE
        return {n >> static_cast< int64_t >(rhs.m_ap.i),
                lhs.m_bit_width,
                lhs.m_sign};
    }

    exact = ((*lhs.m_ap.p) & (MachineZNum::power_2(*rhs.m_ap.p) - 1)) == 0;
    if (lhs.is_signed()) {
        return {(*lhs.m_ap.p) >> (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
    }

    ZNum s = mod((*lhs.m_ap.p) + MachineZNum::power_2(lhs.m_bit_width - 1),
                 MachineZNum::power_2(lhs.m_bit_width)) -
             MachineZNum::power_2(lhs.m_bit_width - 1);
    return {s >> (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

inline MachineZNum ashr(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    knight_assert_shift(rhs, lhs.m_bit_width);
    if (lhs.is_small_num()) {
        int64_t n = MachineZNum::sign_extend_64(lhs.m_ap.i, lhs.m_bit_width);
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        return {n >> static_cast< int64_t >(rhs.m_ap.i),
                lhs.m_bit_width,
                lhs.m_sign};
    } else {
        if (lhs.is_signed()) {
            return {(*lhs.m_ap.p) >> (*rhs.m_ap.p),
                    lhs.m_bit_width,
                    lhs.m_sign};
        }

        ZNum s = mod((*lhs.m_ap.p) + MachineZNum::power_2(lhs.m_bit_width - 1),
                     MachineZNum::power_2(lhs.m_bit_width)) -
                 MachineZNum::power_2(lhs.m_bit_width - 1);
        return {s >> (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
    }
}

inline MachineZNum bit_and(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        return {lhs.m_ap.i & rhs.m_ap.i,
                lhs.m_bit_width,
                lhs.m_sign,
                MachineZNum::NormalizedTag{}};
    }

    return {(*lhs.m_ap.p) & (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

inline MachineZNum operator&(const MachineZNum& lhs, const MachineZNum& rhs) {
    return bit_and(lhs, rhs);
}

inline MachineZNum bit_or(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        return {lhs.m_ap.i | rhs.m_ap.i,
                lhs.m_bit_width,
                lhs.m_sign,
                MachineZNum::NormalizedTag{}};
    }

    return {(*lhs.m_ap.p) | (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

inline MachineZNum operator|(const MachineZNum& lhs, const MachineZNum& rhs) {
    return bit_or(lhs, rhs);
}

inline MachineZNum bit_xor(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        return MachineZNum(lhs.m_ap.i ^ rhs.m_ap.i,
                           lhs.m_bit_width,
                           lhs.m_sign,
                           MachineZNum::NormalizedTag{});
    }

    return {(*lhs.m_ap.p) ^ (*rhs.m_ap.p), lhs.m_bit_width, lhs.m_sign};
}

inline MachineZNum operator^(const MachineZNum& lhs, const MachineZNum& rhs) {
    return bit_xor(lhs, rhs);
}

inline MachineZNum gcd(const MachineZNum& a, const MachineZNum& b) {
    knight_assert_compatiblem_num(a, b);
    if (a.is_small_num()) {
        MachineZNum abs_a = a.is_negative() ? -a : a;
        MachineZNum abs_b = b.is_negative() ? -b : b;
        uint64_t u = abs_a.m_ap.i;
        uint64_t v = abs_b.m_ap.i;
        while (v != 0) {
            uint64_t r = u % v;
            u = v;
            v = r;
        }
        return {u, a.m_bit_width, b.m_sign};
    }

    return {gcd(*a.m_ap.p, *b.m_ap.p), a.m_bit_width, b.m_sign};
}

inline MachineZNum gcd(const MachineZNum& a,
                       const MachineZNum& b,
                       const MachineZNum& c) {
    return gcd(gcd(a, b), c);
}

inline bool operator==(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        return lhs.m_ap.i == rhs.m_ap.i;
    }

    return (*lhs.m_ap.p) == (*rhs.m_ap.p);
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator==(const MachineZNum& lhs, T rhs) {
    return lhs == MachineZNum(rhs, lhs.get_bit_width(), lhs.get_sign());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator==(T lhs, const MachineZNum& rhs) {
    return MachineZNum(lhs, rhs.get_bit_width(), rhs.get_sign()) == rhs;
}

inline bool operator!=(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        return lhs.m_ap.i != rhs.m_ap.i;
    } else {
        return (*lhs.m_ap.p) != (*rhs.m_ap.p);
    }
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator!=(const MachineZNum& lhs, T rhs) {
    return lhs != MachineZNum(rhs, lhs.get_bit_width(), lhs.get_sign());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator!=(T lhs, const MachineZNum& rhs) {
    return MachineZNum(lhs, rhs.get_bit_width(), rhs.get_sign()) != rhs;
}

inline bool operator<(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        if (lhs.is_signed()) {
            return MachineZNum::sign_extend_64(lhs.m_ap.i, lhs.m_bit_width) <
                   MachineZNum::sign_extend_64(rhs.m_ap.i, rhs.m_bit_width);
        } else {
            return lhs.m_ap.i < rhs.m_ap.i;
        }
    } else {
        return (*lhs.m_ap.p) < (*rhs.m_ap.p);
    }
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator<(const MachineZNum& lhs, T rhs) {
    return lhs < MachineZNum(rhs, lhs.get_bit_width(), lhs.get_sign());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator<(T lhs, const MachineZNum& rhs) {
    return MachineZNum(lhs, rhs.get_bit_width(), rhs.get_sign()) < rhs;
}

inline bool operator<=(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        if (lhs.is_signed()) {
            return MachineZNum::sign_extend_64(lhs.m_ap.i, lhs.m_bit_width) <=
                   MachineZNum::sign_extend_64(rhs.m_ap.i, rhs.m_bit_width);
        }

        return lhs.m_ap.i <= rhs.m_ap.i;
    }

    return (*lhs.m_ap.p) <= (*rhs.m_ap.p);
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator<=(const MachineZNum& lhs, T rhs) {
    return lhs <= MachineZNum(rhs, lhs.get_bit_width(), lhs.get_sign());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator<=(T lhs, const MachineZNum& rhs) {
    return MachineZNum(lhs, rhs.get_bit_width(), rhs.get_sign()) <= rhs;
}

inline bool operator>(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        if (lhs.is_signed()) {
            return MachineZNum::sign_extend_64(lhs.m_ap.i, lhs.m_bit_width) >
                   MachineZNum::sign_extend_64(rhs.m_ap.i, rhs.m_bit_width);
        }

        return lhs.m_ap.i > rhs.m_ap.i;
    }

    return (*lhs.m_ap.p) > (*rhs.m_ap.p);
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator>(const MachineZNum& lhs, T rhs) {
    return lhs > MachineZNum(rhs, lhs.get_bit_width(), lhs.get_sign());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator>(T lhs, const MachineZNum& rhs) {
    return MachineZNum(lhs, rhs.get_bit_width(), rhs.get_sign()) > rhs;
}

inline bool operator>=(const MachineZNum& lhs, const MachineZNum& rhs) {
    knight_assert_compatiblem_num(lhs, rhs);
    if (lhs.is_small_num()) {
        if (lhs.is_signed()) {
            return MachineZNum::sign_extend_64(lhs.m_ap.i, lhs.m_bit_width) >=
                   MachineZNum::sign_extend_64(rhs.m_ap.i, rhs.m_bit_width);
        }

        return lhs.m_ap.i >= rhs.m_ap.i;
    }

    return (*lhs.m_ap.p) >= (*rhs.m_ap.p);
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator>=(const MachineZNum& lhs, T rhs) {
    return lhs >= MachineZNum(rhs, lhs.get_bit_width(), lhs.get_sign());
}

template < typename T,
           class = std::enable_if_t< std::is_integral< T >::value > >
inline bool operator>=(T lhs, const MachineZNum& rhs) {
    return MachineZNum(lhs, rhs.get_bit_width(), rhs.get_sign()) >= rhs;
}

inline const MachineZNum& min(const MachineZNum& a, const MachineZNum& b) {
    return (a < b) ? a : b;
}

inline const MachineZNum& max(const MachineZNum& a, const MachineZNum& b) {
    return (a < b) ? b : a;
}

inline MachineZNum abs(const MachineZNum& n) {
    return n.is_negative() ? -n : n;
}

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                     const MachineZNum& n) {
    if (n.is_small_num()) {
        if (n.is_signed()) {
            os << MachineZNum::sign_extend_64(n.m_ap.i, n.m_bit_width);
        } else {
            os << n.m_ap.i;
        }
    } else {
        os << *n.m_ap.p;
    }
    return os;
}

inline std::size_t hash_value(const MachineZNum& n) {
    std::size_t hash = 0;

    if (n.is_small_num()) {
        hash_combine(hash, std::hash< int >()(n.m_ap.i));
    } else {
        hash_combine(hash, std::hash< ZNum >()(*n.m_ap.p));
    }

    hash_combine(hash, std::hash< int >()(n.m_bit_width));
    hash_combine(hash, std::hash< int >()(n.m_sign));

    return hash;
}

} // namespace knight::analyzer

namespace std {

template <>
struct hash< knight::analyzer::MachineZNum > {
    std::size_t operator()(const knight::analyzer::MachineZNum& n) const {
        return knight::analyzer::hash_value(n);
    }
};

} // namespace std
