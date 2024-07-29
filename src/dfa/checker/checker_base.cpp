//===- checker_base.cpp ----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the checker base class.
//
//===------------------------------------------------------------------===//

#include "dfa/checker/checker_base.hpp"
#include "dfa/analysis/analysis_base.hpp"
#include "dfa/checker/checkers.hpp"
#include "tooling/context.hpp"

namespace knight::dfa {

CheckerBase::CheckerBase(KnightContext& ctx, CheckerKind k)
    : m_ctx(ctx), kind(k) {}

clang::DiagnosticBuilder CheckerBase::diagnose(
    clang::SourceLocation loc,
    llvm::StringRef info,
    clang::DiagnosticIDs::Level diag_level) {
    return m_ctx.diagnose(get_checker_name(kind), loc, info, diag_level);
}

clang::DiagnosticBuilder CheckerBase::diagnose(
    llvm::StringRef info, clang::DiagnosticIDs::Level diag_level) {
    return m_ctx.diagnose(get_checker_name(kind), info, diag_level);
}

clang::DiagnosticBuilder CheckerBase::diagnose(
    clang::DiagnosticIDs::Level diag_level) {
    return m_ctx.diagnose(get_checker_name(kind),
                          get_checker_desc(kind),
                          diag_level);
}

} // namespace knight::dfa