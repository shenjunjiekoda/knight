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
class RegionManager;

class AnalysisContext {
  private:
    KnightContext& m_ctx;
    const StackFrame* m_frame{nullptr};
    ProgramStateRef m_state{nullptr};
    RegionManager& m_region_manager;

  public:
    explicit AnalysisContext(KnightContext& ctx, RegionManager& region_manager);

    [[nodiscard]] RegionManager& get_region_manager() const;
    [[nodiscard]] KnightContext& get_knight_context() const { return m_ctx; }
    [[nodiscard]] clang::ASTContext& get_ast_context() const;
    [[nodiscard]] clang::SourceManager& get_source_manager() const;
    [[nodiscard]] const clang::Decl* get_current_decl() const;
    [[nodiscard]] const StackFrame* get_current_stack_frame() const;
    [[nodiscard]] ProgramStateRef get_state() const;
    void set_current_stack_frame(const StackFrame* frame);
    void set_state(ProgramStateRef state);
}; // class AnalysisContext

} // namespace dfa

} // namespace knight