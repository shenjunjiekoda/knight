//===- demo_dom.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines some demo domain for testing purpose.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"
#include "util/assert.hpp"

namespace knight::dfa {

class DemoItvDom : public AbsDom< DemoItvDom > {
    int lower, upper;

  public:
    DemoItvDom(int l, int u) : lower(l), upper(u) {}

    /// \brief the unique name of the domain
    llvm::StringRef get_name() const override {
        return get_domain_name(DomainKind::DemoItvDom);
    }

    /// \brief the unique id of the domain
    DomID get_id() const override {
        return get_domain_id(DomainKind::DemoItvDom);
    }

  public:
    void normalize() override {}

    bool is_bottom() const override { return lower > upper; }

    bool is_top() const override {
        return lower == INT_MIN && upper == INT_MAX;
    }

    void set_to_bottom() override {
        lower = 1;
        upper = 0;
    }

    void set_to_top() override {
        lower = INT_MIN;
        upper = INT_MAX;
    }

    void join_with(const DemoItvDom& other) {
        lower = std::min(lower, other.lower);
        upper = std::max(upper, other.upper);
    }

    void join_with_at_loop_head(const DemoItvDom& other) { join_with(other); }

    void join_consecutive_iter_with(const DemoItvDom& other) {
        join_with(other);
    }

    void widen_with(const DemoItvDom& other) {
        knight_assert(false && "not implemented");
    }

    void meet_with(const DemoItvDom& other) {
        knight_assert(false && "not implemented");
    }

    void narrow_with(const DemoItvDom& other) {
        knight_assert(false && "not implemented");
    }

    bool leq(const DemoItvDom& other) const {
        return lower <= other.lower && upper >= other.upper;
    }

    bool equals(const DemoItvDom& other) const {
        return lower == other.lower && upper == other.upper;
    }

    void dump(llvm::raw_ostream& os) const override {
        os << "[" << lower << ", " << upper << "]";
    }
};

} // namespace knight::dfa