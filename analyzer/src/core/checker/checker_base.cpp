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

#include "analyzer/core/checker/checker_base.hpp"
#include "analyzer/core/analysis/analysis_base.hpp"
#include "analyzer/core/checker/checkers.hpp"
#include "analyzer/tooling/context.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>

namespace knight::analyzer {

CheckerBase::CheckerBase(KnightContext& ctx, CheckerKind k)
    : m_ctx(ctx), kind(k) {}

clang::DiagnosticBuilder CheckerBase::diagnose(
    clang::SourceLocation loc,
    llvm::StringRef info,
    clang::DiagnosticIDs::Level diag_level) const {
    return m_ctx.diagnose(get_checker_name(kind), loc, info, diag_level);
}

clang::DiagnosticBuilder CheckerBase::diagnose(
    llvm::StringRef info, clang::DiagnosticIDs::Level diag_level) const {
    return m_ctx.diagnose(get_checker_name(kind), info, diag_level);
}

clang::DiagnosticBuilder CheckerBase::diagnose(
    clang::DiagnosticIDs::Level diag_level) const {
    return m_ctx.diagnose(get_checker_name(kind),
                          get_checker_desc(kind),
                          diag_level);
}

} // namespace knight::analyzer