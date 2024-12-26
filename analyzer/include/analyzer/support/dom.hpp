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

namespace knight::analyzer {

template < typename DerivedDom >
concept derived_dom_has_join_with_method = requires {
    {
        &DerivedDom::join_with
    } -> std::same_as< void (DerivedDom::*)(const DerivedDom&) >;
};

template < typename DerivedDom >
struct does_derived_dom_can_join_with // NOLINT
    : std::bool_constant< derived_dom_has_join_with_method< DerivedDom > > {
}; // struct does_derived_dom_can_join_with

template < typename DerivedDom >
concept derived_dom_has_join_with_at_loop_head_method = requires {
    {
        &DerivedDom::join_with_at_loop_head
    } -> std::same_as< void (DerivedDom::*)(const DerivedDom&) >;
};

template < typename DerivedDom >
struct does_derived_dom_can_join_with_at_loop_head // NOLINT
    : std::bool_constant<
          derived_dom_has_join_with_at_loop_head_method< DerivedDom > > {
}; // struct does_derived_dom_can_join_with_at_loop_head

template < typename DerivedDom >
concept derived_dom_has_join_consecutive_iter_with_method = requires {
    {
        &DerivedDom::join_consecutive_iter_with
    } -> std::same_as< void (DerivedDom::*)(const DerivedDom&) >;
};

template < typename DerivedDom >
struct does_derived_dom_can_join_consecutive_iter_with // NOLINT
    : std::bool_constant<
          derived_dom_has_join_consecutive_iter_with_method< DerivedDom > > {
}; // struct does_derived_dom_can_join_consecutive_iter_with

template < typename DerivedDom >
concept derived_dom_has_widen_with_method = requires {
    {
        &DerivedDom::widen_with
    } -> std::same_as< void (DerivedDom::*)(const DerivedDom&) >;
};

template < typename DerivedDom >
struct does_derived_dom_can_widen_with // NOLINT
    : std::bool_constant< derived_dom_has_widen_with_method< DerivedDom > > {
}; // struct does_derived_dom_can_widen_with

template < typename DerivedDom >
concept derived_dom_has_meet_with_method = requires {
    {
        &DerivedDom::meet_with
    } -> std::same_as< void (DerivedDom::*)(const DerivedDom&) >;
};

template < typename DerivedDom >
struct does_derived_dom_can_meet_with // NOLINT
    : std::bool_constant< derived_dom_has_meet_with_method< DerivedDom > > {
}; // struct does_derived_dom_can_meet_with

template < typename DerivedDom >
concept derived_dom_has_narrow_with_method = requires {
    {
        &DerivedDom::narrow_with
    } -> std::same_as< void (DerivedDom::*)(const DerivedDom&) >;
};

template < typename DerivedDom >
struct does_derived_dom_can_narrow_with // NOLINT
    : std::bool_constant< derived_dom_has_narrow_with_method< DerivedDom > > {
}; // struct does_derived_dom_can_narrow_with

template < typename DerivedDom >
concept derived_dom_has_leq_method = requires {
    {
        &DerivedDom::leq
    } -> std::same_as< bool (DerivedDom::*)(const DerivedDom&) const >;
};

template < typename DerivedDom >
struct does_derived_dom_can_leq // NOLINT
    : std::bool_constant< derived_dom_has_leq_method< DerivedDom > > {
}; // struct does_derived_dom_can_leq

template < typename DerivedDom >
concept derived_dom_has_equals_method = requires {
    {
        &DerivedDom::equals
    } -> std::same_as< bool (DerivedDom::*)(const DerivedDom&) const >;
};

template < typename DerivedDom >
struct does_derived_dom_can_equals // NOLINT
    : std::bool_constant< derived_dom_has_equals_method< DerivedDom > > {
}; // struct does_derived_dom_can_equals

template < typename DerivedNumericalDom, typename Num >
concept derived_numerical_dom_has_widen_with_threshold_method = requires {
    {
        &DerivedNumericalDom::widen_with_threshold
    } -> std::same_as< void (DerivedNumericalDom::*)(const DerivedNumericalDom&,
                                                     const Num&) >;
};

template < typename DerivedNumericalDom, typename Num >
struct does_derived_numerical_dom_can_widen_with_threshold // NOLINT
    : std::bool_constant< derived_numerical_dom_has_widen_with_threshold_method<
          DerivedNumericalDom,
          Num > > {
}; // struct does_derived_numerical_dom_can_widen_with_threshold

template < typename DerivedNumericalDom, typename Num >
concept derived_numerical_dom_has_narrow_with_threshold_method = requires {
    {
        &DerivedNumericalDom::narrow_with_threshold
    } -> std::same_as< void (DerivedNumericalDom::*)(const DerivedNumericalDom&,
                                                     const Num&) >;
};

template < typename DerivedNumericalDom, typename Num >
struct does_derived_numerical_dom_can_narrow_with_threshold // NOLINT
    : std::bool_constant<
          derived_numerical_dom_has_narrow_with_threshold_method<
              DerivedNumericalDom,
              Num > > {
}; // struct does_derived_numerical_dom_can_narrow_with_threshold

} // namespace knight::analyzer