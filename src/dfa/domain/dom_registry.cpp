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

#include "dfa/domain/demo_dom.hpp"
#include "dfa/domain/numerical/interval_dom.hpp"
#include "dfa/domain/pointer.hpp"

namespace knight::dfa {

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables,fuchsia-statically-constructed-objects)
std::unordered_map< DomainKind, DomainDefaultValFn > DefaultValFns;
std::unordered_map< DomainKind, DomainBottomValFn > BottomValFns;

DomainRegistry< ZIntervalDom > A;
DomainRegistry< DemoItvDom > B;
DomainRegistry< DemoMapDomain > C;
DomainRegistry< PointToSet > D;
DomainRegistry< PointerDom > E;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,fuchsia-statically-constructed-objects)

} // namespace knight::dfa