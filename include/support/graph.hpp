//===- graph.hpp ------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the graph concept and utility.
//
//===------------------------------------------------------------------===//

#pragma once

#include <concepts>

namespace knight {

template < typename T >
concept graph =
    requires(typename T::GraphRef graph, typename T::NodeRef nodeRef) {
        typename T::GraphRef;
        typename T::NodeRef;
        typename T::PredNodeIterator;
        typename T::SuccNodeIterator;

        // T::NodeRef shall have operator==
        {
            std::declval< typename T::NodeRef >() ==
                std::declval< typename T::NodeRef >()
        } -> std::same_as< bool >;

        // iterators shall be dereferenced to NodeRef
        {
            *std::declval< typename T::SuccNodeIterator >()
        } -> std::convertible_to< typename T::NodeRef >;
        {
            *std::declval< typename T::PredNodeIterator >()
        } -> std::convertible_to< typename T::NodeRef >;

        // T must have static member functions `entry`, `succ_begin`,
        // `succ_end`, `pred_begin`, `pred_end`
        { T::entry(graph) } -> std::convertible_to< typename T::NodeRef >;
        {
            T::succ_begin(nodeRef)
        } -> std::convertible_to< typename T::SuccNodeIterator >;
        {
            T::succ_end(nodeRef)
        } -> std::convertible_to< typename T::SuccNodeIterator >;
        {
            T::pred_begin(nodeRef)
        } -> std::convertible_to< typename T::PredNodeIterator >;
        {
            T::pred_end(nodeRef)
        } -> std::convertible_to< typename T::PredNodeIterator >;
    }; // concept graph

template < graph T > struct GraphTrait {
    using GraphRef = typename T::GraphRef;
    using NodeRef = typename T::NodeRef;
    using SuccNodeIterator = typename T::SuccNodeIterator;
    using PredNodeIterator = typename T::PredNodeIterator;

    static NodeRef entry(GraphRef graph) { return T::entry(graph); }
    static SuccNodeIterator succ_begin(NodeRef nodeRef) {
        return T::succ_begin(nodeRef);
    }
    static SuccNodeIterator succ_end(NodeRef nodeRef) {
        return T::succ_end(nodeRef);
    }
    static PredNodeIterator pred_begin(NodeRef nodeRef) {
        return T::pred_begin(nodeRef);
    }
    static PredNodeIterator pred_end(NodeRef nodeRef) {
        return T::pred_end(nodeRef);
    }
}; // struct GraphTrait

template < typename T >
struct isa_graph :                                      // NOLINT
                   std::bool_constant< graph< T > > {}; // struct isa_graph

} // namespace knight