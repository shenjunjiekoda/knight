
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

#include "dfa/domain/domains.hpp"
#include "dfa/domain/map/map_domain.hpp"
#include "dfa/domain/set/discrete_domain.hpp"
#include "dfa/region/region.hpp"

namespace knight::dfa {

constexpr DomID PointerDomID = get_domain_id(DomainKind::PointerDomain);

using PointToSet = DiscreteDom< RegionRef, DomainKind::PointToSetDomain >;
using PointerDom = MapDom< RegionRef, PointToSet, DomainKind::PointerDomain >;

} // namespace knight::dfa