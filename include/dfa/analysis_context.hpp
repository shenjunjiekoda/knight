//===- analysis_context.hpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the analysis context class.
//
//===------------------------------------------------------------------===//

#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclBase.h>

#include "dfa/program_state.hpp"

namespace knight {

class KnightContext;

namespace dfa {

class AnalysisContext {
  private:
    KnightContext& m_ctx;
    const clang::Decl* m_current_decl;
    ProgramStateRef m_state;

  public:
    AnalysisContext(KnightContext& ctx) : m_ctx(ctx) {}

    KnightContext& get_knight_context() const { return m_ctx; }
    clang::ASTContext& get_ast_context() const;
    clang::SourceManager& get_source_manager() const;
    const clang::Decl* get_current_decl() const { return m_current_decl; }
    ProgramStateRef get_state() const;

    void set_current_decl(const clang::Decl* decl) { m_current_decl = decl; }
    void set_state(ProgramStateRef state);
}; // class AnalysisContext

} // namespace dfa

} // namespace knight