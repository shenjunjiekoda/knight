
//===- pointer.hpp ----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the pointer analysis domain.
//
//===------------------------------------------------------------------===//

#pragma once

#include <utility>

#include "analyzer/core/analysis_manager.hpp"
#include "analyzer/core/domain/dom_base.hpp"
#include "analyzer/core/domain/domains.hpp"
#include "analyzer/core/domain/map/map_domain.hpp"
#include "analyzer/core/domain/set/discrete_domain.hpp"
#include "analyzer/core/region/region.hpp"

#include "analyzer/support/clang_ast.hpp"

namespace knight::analyzer {

constexpr DomID PointerInfoID = get_domain_id(DomainKind::PointerInfo);

using PointToSet = DiscreteDom< RegionDefRef, DomainKind::PointToSetDomain >;
using RegionPointToDom =
    MapDom< RegionRef, PointToSet, DomainKind::RegionPointToSetDomain >;
using StmtPointToDom = MapDom< internal::StmtRef,
                               RegionPointToDom,
                               DomainKind::StmtPointToSetDomain >;
using AliasToSet = DiscreteDom< RegionDefRef, DomainKind::AliasToDomain >;
using RegionAliasDom =
    MapDom< RegionRef, AliasToSet, DomainKind::RegionAliasToDomain >;
using StmtAliasDom =
    MapDom< internal::StmtRef, RegionAliasDom, DomainKind::StmtAliasToDomain >;

/// \brief The pointer analysis domain.
class PointerInfo : public AbsDom< PointerInfo > {
  private:
    RegionPointToDom m_region_point_to;
    StmtPointToDom m_stmt_point_to;
    RegionAliasDom m_region_alias;
    StmtAliasDom m_stmt_alias;

  public:
    explicit PointerInfo(
        RegionPointToDom region_point_to = RegionPointToDom::top(),
        StmtPointToDom stmt_point_to = StmtPointToDom::top(),
        RegionAliasDom region_alias = RegionAliasDom::top(),
        StmtAliasDom stmt_alias = StmtAliasDom::top())
        : m_region_point_to(std::move(region_point_to)),
          m_stmt_point_to(std::move(stmt_point_to)),
          m_region_alias(std::move(region_alias)),
          m_stmt_alias(std::move(stmt_alias)) {}

    PointerInfo(const PointerInfo&) = default;
    PointerInfo(PointerInfo&&) = default;
    PointerInfo& operator=(const PointerInfo&) = default;
    PointerInfo& operator=(PointerInfo&&) = default;
    ~PointerInfo() = default;

  public:
    [[nodiscard]] static PointerInfo top() { return PointerInfo(); }

    [[nodiscard]] static PointerInfo bottom() {
        return PointerInfo(RegionPointToDom::bottom(),
                           StmtPointToDom::bottom(),
                           RegionAliasDom::bottom(),
                           StmtAliasDom::bottom());
    }

    [[nodiscard]] RegionPointToDom& get_region_point_to_ref() {
        return m_region_point_to;
    }

    [[nodiscard]] const RegionPointToDom& get_region_point_to() const {
        return m_region_point_to;
    }

    void set_region_point_to(RegionPointToDom region_point_to) {
        m_region_point_to = std::move(region_point_to);
    }

    [[nodiscard]] StmtPointToDom& get_stmt_point_to_ref() {
        return m_stmt_point_to;
    }

    [[nodiscard]] const StmtPointToDom& get_stmt_point_to() const {
        return m_stmt_point_to;
    }

    void set_stmt_point_to(StmtPointToDom stmt_point_to) {
        m_stmt_point_to = std::move(stmt_point_to);
    }

    [[nodiscard]] RegionAliasDom& get_region_alias_ref() {
        return m_region_alias;
    }

    [[nodiscard]] const RegionAliasDom& get_region_alias() const {
        return m_region_alias;
    }

    void set_region_alias(RegionAliasDom region_alias) {
        m_region_alias = std::move(region_alias);
    }

    [[nodiscard]] StmtAliasDom& get_stmt_alias_ref() { return m_stmt_alias; }

    [[nodiscard]] const StmtAliasDom& get_stmt_alias() const {
        return m_stmt_alias;
    }

    void set_stmt_alias(StmtAliasDom stmt_alias) {
        m_stmt_alias = std::move(stmt_alias);
    }

  public:
    [[nodiscard]] static DomainKind get_kind() {
        return DomainKind::PointerInfo;
    }

    [[nodiscard]] static SharedVal default_val() {
        return std::make_shared< PointerInfo >();
    }

    [[nodiscard]] static SharedVal bottom_val() {
        return std::make_shared< PointerInfo >(RegionPointToDom::bottom(),
                                               StmtPointToDom::bottom(),
                                               RegionAliasDom::bottom(),
                                               StmtAliasDom::bottom());
    }

    [[nodiscard]] AbsDomBase* clone() const override {
        return new PointerInfo(m_region_point_to,
                               m_stmt_point_to,
                               m_region_alias,
                               m_stmt_alias);
    }

    void normalize() override {}

    [[nodiscard]] bool is_bottom() const override {
        return m_region_point_to.is_bottom() && m_stmt_point_to.is_bottom() &&
               m_region_alias.is_bottom() && m_stmt_alias.is_bottom();
    }

    [[nodiscard]] bool is_top() const override {
        return m_region_point_to.is_top() && m_stmt_point_to.is_top() &&
               m_region_alias.is_top() && m_stmt_alias.is_top();
    }

    void set_to_bottom() override {
        m_region_point_to.set_to_bottom();
        m_stmt_point_to.set_to_bottom();
        m_region_alias.set_to_bottom();
        m_stmt_alias.set_to_bottom();
    }

    void set_to_top() override {
        m_region_point_to.set_to_top();
        m_stmt_point_to.set_to_top();
        m_region_alias.set_to_top();
        m_stmt_alias.set_to_top();
    }

    void join_with(const PointerInfo& other) {
        m_region_point_to.join_with(other.m_region_point_to);
        m_stmt_point_to.join_with(other.m_stmt_point_to);
        m_region_alias.join_with(other.m_region_alias);
        m_stmt_alias.join_with(other.m_stmt_alias);
    }

    void widen_with(const PointerInfo& other) {
        m_region_point_to.widen_with(other.m_region_point_to);
        m_stmt_point_to.widen_with(other.m_stmt_point_to);
        m_region_alias.widen_with(other.m_region_alias);
        m_stmt_alias.widen_with(other.m_stmt_alias);
    }

    void meet_with(const PointerInfo& other) {
        m_region_point_to.meet_with(other.m_region_point_to);
        m_stmt_point_to.meet_with(other.m_stmt_point_to);
        m_region_alias.meet_with(other.m_region_alias);
        m_stmt_alias.meet_with(other.m_stmt_alias);
    }

    void narrow_with(const PointerInfo& other) {
        m_region_point_to.narrow_with(other.m_region_point_to);
        m_stmt_point_to.narrow_with(other.m_stmt_point_to);
        m_region_alias.narrow_with(other.m_region_alias);
        m_stmt_alias.narrow_with(other.m_stmt_alias);
    }

    [[nodiscard]] bool leq(const PointerInfo& other) const {
        return m_region_point_to.leq(other.m_region_point_to) &&
               m_stmt_point_to.leq(other.m_stmt_point_to) &&
               m_region_alias.leq(other.m_region_alias) &&
               m_stmt_alias.leq(other.m_stmt_alias);
    }

    [[nodiscard]] bool equals(const PointerInfo& other) const {
        return m_region_point_to.equals(other.m_region_point_to) &&
               m_stmt_point_to.equals(other.m_stmt_point_to) &&
               m_region_alias.equals(other.m_region_alias) &&
               m_stmt_alias.equals(other.m_stmt_alias);
    }

    void dump(llvm::raw_ostream& os) const override {
        os << "PointerInfo: \n";
        os << "RegionPointTo: \n";
        m_region_point_to.dump(os);
        os << "StmtPointTo: \n";
        m_stmt_point_to.dump(os);
        os << "RegionAlias: \n";
        m_region_alias.dump(os);
        os << "StmtAlias: \n";
        m_stmt_alias.dump(os);
    }

}; // class PointerInfoDom

} // namespace knight::analyzer