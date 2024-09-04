//===- dom_base.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the base for all domains.
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Registry.h>

#include "dfa/domain/domains.hpp"
#include "support/dom.hpp"
#include "util/log.hpp"

#include <memory>
#include <unordered_set>

namespace knight::dfa {

class AbsDomBase;

using DomIDs = std::unordered_set< DomID >;
using AbsValRef = AbsDomBase*;
using AbsValConstRef = const AbsDomBase*;
using SharedVal = std::shared_ptr< AbsDomBase >;

/// \brief Base for all abstract domains
///
/// Abstract domain should be thread-safe on copy and `const` methods
struct AbsDomBase {
    DomainKind kind;
    bool is_numerical;

    /// \brief Create the `Top` abstract value
    explicit AbsDomBase(DomainKind k, bool is_numerical = false)
        : kind(k), is_numerical(is_numerical) {}

    virtual ~AbsDomBase() = default;

    /// \brief Clone the abstract value
    [[nodiscard]] virtual AbsDomBase* clone() const = 0;

    [[nodiscard]] SharedVal clone_shared() const { return SharedVal(clone()); }

    /// \brief Normalize the abstract value
    ///
    /// It shall be called before state check operation,
    /// Default impl is do nothing.
    virtual void normalize() {}

    /// \brief Check if the abstract value is bottom
    [[nodiscard]] virtual bool is_bottom() const = 0;

    /// \brief Check if the abstract value is top
    [[nodiscard]] virtual bool is_top() const = 0;

    /// \brief Set the abstract value to bottom
    virtual void set_to_bottom() = 0;

    /// \brief Set the abstract value to top
    virtual void set_to_top() = 0;

    /// \brief Join with another abstract value
    ///
    /// any time, at loop head, between consecutive iterations
    virtual void join_with(const AbsDomBase& other) = 0;
    virtual void join_with_at_loop_head(const AbsDomBase& other) {
        this->join_with(other);
    }
    virtual void join_consecutive_iter_with(const AbsDomBase& other) {
        this->join_with(other);
    }

    /// \brief Widen with another abstract value
    ///
    /// default impl is equivalent to `join_with`
    virtual void widen_with(const AbsDomBase& other) { this->join_with(other); }

    /// \brief Meet with another abstract value
    ///
    /// default impl is do nothing
    virtual void meet_with(const AbsDomBase& other) {}

    /// \brief Narrow with another abstract value
    ///
    /// default impl is equivalent to `meet_with`
    virtual void narrow_with(const AbsDomBase& other) {
        this->meet_with(other);
    }

    /// \brief Check the inclusion relation
    [[nodiscard]] virtual bool leq(const AbsDomBase& other) const = 0;

    /// \brief Equality comparison
    [[nodiscard]] virtual bool equals(const AbsDomBase& other) const {
        return leq(other) && other.leq(*this);
    }

    /// \brief Equality comparison operator
    [[nodiscard]] bool operator==(const AbsDomBase& other) const {
        return this->equals(other);
    }

    /// \brief Inequality comparison operator
    [[nodiscard]] bool operator!=(const AbsDomBase& other) const {
        return !this->equals(other);
    }

    /// \brief dump abstract value for debugging
    ///
    /// default impl is dump nothing
    virtual void dump(llvm::raw_ostream& os) const {}

} __attribute__((aligned(8))) __attribute__((packed)); // class AbsDomBase

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                     const AbsDomBase& dom) {
    dom.dump(os);
    return os;
}

using DomainDefaultValFn = std::function< SharedVal() >;
using DomainBottomValFn = std::function< SharedVal() >;

extern std::unordered_map< DomainKind, DomainDefaultValFn >
    DefaultValFns; // NOLINT
extern std::unordered_map< DomainKind, DomainBottomValFn >
    BottomValFns; // NOLINT

template < typename Dom >
struct DomainRegistry {
    DomainRegistry() {
        DefaultValFns[Dom::get_kind()] = Dom::default_val;
        BottomValFns[Dom::get_kind()] = Dom::bottom_val;
    }
};

[[nodiscard]] inline std::optional< DomainDefaultValFn >
get_domain_default_val_fn(DomID id) {
    auto it = DefaultValFns.find(get_domain_kind(id));
    if (it == DefaultValFns.end()) {
        return std::nullopt;
    }
    return it->second;
}

[[nodiscard]] inline std::optional< DomainBottomValFn >
get_domain_bottom_val_fn(DomID id) {
    auto it = BottomValFns.find(get_domain_kind(id));
    if (it == BottomValFns.end()) {
        return std::nullopt;
    }
    return it->second;
}

/// \brief Base wrapper class for all domains
///
/// `Derived` domain *requires* the following methods:
/// - `static get_kind()`
/// - `static UniqueVal default_val()` which returns the default abstract value
/// - `static UniqueVal bottom_val()` which returns the bottom abstract value
/// - `clone() const`
/// - `is_bottom() const`
/// - `is_top() const`
/// - `set_to_bottom()`
/// - `set_to_top()`
/// - `join_with(const Derived& other)`
/// - `leq(const Derived& other) const`
///
/// `Derived` domain may also implement the following *optional* methods:
/// - `normalize()`
/// - `join_with_at_loop_head(const Derived& other)`
/// - `join_consecutive_iter_with(const Derived& other)`
/// - `widen_with(const Derived& other)`
/// - `meet_with(const Derived& other)`
/// - `narrow_with(const Derived& other)`
/// - `equals(const Derived& other) const`
/// - `dump(llvm::Derived& os) const`
template < typename Derived >
class AbsDom : public AbsDomBase {
  public:
    AbsDom() : AbsDomBase(Derived::get_kind()) {}

    void join_with(const AbsDomBase& other) override {
        static_assert(does_derived_dom_can_join_with< Derived >::value,
                      "derived domain needs to implement `join_with` method");
        static_cast< Derived* >(this)->join_with(
            static_cast< const Derived& >(other));
    }

    void join_with_at_loop_head(const AbsDomBase& other) override {
        if constexpr (does_derived_dom_can_join_with_at_loop_head<
                          Derived >::value) {
            static_cast< Derived* >(this)->join_with_at_loop_head(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::join_with_at_loop_head(other);
        }
    }

    void join_consecutive_iter_with(const AbsDomBase& other) override {
        if constexpr (does_derived_dom_can_join_consecutive_iter_with<
                          Derived >::value) {
            static_cast< Derived* >(this)->join_consecutive_iter_with(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::join_consecutive_iter_with(other);
        }
    }

    void widen_with(const AbsDomBase& other) override {
        if constexpr (does_derived_dom_can_widen_with< Derived >::value) {
            static_cast< Derived* >(this)->widen_with(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::widen_with(other);
        }
    }

    void meet_with(const AbsDomBase& other) override {
        if constexpr (does_derived_dom_can_meet_with< Derived >::value) {
            static_cast< Derived* >(this)->meet_with(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::meet_with(other);
        }
    }

    void narrow_with(const AbsDomBase& other) override {
        if constexpr (does_derived_dom_can_narrow_with< Derived >::value) {
            static_cast< Derived* >(this)->narrow_with(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::narrow_with(other);
        }
    }

    [[nodiscard]] virtual bool leq(const AbsDomBase& other) const override {
        static_assert(does_derived_dom_can_leq< Derived >::value,
                      "derived domain needs to implement `leq` method");
        return static_cast< const Derived& >(other).leq(
            static_cast< const Derived& >(*this));
    }

    [[nodiscard]] virtual bool equals(const AbsDomBase& other) const override {
        if constexpr (does_derived_dom_can_equals< Derived >::value) {
            return static_cast< const Derived& >(other).equals(
                static_cast< const Derived& >(*this));
        } else {
            return AbsDomBase::equals(other);
        }
    }

}; // class AbsDom

template < typename DerivedDom >
struct isa_abs_dom
    : std::bool_constant<
          std::is_base_of_v< AbsDom< DerivedDom >, DerivedDom > > {};

template < typename DerivedDom >
concept derived_dom = isa_abs_dom< DerivedDom >::value;

} // namespace knight::dfa
