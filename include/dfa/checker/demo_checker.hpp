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

#include "dfa/analysis/demo_analysis.hpp"
#include "dfa/checker/checker_base.hpp"
#include "dfa/checker_context.hpp"
#include "dfa/checker_manager.hpp"
#include "tooling/context.hpp"

#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

class DemoChecker : public Checker< DemoChecker, check::BeginFunction > {
  public:
    DemoChecker(KnightContext& C) : Checker(C) {}

    static CheckerKind get_kind() { return CheckerKind::DemoChecker; }

    void check_begin_function(CheckerContext& C) const {
        llvm::outs() << "DemoChecker::check_begin_function()\n";
    }

    static void add_dependencies(CheckerManager& mgr) {
        // add dependencies here
        mgr.add_checker_dependency< DemoChecker, DemoAnalysis >();
    }

    static UniqueCheckerRef register_checker(CheckerManager& mgr,
                                             KnightContext& ctx) {
        return mgr.register_checker< DemoChecker >(ctx);
    }
}; // class DemoChecker

} // namespace knight::dfa