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

namespace knight::dfa {

AnalysisBase::AnalysisBase(KnightContext& ctx) : m_ctx(ctx) {}

clang::DiagnosticBuilder AnalysisBase::diagnose(
    clang::SourceLocation loc,
    llvm::StringRef info,
    clang::DiagnosticIDs::Level diag_level) {
    return m_ctx.diagnose(get_analysis_name(get_kind()), loc, info, diag_level);
}

clang::DiagnosticBuilder AnalysisBase::diagnose(
    llvm::StringRef info, clang::DiagnosticIDs::Level diag_level) {
    return m_ctx.diagnose(get_analysis_name(get_kind()), info, diag_level);
}

clang::DiagnosticBuilder AnalysisBase::diagnose(
    clang::DiagnosticIDs::Level diag_level) {
    return m_ctx.diagnose(get_analysis_name(get_kind()),
                          get_analysis_desc(get_kind()),
                          diag_level);
}

} // namespace knight::dfa