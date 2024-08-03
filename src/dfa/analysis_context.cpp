//===- analysis_context.cpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the analysis context class.
//
//===------------------------------------------------------------------===//

#include "dfa/analysis_context.hpp"
#include "dfa/analysis/analysis_base.hpp"
#include "dfa/program_state.hpp"
#include "tooling/context.hpp"

namespace knight::dfa {

clang::ASTContext& AnalysisContext::get_ast_context() const {
    return *m_ctx.get_ast_context();
}

clang::SourceManager& AnalysisContext::get_source_manager() const {
    return m_ctx.get_source_manager();
}

ProgramStateRef AnalysisContext::get_state() const {
    return m_state;
}

void AnalysisContext::set_state(ProgramStateRef state) {
    m_state = std::move(state);
}

} // namespace knight::dfa