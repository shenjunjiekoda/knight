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

class AbsDomBase {
  public:
    virtual llvm::StringRef get_name() const = 0;
    virtual DomID get_id() const = 0;

    virtual void normalize() = 0;
    virtual bool is_bottom() const = 0;
    virtual bool is_top() const = 0;
    virtual void set_to_bottom() = 0;
    virtual void set_to_top() = 0;

    virtual void join_with(const AbsDomBase& other) = 0;
    virtual void widen_with(const AbsDomBase& other) = 0;
    virtual void meet_with(const AbsDomBase& other) = 0;
    virtual void narrow_with(const AbsDomBase& other) = 0;

    virtual void dump(llvm::raw_ostream& os) const = 0;
}; // class AbsDomBase

template < typename Derived > class AbsDom : public AbsDomBase {
  public:
    void join_with(const AbsDomBase& other) override {
        static_cast< Derived* >(this)->join_with(
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
}; // class AbsDom

} // namespace knight::dfa
