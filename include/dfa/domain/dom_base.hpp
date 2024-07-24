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

namespace knight::dfa {

using DomID = uint8_t;

/// \brief Base for all abstract domains
///
/// Abstract domain should be thread-safe on copy and `const` methods
class AbsDomBase {
  public:
    /// \brief Create the top abstract value
    AbsDomBase() = default;

    /// \brief the unique name of the domain
    virtual llvm::StringRef get_name() const = 0;

    /// \brief the unique id of the domain
    virtual DomID get_id() const = 0;

    /// \brief Normalize the abstract value
    ///
    /// It shall be called before state check operation
    virtual void normalize() = 0;

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
    virtual void widen_with(const AbsDomBase& other) = 0;

    /// \brief Meet with another abstract value
    virtual void meet_with(const AbsDomBase& other) = 0;

    /// \brief Narrow with another abstract value
    virtual void narrow_with(const AbsDomBase& other) = 0;

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
    virtual void dump(llvm::raw_ostream& os) const = 0;
}; // class AbsDomBase

/// \brief Base wrapper class for all domains
///
/// Derived domain shall be final and requires the following methods:
/// - `get_name() const`
/// - `get_id() const`
/// - `normalize()`
/// - `is_bottom() const`
/// - `is_top() const`
/// - `set_to_bottom()`
/// - `set_to_top()`
/// - `join_with(const Derived& other)`
/// - `join_with_at_loop_head(const Derived& other)`
/// - `join_consecutive_iter_with(const Derived& other)`
/// - `widen_with(const Derived& other)`
/// - `meet_with(const Derived& other)`
/// - `narrow_with(const Derived& other)`
/// - `leq(const Derived& other) const`
/// - `equals(const Derived& other) const`
/// - `dump(llvm::Derived& os) const`
template < typename Derived > class AbsDom : public AbsDomBase {
  public:
    void join_with(const AbsDomBase& other) override {
        static_cast< Derived* >(this)->join_with(
            static_cast< const Derived& >(other));
    }

    void join_with_at_loop_head(const AbsDomBase& other) override {
        static_cast< Derived* >(this)->join_with_at_loop_head(
            static_cast< const Derived& >(other));
    }

    void join_consecutive_iter_with(const AbsDomBase& other) override {
        static_cast< Derived* >(this)->join_consecutive_iter_with(
            static_cast< const Derived& >(other));
    }

    void widen_with(const AbsDomBase& other) override {
        static_cast< Derived* >(this)->widen_with(
            static_cast< const Derived& >(other));
    }

    void meet_with(const AbsDomBase& other) override {
        static_cast< Derived* >(this)->meet_with(
            static_cast< const Derived& >(other));
    }

    void narrow_with(const AbsDomBase& other) override {
        static_cast< Derived* >(this)->narrow_with(
            static_cast< const Derived& >(other));
    }

    virtual bool leq(const AbsDomBase& other) const override {
        return static_cast< const Derived& >(other).leq(
            static_cast< const Derived& >(*this));
    }

    virtual bool equals(const AbsDomBase& other) const override {
        return static_cast< const Derived& >(other).equals(
            static_cast< const Derived& >(*this));
    }

}; // class AbsDom

} // namespace knight::dfa
