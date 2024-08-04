//===- analysis_base.cpp ----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the analysis base class.
//
//===------------------------------------------------------------------===//

#include "dfa/analysis/analysis_base.hpp"
#include "dfa/analysis/analyses.hpp"
#include "tooling/context.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>

namespace knight::dfa {

AnalysisBase::AnalysisBase(KnightContext& ctx, AnalysisKind k)
    : m_ctx(ctx), kind(k) {}

clang::DiagnosticBuilder AnalysisBase::diagnose(
    clang::SourceLocation loc,
    llvm::StringRef info,
    clang::DiagnosticIDs::Level diag_level) {
    return m_ctx.diagnose(get_analysis_name(kind), loc, info, diag_level);
}

clang::DiagnosticBuilder AnalysisBase::diagnose(
    llvm::StringRef info, clang::DiagnosticIDs::Level diag_level) {
    return m_ctx.diagnose(get_analysis_name(kind), info, diag_level);
}

clang::DiagnosticBuilder AnalysisBase::diagnose(
    clang::DiagnosticIDs::Level diag_level) {
    return m_ctx.diagnose(get_analysis_name(kind),
                          get_analysis_desc(kind),
                          diag_level);
}

} // namespace knight::dfa