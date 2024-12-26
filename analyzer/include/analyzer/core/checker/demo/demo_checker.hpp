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

#include "analyzer/core/analysis/demo/demo_analysis.hpp"
#include "analyzer/core/checker/checker_base.hpp"
#include "analyzer/core/checker_context.hpp"
#include "analyzer/core/checker_manager.hpp"
#include "analyzer/tooling/context.hpp"

#include "common/util/log.hpp"

namespace knight::analyzer {

class DemoChecker : public Checker< DemoChecker, check::BeginFunction > {
  public:
    DemoChecker(KnightContext& C) : Checker(C) {}

    [[nodiscard]] static CheckerKind get_kind() {
        return CheckerKind::DemoChecker;
    }

    void check_begin_function(CheckerContext& C) const {
        auto* func =
            dyn_cast_or_null< clang::FunctionDecl >(C.get_current_decl());
        if (func == nullptr || !func->getDeclName().isIdentifier()) {
            return;
        }

        if (func->getName().starts_with("_")) {
            diagnose(func->getNameInfo().getLoc(),
                     "bad function name `" + func->getName().str() + "`");
        }
        if (!func->isUsed() && !func->isReferenced()) {
            diagnose(func->getBeginLoc(),
                     "unused function `" + func->getName().str() + "`");
        }
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

} // namespace knight::analyzer