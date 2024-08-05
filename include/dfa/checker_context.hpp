//===- checker_context.hpp --------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the checker context class.
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

class CheckerContext {
  private:
    KnightContext& m_ctx;
    const StackFrame* m_frame;
    ProgramStateRef m_state{};

  public:
    explicit CheckerContext(KnightContext& ctx) : m_ctx(ctx) {}

    [[nodiscard]] KnightContext& get_knight_context() const { return m_ctx; }
    [[nodiscard]] clang::ASTContext& get_ast_context() const;
    [[nodiscard]] clang::SourceManager& get_source_manager() const;
    [[nodiscard]] ProgramStateRef get_state() const { return m_state; }
    [[nodiscard]] const clang::Decl* get_current_decl() const;
    [[nodiscard]] const StackFrame* get_current_stack_frame() const;

    void set_current_state(ProgramStateRef state) {
        m_state = std::move(state);
    }
    void set_current_stack_frame(const StackFrame* frame);
}; // class CheckerContext

} // namespace dfa
} // namespace knight