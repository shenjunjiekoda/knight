//===- dom_registry.cpp -----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file registers all the domains.
//
//===------------------------------------------------------------------===//

#include "analyzer/core/domain/demo_dom.hpp"
#include "analyzer/core/domain/numerical/interval_dom.hpp"
#include "analyzer/core/domain/pointer.hpp"

namespace knight::analyzer {

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables,fuchsia-statically-constructed-objects)
std::unordered_map< DomainKind, DomainDefaultValFn > DefaultValFns;
std::unordered_map< DomainKind, DomainBottomValFn > BottomValFns;

DomainRegistry< ZIntervalDom > A;
DomainRegistry< DemoItvDom > B;
DomainRegistry< DemoMapDomain > C;
DomainRegistry< PointToSet > D;
DomainRegistry< PointerInfo > E;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,fuchsia-statically-constructed-objects)

} // namespace knight::analyzer