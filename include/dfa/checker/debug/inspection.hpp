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

#include "clang/AST/Expr.h"
#include "dfa/analysis/demo/itv_analysis.hpp"
#include "dfa/checker/checker_base.hpp"
#include "dfa/checker_context.hpp"
#include "dfa/checker_manager.hpp"
#include "llvm/ADT/StringRef.h"
#include "tooling/context.hpp"

#include "util/log.hpp"

namespace knight::dfa {

class InspectionChecker
    : public Checker< InspectionChecker, check::PostStmt< clang::CallExpr > > {
  private:
    static inline llvm::StringRef ZValDumper = "knight_dump_zval";

  public:
    explicit InspectionChecker(KnightContext& C) : Checker(C) {}

    [[nodiscard]] static CheckerKind get_kind() {
        return CheckerKind::DebugInspection;
    }

    void post_check_stmt(const clang::CallExpr* call_expr,
                         CheckerContext& ctx) const;

    void dump_zval(const clang::Expr* expr, CheckerContext& ctx) const;

    void check_begin_function(CheckerContext& C) const {}

    static void add_dependencies(CheckerManager& mgr) {
        // add dependencies here
        mgr.add_checker_dependency< InspectionChecker, ItvAnalysis >();
    }

    static UniqueCheckerRef register_checker(CheckerManager& mgr,
                                             KnightContext& ctx) {
        return mgr.register_checker< InspectionChecker >(ctx);
    }
}; // class InspectionChecker

} // namespace knight::dfa