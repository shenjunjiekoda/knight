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

#include "analyzer/core/location_context.hpp"
#include "analyzer/core/program_state.hpp"

namespace knight {

class KnightContext;

namespace analyzer {

class StackFrame;
class RegionManager;
class SymbolManager;

class AnalysisContext {
  private:
    KnightContext& m_ctx;
    const StackFrame* m_frame;
    const LocationContext* m_location_context;

    ProgramStateRef m_state{nullptr};
    RegionManager& m_region_manager;
    SymbolManager& m_sym_manager;

    bool m_is_state_changed = false;

  public:
    explicit AnalysisContext(KnightContext& ctx,
                             RegionManager& region_manager,
                             const StackFrame* frame,
                             SymbolManager& sym_manager,
                             const LocationContext* loc_ctx);

    [[nodiscard]] RegionManager& get_region_manager() const;
    [[nodiscard]] SymbolManager& get_symbol_manager() const;
    [[nodiscard]] KnightContext& get_knight_context() const { return m_ctx; }
    [[nodiscard]] clang::ASTContext& get_ast_context() const;
    [[nodiscard]] clang::SourceManager& get_source_manager() const;
    [[nodiscard]] const clang::Decl* get_current_decl() const;
    [[nodiscard]] const StackFrame* get_current_stack_frame() const;
    [[nodiscard]] const LocationContext* get_current_location_context() const;
    [[nodiscard]] ProgramStateRef get_state() const;
    void set_current_stack_frame(const StackFrame* frame);
    void set_state(ProgramStateRef state);
    [[nodiscard]] bool is_state_changed() const { return m_is_state_changed; }
}; // class AnalysisContext

} // namespace analyzer

} // namespace knight