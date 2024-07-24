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
#include "util/assert.hpp"

namespace knight::dfa {

class IntervalDomain : public AbsDom< IntervalDomain > {
    int lower, upper;

  public:
    IntervalDomain(int l, int u) : lower(l), upper(u) {}
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
    void dump(llvm::raw_ostream& os) const override {
        os << "[" << lower << ", " << upper << "]";
    }

    void join_with(const IntervalDomain& other) {
        lower = std::min(lower, other.lower);
        upper = std::max(upper, other.upper);
    }

    void widen_with(const IntervalDomain& other) {
        knight_assert(false && "not implemented");
    }

    void meet_with(const IntervalDomain& other) {
        knight_assert(false && "not implemented");
    }

    void narrow_with(const IntervalDomain& other) {
        knight_assert(false && "not implemented");
    }
};

} // namespace knight::dfa
