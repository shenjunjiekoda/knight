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
#include "tooling/context.hpp"

namespace knight::dfa {

clang::ASTContext& AnalysisContext::get_ast_context() const {
    return *m_ctx.get_ast_context();
}

clang::SourceManager& AnalysisContext::get_source_manager() const {
    return m_ctx.get_source_manager();
}

} // namespace knight::dfa