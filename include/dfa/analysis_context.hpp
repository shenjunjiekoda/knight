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

class StackFrame;

class AnalysisContext {
  private:
    KnightContext& m_ctx;
    StackFrame* m_frame;
    ProgramStateRef m_state{nullptr};

  public:
    explicit AnalysisContext(KnightContext& ctx) : m_ctx(ctx) {}

    [[nodiscard]] KnightContext& get_knight_context() const { return m_ctx; }
    [[nodiscard]] clang::ASTContext& get_ast_context() const;
    [[nodiscard]] clang::SourceManager& get_source_manager() const;
    [[nodiscard]] const clang::Decl* get_current_decl() const;
    [[nodiscard]] StackFrame* get_current_stack_frame() const;
    [[nodiscard]] ProgramStateRef get_state() const;
    void set_current_stack_frame(StackFrame* frame);
    void set_state(ProgramStateRef state);
}; // class AnalysisContext

} // namespace dfa

} // namespace knight