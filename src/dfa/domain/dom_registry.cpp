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
#include "dfa/domain/interval_dom.hpp"

namespace knight::dfa {

std::unordered_map< DomainKind, DomainDefaultValFn > DefaultValFns;
std::unordered_map< DomainKind, DomainBottomValFn > BottomValFns;

static DomainRegistry< ZIntervalDom > A;
static DomainRegistry< QIntervalDom > B;
static DomainRegistry< DemoItvDom > C;
static DomainRegistry< DemoMapDomain > D;

} // namespace knight::dfa