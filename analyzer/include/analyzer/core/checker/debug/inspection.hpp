//===- inspection.hpp -------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines inspection checker for debugging purpose.
//
//===------------------------------------------------------------------===//

#pragma once

#include "analyzer/core/analysis/core/numerical_analysis.hpp"
#include "analyzer/core/checker/checker_base.hpp"
#include "analyzer/core/checker_context.hpp"
#include "analyzer/core/checker_manager.hpp"
#include "analyzer/tooling/context.hpp"
#include "common/util/log.hpp"

#include <clang/AST/Expr.h>
#include <llvm/ADT/StringRef.h>

namespace knight::analyzer {

class InspectionChecker
    : public Checker< InspectionChecker, check::PostStmt< clang::CallExpr > > {
  private:
    static inline llvm::StringRef ZValDumper = "knight_dump_zval";
    static inline llvm::StringRef ReachabilityDumper = "knight_reachable";

  public:
    explicit InspectionChecker(KnightContext& C) : Checker(C) {}

    [[nodiscard]] static CheckerKind get_kind() {
        return CheckerKind::DebugInspection;
    }

    void post_check_stmt(const clang::CallExpr* call_expr,
                         CheckerContext& ctx) const;

    void dump_zval(const clang::Expr* expr, CheckerContext& ctx) const;

    void dump_reachability(const clang::CallExpr* call_expr,
                           CheckerContext& ctx) const;

    void check_begin_function(CheckerContext& C) const {}

    static void add_dependencies(CheckerManager& mgr) {
        // add dependencies here
        mgr.add_checker_dependency< InspectionChecker, NumericalAnalysis >();
    }

    static UniqueCheckerRef register_checker(CheckerManager& mgr,
                                             KnightContext& ctx) {
        return mgr.register_checker< InspectionChecker >(ctx);
    }
}; // class InspectionChecker

} // namespace knight::analyzer