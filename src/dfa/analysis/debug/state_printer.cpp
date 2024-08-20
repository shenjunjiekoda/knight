//===- state_printer.cpp ----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements the state printer.
//
//===------------------------------------------------------------------===//

#include "dfa/analysis/debug/state_printer.hpp"
#include "llvm/Support/raw_ostream.h"

namespace knight::dfa {

void StatePrinter::pre_analyze_stmt(const clang::Stmt* stmt,
                                    AnalysisContext& ctx) const {
    llvm::outs().changeColor(llvm::raw_ostream::Colors::BLUE);
    llvm::outs() << "STATE-BEFORE(" << stmt->getStmtClassName() << "): ";
    stmt->printPretty(llvm::outs(),
                      nullptr,
                      ctx.get_ast_context().getPrintingPolicy());
    llvm::outs().resetColor();
    llvm::outs() << "\n";

    ctx.get_state()->dump(llvm::outs());

    llvm::outs() << "\n";
}

void StatePrinter::post_analyze_stmt(const clang::Stmt* stmt,
                                     AnalysisContext& ctx) const {
    llvm::outs().changeColor(llvm::raw_ostream::Colors::GREEN);
    llvm::outs() << "STATE-AFTER(" << stmt->getStmtClassName() << "): ";
    stmt->printPretty(llvm::outs(),
                      nullptr,
                      ctx.get_ast_context().getPrintingPolicy());
    llvm::outs().resetColor();
    llvm::outs() << "\n";

    ctx.get_state()->dump(llvm::outs());

    llvm::outs() << "\n";
}

} // namespace knight::dfa