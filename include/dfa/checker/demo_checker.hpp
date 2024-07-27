//===- demo_checker.hpp -----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines some demo checker for testing purpose.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/checker/checker_base.hpp"
#include "dfa/checker_context.hpp"
#include "tooling/context.hpp"

#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

class DemoChecker : public Checker< check::BeginFunction > {
  public:
    DemoChecker(KnightContext& C) : Checker(C) {}

    CheckerKind get_kind() const override { return CheckerKind::DemoChecker; }

    void check_begin_function(CheckerContext& C) {
        llvm::outs() << "DemoChecker::check_begin_function()\n";
    }

}; // class DemoChecker

} // namespace knight::dfa