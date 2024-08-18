//===- qnum.hpp -------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the QNum class, which is a wrapper around GMP's
//  mpz_class
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/ADT/FoldingSet.h>
#include <llvm/Support/raw_ostream.h>

#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <gmpxx.h>

#include "util/assert.hpp"
#include "znum.hpp"

namespace knight::dfa {

template < typename T >
struct ZNumOrIntegral : public std::is_integral< T > {};

template <>
struct ZNumOrIntegral< const ZNum& > : public std::true_type {};

namespace internal {

template < typename T >
struct ZNumTransformer : public MpzTransformer< T > {};

template <>
struct ZNumTransformer< const ZNum& > {
    const mpz_class& operator()(const ZNum& n) { return n.get_mpz(); }
};

} // namespace internal

/// \brief Class for unlimited precision rationals
class QNum : public llvm::FoldingSetNode {
  private:
    mpq_class m_mpq;

  public:
    [[nodiscard]] static std::optional< QNum > from_string(
        const std::string& str, int base = internal::K10Base) {
        try {
            return QNum(mpq_class(str, base));
        } catch (std::invalid_argument&) {
            llvm::errs() << "QNum: invalid input string '" << str << "'\n";
            return std::nullopt;
        }
    }

    [[nodiscard]] const mpq_class& get_mpq() const { return this->m_mpq; }

    QNum() = default;
    explicit QNum(const ZNum& n) : m_mpq(n.m_mpz) {}
    explicit QNum(ZNum&& n) : m_mpq(std::move(n).m_mpz) {}

    template < typename N,
               class = std::enable_if_t< std::is_integral< N >::value > >
    explicit QNum(N n) : m_mpq(internal::MpzTransformer< N >()(n)) {}

    explicit QNum(const mpq_class& n) : m_mpq(n) {
        knight_assert_msg(this->m_mpq.get_den() != 0, "denominator is zero");
        this->m_mpq.canonicalize();
    }

    explicit QNum(mpq_class&& n) : m_mpq(std::move(n)) {
        knight_assert_msg(this->m_mpq.get_den() != 0, "denominator is zero");
        this->m_mpq.canonicalize();
    }

    /// \brief Create a QNum from a numerator and a denominator
    template < typename N,
               typename D,
               class = std::enable_if_t< std::is_integral< N >::value &&
                                         std::is_integral< D >::value > >
    explicit QNum(N n, D d)
        : m_mpq(internal::MpzTransformer< N >()(n),
                internal::MpzTransformer< D >()(d)) {
        knight_assert_msg(this->m_mpq.get_den() != 0, "denominator is zero");
        this->m_mpq.canonicalize();
    }

    /// \brief Create a QNum from a numerator and a denominator
    explicit QNum(const ZNum& n, const ZNum& d) : m_mpq(n.m_mpz, d.m_mpz) {
        knight_assert_msg(this->m_mpq.get_den() != 0, "denominator is zero");
        this->m_mpq.canonicalize();
    }

    /// \brief Create a QNum from a numerator and a denominator
    explicit QNum(ZNum&& n, ZNum&& d)
        : m_mpq(std::move(n).m_mpz, std::move(d).m_mpz) {
        knight_assert_msg(this->m_mpq.get_den() != 0, "denominator is zero");
        this->m_mpq.canonicalize();
    }

    /// \brief Create a QNum from a normalized mpq_class
    struct NormalizedTag {};
    QNum(const mpq_class& n, NormalizedTag) : m_mpq(n) {}       // NOLINT
    QNum(mpq_class&& n, NormalizedTag) : m_mpq(std::move(n)) {} // NOLINT

    QNum(const QNum&) = default;
    QNum(QNum&&) = default;
    QNum& operator=(const QNum&) = default;
    QNum& operator=(QNum&&) noexcept = default;
    ~QNum() = default;

    QNum& operator=(const ZNum& n) {
        this->m_mpq = n.m_mpz;
        return *this;
    }

    QNum& operator=(ZNum&& n) noexcept {
        this->m_mpq = std::move(n.m_mpz);
        return *this;
    }

    template < typename N,
               typename = std::enable_if_t< std::is_integral< N >::value > >
    QNum& operator=(N n) {
        this->m_mpq = internal::MpzTransformer< N >()(n);
        return *this;
    }

    /// NOLINTNEXTLINE
    void Profile(llvm::FoldingSetNodeID& ID) const {
        const mpz_class& num = this->m_mpq.get_num();
        const mpz_class& den = this->m_mpq.get_den();

        // Add the size of the numerator mpz_t
        ID.AddInteger(num.get_mpz_t()[0]._mp_size);
        // Add each limb of the numerator mpz_t
        for (int i = 0, e = std::abs(num.get_mpz_t()[0]._mp_size); i < e; ++i) {
            ID.AddInteger(num.get_mpz_t()[0]._mp_d[i]); // NOLINT
        }

        // Add the size of the denominator mpz_t
        ID.AddInteger(den.get_mpz_t()[0]._mp_size);
        // Add each limb of the denominator mpz_t
        for (int i = 0, e = std::abs(den.get_mpz_t()[0]._mp_size); i < e; ++i) {
            ID.AddInteger(den.get_mpz_t()[0]._mp_d[i]); // NOLINT
        }
    }

    QNum& operator+=(const QNum& x) {
        this->m_mpq += x.m_mpq;
        return *this;
    }

    template < typename N,
               class = std::enable_if_t< ZNumOrIntegral< N >::value > >
    QNum& operator+=(N x) {
        this->m_mpq += internal::ZNumTransformer< N >()(x);
        return *this;
    }

    QNum& operator-=(const QNum& x) {
        this->m_mpq -= x.m_mpq;
        return *this;
    }

    template < typename N,
               class = std::enable_if_t< ZNumOrIntegral< N >::value > >
    QNum& operator-=(N x) {
        this->m_mpq -= internal::ZNumTransformer< N >()(x);
        return *this;
    }

    QNum& operator*=(const QNum& x) {
        this->m_mpq *= x.m_mpq;
        return *this;
    }

    template < typename N,
               class = std::enable_if_t< ZNumOrIntegral< N >::value > >
    QNum& operator*=(N x) {
        this->m_mpq *= internal::ZNumTransformer< N >()(x);
        return *this;
    }

    QNum& operator/=(const QNum& x) {
        knight_assert_msg(x.m_mpq != 0, "divided by zero");
        this->m_mpq /= x.m_mpq;
        return *this;
    }

    template < typename N,
               class = std::enable_if_t< ZNumOrIntegral< N >::value > >
    QNum& operator/=(N x) {
        knight_assert_msg(x != 0, "divided by zero");
        this->m_mpq /= internal::ZNumTransformer< N >()(x);
        return *this;
    }

    /// \brief Unary plus
    const QNum& operator+() const { return *this; }

    /// \brief Prefix increment
    QNum& operator++() {
        ++this->m_mpq;
        return *this;
    }

    /// \brief Postfix increment
    const QNum operator++(int) { // NOLINT
        QNum r(*this);
        ++this->m_mpq;
        return r;
    }

    /// \brief Unary minus
    const QNum operator-() const { return QNum(-this->m_mpq); } // NOLINT

    /// \brief Prefix decrement
    QNum& operator--() {
        --this->m_mpq;
        return *this;
    }

    /// \brief Postfix decrement
    const QNum operator--(int) { // NOLINT
        QNum r(*this);
        --this->m_mpq;
        return r;
    }

    [[nodiscard]] ZNum numerator() const { return ZNum(this->m_mpq.get_num()); }
    [[nodiscard]] ZNum denominator() const {
        return ZNum(this->m_mpq.get_den());
    }

    [[nodiscard]] ZNum round_to_upper() const {
        const mpz_class& num = this->m_mpq.get_num();
        const mpz_class& den = this->m_mpq.get_den();
        ZNum q(num / den);
        ZNum r(num % den);
        return (r == 0 || this->m_mpq < 0) ? q : q + 1;
    }

    [[nodiscard]] ZNum round_to_lower() const {
        const mpz_class& num = this->m_mpq.get_num();
        const mpz_class& den = this->m_mpq.get_den();
        ZNum q(num / den);
        ZNum r(num % den);
        return (r == 0 || this->m_mpq > 0) ? q : q - 1;
    }

    [[nodiscard]] std::string str(int base = internal::K10Base) const {
        return this->m_mpq.get_str(base);
    }

}; // end class QNum

[[nodiscard]] inline QNum operator+(const QNum& lhs, const QNum& rhs) {
    return QNum(lhs.get_mpq() + rhs.get_mpq(), QNum::NormalizedTag{});
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline QNum operator+(const QNum& lhs, T rhs) {
    return QNum(lhs.get_mpq() + internal::ZNumTransformer< T >()(rhs),
                QNum::NormalizedTag{});
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline QNum operator+(T lhs, const QNum& rhs) {
    return QNum(internal::ZNumTransformer< T >()(lhs) + rhs.get_mpq(),
                QNum::NormalizedTag{});
}

[[nodiscard]] inline QNum operator-(const QNum& lhs, const QNum& rhs) {
    return QNum(lhs.get_mpq() - rhs.get_mpq(), QNum::NormalizedTag{});
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline QNum operator-(const QNum& lhs, T rhs) {
    return QNum(lhs.get_mpq() - internal::ZNumTransformer< T >()(rhs),
                QNum::NormalizedTag{});
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline QNum operator-(T lhs, const QNum& rhs) {
    return QNum(internal::ZNumTransformer< T >()(lhs) - rhs.get_mpq(),
                QNum::NormalizedTag{});
}

[[nodiscard]] inline QNum operator*(const QNum& lhs, const QNum& rhs) {
    return QNum(lhs.get_mpq() * rhs.get_mpq(), QNum::NormalizedTag{});
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline QNum operator*(const QNum& lhs, T rhs) {
    return QNum(lhs.get_mpq() * internal::ZNumTransformer< T >()(rhs),
                QNum::NormalizedTag{});
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline QNum operator*(T lhs, const QNum& rhs) {
    return QNum(internal::ZNumTransformer< T >()(lhs) * rhs.get_mpq(),
                QNum::NormalizedTag{});
}

[[nodiscard]] inline QNum operator/(const QNum& lhs, const QNum& rhs) {
    knight_assert_msg(rhs.get_mpq() != 0, "divided by zero");
    return QNum(lhs.get_mpq() / rhs.get_mpq(), QNum::NormalizedTag{});
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline QNum operator/(const QNum& lhs, T rhs) {
    knight_assert_msg(rhs != 0, "divided by zero");
    return QNum(lhs.get_mpq() / internal::ZNumTransformer< T >()(rhs),
                QNum::NormalizedTag{});
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline QNum operator/(T lhs, const QNum& rhs) {
    knight_assert_msg(rhs.get_mpq() != 0, "divided by zero");
    return QNum(internal::ZNumTransformer< T >()(lhs) / rhs.get_mpq(),
                QNum::NormalizedTag{});
}

[[nodiscard]] inline bool operator==(const QNum& lhs, const QNum& rhs) {
    return lhs.get_mpq() == rhs.get_mpq();
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline bool operator==(const QNum& lhs, T rhs) {
    return lhs.get_mpq() == internal::ZNumTransformer< T >()(rhs);
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline bool operator==(T lhs, const QNum& rhs) {
    return internal::ZNumTransformer< T >()(lhs) == rhs.get_mpq();
}

[[nodiscard]] inline bool operator!=(const QNum& lhs, const QNum& rhs) {
    return lhs.get_mpq() != rhs.get_mpq();
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline bool operator!=(const QNum& lhs, T rhs) {
    return lhs.get_mpq() != internal::ZNumTransformer< T >()(rhs);
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline bool operator!=(T lhs, const QNum& rhs) {
    return internal::ZNumTransformer< T >()(lhs) != rhs.get_mpq();
}

[[nodiscard]] inline bool operator<(const QNum& lhs, const QNum& rhs) {
    return lhs.get_mpq() < rhs.get_mpq();
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline bool operator<(const QNum& lhs, T rhs) {
    return lhs.get_mpq() < internal::ZNumTransformer< T >()(rhs);
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline bool operator<(T lhs, const QNum& rhs) {
    return internal::ZNumTransformer< T >()(lhs) < rhs.get_mpq();
}

[[nodiscard]] inline bool operator<=(const QNum& lhs, const QNum& rhs) {
    return lhs.get_mpq() <= rhs.get_mpq();
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline bool operator<=(const QNum& lhs, T rhs) {
    return lhs.get_mpq() <= internal::ZNumTransformer< T >()(rhs);
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline bool operator<=(T lhs, const QNum& rhs) {
    return internal::ZNumTransformer< T >()(lhs) <= rhs.get_mpq();
}

[[nodiscard]] inline bool operator>(const QNum& lhs, const QNum& rhs) {
    return lhs.get_mpq() > rhs.get_mpq();
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline bool operator>(const QNum& lhs, T rhs) {
    return lhs.get_mpq() > internal::ZNumTransformer< T >()(rhs);
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline bool operator>(T lhs, const QNum& rhs) {
    return internal::ZNumTransformer< T >()(lhs) > rhs.get_mpq();
}

[[nodiscard]] inline bool operator>=(const QNum& lhs, const QNum& rhs) {
    return lhs.get_mpq() >= rhs.get_mpq();
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline bool operator>=(const QNum& lhs, T rhs) {
    return lhs.get_mpq() >= internal::ZNumTransformer< T >()(rhs);
}

template < typename T, class = std::enable_if_t< ZNumOrIntegral< T >::value > >
[[nodiscard]] inline bool operator>=(T lhs, const QNum& rhs) {
    return internal::ZNumTransformer< T >()(lhs) >= rhs.get_mpq();
}

[[nodiscard]] inline const QNum& min(const QNum& a, const QNum& b) {
    return (a < b) ? a : b;
}

[[nodiscard]] inline const QNum& min(const QNum& a,
                                     const QNum& b,
                                     const QNum& c) {
    return min(min(a, b), c);
}

[[nodiscard]] inline const QNum& min(const QNum& a,
                                     const QNum& b,
                                     const QNum& c,
                                     const QNum& d) {
    return min(min(min(a, b), c), d);
}

[[nodiscard]] inline const QNum& max(const QNum& a, const QNum& b) {
    return (a < b) ? b : a;
}

[[nodiscard]] inline const QNum& max(const QNum& a,
                                     const QNum& b,
                                     const QNum& c) {
    return max(max(a, b), c);
}

[[nodiscard]] inline const QNum& max(const QNum& a,
                                     const QNum& b,
                                     const QNum& c,
                                     const QNum& d) {
    return max(max(max(a, b), c), d);
}

[[nodiscard]] inline QNum abs(const QNum& n) {
    return QNum(abs(n.get_mpq()));
}

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const QNum& n) {
    return os << n.get_mpq().get_str();
}

[[nodiscard]] inline std::size_t hash_value(const QNum& n) {
    const mpq_class& m = n.get_mpq();
    std::size_t result = 0;

    // Hash the numerator
    hash_combine(result, std::hash< int >()(m.get_num_mpz_t()[0]._mp_size));
    for (int i = 0, e = std::abs(m.get_num_mpz_t()[0]._mp_size); i < e; ++i) {
        hash_combine(result,
                     std::hash< mp_limb_t >()(
                         m.get_num_mpz_t()[0]._mp_d[i])); // NOLINT
    }

    // Hash the denominator
    hash_combine(result, std::hash< int >()(m.get_den_mpz_t()[0]._mp_size));
    for (int i = 0, e = std::abs(m.get_den_mpz_t()[0]._mp_size); i < e; ++i) {
        hash_combine(result,
                     std::hash< mp_limb_t >()(
                         m.get_den_mpz_t()[0]._mp_d[i])); // NOLINT
    }

    return result;
}

} // end namespace knight::dfa

namespace std {

template <>
struct hash< knight::dfa::QNum > {
    std::size_t operator()(const knight::dfa::QNum& n) const {
        return knight::dfa::hash_value(n);
    }
};

} // end namespace std
