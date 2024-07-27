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

#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/StringRef.h>

#include "dfa/domain/domains.hpp"
#include "support/dom.hpp"

#include <unordered_set>

namespace knight::dfa {

using DomID = uint8_t;

/// \brief Base for all abstract domains
///
/// Abstract domain should be thread-safe on copy and `const` methods
class AbsDomBase {
  public:
    /// \brief Create the top abstract value
    AbsDomBase() = default;

    /// \brief Get the kind of the domain
    virtual DomainKind get_kind() const = 0;

    /// \brief Normalize the abstract value
    ///
    /// It shall be called before state check operation,
    /// Default impl is do nothing.
    virtual void normalize() {}

    /// \brief Check if the abstract value is bottom
    virtual bool is_bottom() const = 0;

    /// \brief Check if the abstract value is top
    virtual bool is_top() const = 0;

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
    virtual bool leq(const AbsDomBase& other) const = 0;

    /// \brief Equality comparison
    virtual bool equals(const AbsDomBase& other) const {
        return leq(other) && other.leq(*this);
    }

    /// \brief Equality comparison operator
    bool operator==(const AbsDomBase& other) const {
        return this->equals(other);
    }

    /// \brief Inequality comparison operator
    bool operator!=(const AbsDomBase& other) const {
        return !this->equals(other);
    }

    /// \brief dump abstract value for debugging
    ///
    /// default impl is dump nothing
    virtual void dump(llvm::raw_ostream& os) const {}
}; // class AbsDomBase

using DomIDs = std::unordered_set< DomID >;
using DomRef = AbsDomBase*;
using DomSharedRef = std::shared_ptr< AbsDomBase >;

/// \brief Base wrapper class for all domains
///
/// Derived domain shall be final and requires the following methods:
/// - `get_kind() const`
/// - `is_bottom() const`
/// - `is_top() const`
/// - `set_to_bottom()`
/// - `set_to_top()`
/// - `join_with(const Derived& other)`
/// - `leq(const Derived& other) const`
/// Derived domain can also implement the following optional methods:
/// - `normalize()` [optional]
/// - `join_with_at_loop_head(const Derived& other)` [optional]
/// - `join_consecutive_iter_with(const Derived& other)` [optional]
/// - `widen_with(const Derived& other)` [optional]
/// - `meet_with(const Derived& other)` [optional]
/// - `narrow_with(const Derived& other)` [optional]
/// - `equals(const Derived& other) const` [optional]
/// - `dump(llvm::Derived& os) const` [optional]
template < typename Derived > class AbsDom : public AbsDomBase {
  public:
    void join_with(const AbsDomBase& other) override {
        knight_assert_msg(get_kind() == other.get_kind(),
                          "Incompatible domains");
        static_assert(does_derived_dom_can_join_with< Derived >::value,
                      "derived domain needs to implement `join_with` method");
        static_cast< Derived* >(this)->join_with(
            static_cast< const Derived& >(other));
    }

    void join_with_at_loop_head(const AbsDomBase& other) override {
        knight_assert_msg(get_kind() == other.get_kind(),
                          "Incompatible domains");
        if constexpr (does_derived_dom_can_join_with_at_loop_head<
                          Derived >::value) {
            static_cast< Derived* >(this)->join_with_at_loop_head(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::join_with_at_loop_head(other);
        }
    }

    void join_consecutive_iter_with(const AbsDomBase& other) override {
        knight_assert_msg(get_kind() == other.get_kind(),
                          "Incompatible domains");
        if constexpr (does_derived_dom_can_join_consecutive_iter_with<
                          Derived >::value) {
            static_cast< Derived* >(this)->join_consecutive_iter_with(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::join_consecutive_iter_with(other);
        }
    }

    void widen_with(const AbsDomBase& other) override {
        knight_assert_msg(get_kind() == other.get_kind(),
                          "Incompatible domains");
        if constexpr (does_derived_dom_can_widen_with< Derived >::value) {
            static_cast< Derived* >(this)->widen_with(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::widen_with(other);
        }
    }

    void meet_with(const AbsDomBase& other) override {
        knight_assert_msg(get_kind() == other.get_kind(),
                          "Incompatible domains");
        if constexpr (does_derived_dom_can_meet_with< Derived >::value) {
            static_cast< Derived* >(this)->meet_with(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::meet_with(other);
        }
    }

    void narrow_with(const AbsDomBase& other) override {
        knight_assert_msg(get_kind() == other.get_kind(),
                          "Incompatible domains");
        if constexpr (does_derived_dom_can_narrow_with< Derived >::value) {
            static_cast< Derived* >(this)->narrow_with(
                static_cast< const Derived& >(other));
        } else {
            AbsDomBase::narrow_with(other);
        }
    }

    virtual bool leq(const AbsDomBase& other) const override {
        knight_assert_msg(get_kind() == other.get_kind(),
                          "Incompatible domains");
        static_assert(does_derived_dom_can_leq< Derived >::value,
                      "derived domain needs to implement `leq` method");
        return static_cast< const Derived& >(other).leq(
            static_cast< const Derived& >(*this));
    }

    virtual bool equals(const AbsDomBase& other) const override {
        knight_assert_msg(get_kind() == other.get_kind(),
                          "Incompatible domains");
        if constexpr (does_derived_dom_can_equals< Derived >::value) {
            return static_cast< const Derived& >(other).equals(
                static_cast< const Derived& >(*this));
        } else {
            return AbsDomBase::equals(other);
        }
    }

}; // class AbsDom

} // namespace knight::dfa
