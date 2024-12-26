//===- state_printer.hpp ----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the state printer.
//
//===------------------------------------------------------------------===//

#pragma once

#include "analyzer/core/analysis/analysis_base.hpp"
#include "analyzer/core/analysis_context.hpp"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include "common/util/log.hpp"

namespace knight::analyzer {

class StatePrinter : public Analysis< StatePrinter,
                                      analyze::PreStmt< clang::Stmt >,
                                      analyze::PostStmt< clang::Stmt > > {
  public:
    explicit StatePrinter(KnightContext& ctx) : Analysis(ctx) {}

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::StatePrinter;
    }

    void pre_analyze_stmt(const clang::Stmt* stmt, AnalysisContext& ctx) const;
    void post_analyze_stmt(const clang::Stmt* stmt, AnalysisContext& ctx) const;

    static UniqueAnalysisRef register_analysis(AnalysisManager& mgr,
                                               KnightContext& ctx) {
        return mgr.register_analysis< StatePrinter >(ctx);
    }
};

} // namespace knight::analyzer