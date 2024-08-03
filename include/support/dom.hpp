//===- dom.hpp --------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the domain concept and utility.
//
//===------------------------------------------------------------------===//

#pragma once

#include <concepts>

namespace knight::dfa {

template < typename DerivedDom >
concept derived_dom_has_join_with_method =
    requires(DerivedDom& d1, const DerivedDom& d2) {
        { d1.join_with(d2) } -> std::same_as< void >;
    };

template < typename DerivedDom >
struct does_derived_dom_can_join_with // NOLINT
    : std::bool_constant< derived_dom_has_join_with_method< DerivedDom > > {
}; // struct does_derived_dom_can_join_with

template < typename DerivedDom >
concept derived_dom_has_join_with_at_loop_head_method =
    requires(DerivedDom& d1, const DerivedDom& d2) {
        { d1.join_with_at_loop_head(d2) } -> std::same_as< void >;
    };

template < typename DerivedDom >
struct does_derived_dom_can_join_with_at_loop_head // NOLINT
    : std::bool_constant<
          derived_dom_has_join_with_at_loop_head_method< DerivedDom > > {
}; // struct does_derived_dom_can_join_with_at_loop_head

template < typename DerivedDom >
concept derived_dom_has_join_consecutive_iter_with_method =
    requires(DerivedDom& d1, const DerivedDom& d2) {
        { d1.join_consecutive_iter_with(d2) } -> std::same_as< void >;
    };

template < typename DerivedDom >
struct does_derived_dom_can_join_consecutive_iter_with // NOLINT
    : std::bool_constant<
          derived_dom_has_join_consecutive_iter_with_method< DerivedDom > > {
}; // struct does_derived_dom_can_join_consecutive_iter_with

template < typename DerivedDom >
concept derived_dom_has_widen_with_method =
    requires(DerivedDom& d1, const DerivedDom& d2) {
        { d1.widen_with(d2) } -> std::same_as< void >;
    };

template < typename DerivedDom >
struct does_derived_dom_can_widen_with // NOLINT
    : std::bool_constant< derived_dom_has_widen_with_method< DerivedDom > > {
}; // struct does_derived_dom_can_widen_with

template < typename DerivedDom >
concept derived_dom_has_meet_with_method =
    requires(DerivedDom& d1, const DerivedDom& d2) {
        { d1.meet_with(d2) } -> std::same_as< void >;
    };

template < typename DerivedDom >
struct does_derived_dom_can_meet_with // NOLINT
    : std::bool_constant< derived_dom_has_meet_with_method< DerivedDom > > {
}; // struct does_derived_dom_can_meet_with

template < typename DerivedDom >
concept derived_dom_has_narrow_with_method =
    requires(DerivedDom& d1, const DerivedDom& d2) {
        { d1.narrow_with(d2) } -> std::same_as< void >;
    };

template < typename DerivedDom >
struct does_derived_dom_can_narrow_with // NOLINT
    : std::bool_constant< derived_dom_has_narrow_with_method< DerivedDom > > {
}; // struct does_derived_dom_can_narrow_with

template < typename DerivedDom >
concept derived_dom_has_leq_method =
    requires(const DerivedDom& d1, const DerivedDom& d2) {
        { d1.leq(d2) } -> std::same_as< bool >;
    };

template < typename DerivedDom >
struct does_derived_dom_can_leq // NOLINT
    : std::bool_constant< derived_dom_has_leq_method< DerivedDom > > {
}; // struct does_derived_dom_can_leq

template < typename DerivedDom >
concept derived_dom_has_equals_method =
    requires(const DerivedDom& d1, const DerivedDom& d2) {
        { d1.equals(d2) } -> std::same_as< bool >;
    };

template < typename DerivedDom >
struct does_derived_dom_can_equals // NOLINT
    : std::bool_constant< derived_dom_has_equals_method< DerivedDom > > {
}; // struct does_derived_dom_can_equals

} // namespace knight::dfa