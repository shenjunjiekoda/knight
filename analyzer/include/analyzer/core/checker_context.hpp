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

#include "analyzer/core/location_context.hpp"
#include "analyzer/core/program_state.hpp"
#include "analyzer/core/symbol_manager.hpp"

namespace knight {

class KnightContext;

namespace analyzer {

class StackFrame;

class CheckerContext {
  private:
    KnightContext& m_ctx;
    const StackFrame* m_frame;
    ProgramStateRef m_state{nullptr};
    const LocationContext* m_location_context;

    SymbolManager& m_sym_manager;

  public:
    explicit CheckerContext(KnightContext& ctx,
                            const StackFrame* frame,
                            SymbolManager& sym_manager,
                            const LocationContext* loc_ctx)
        : m_ctx(ctx),
          m_frame(frame),
          m_location_context(loc_ctx),
          m_sym_manager(sym_manager) {}

    [[nodiscard]] KnightContext& get_knight_context() const { return m_ctx; }
    [[nodiscard]] clang::ASTContext& get_ast_context() const;
    [[nodiscard]] clang::SourceManager& get_source_manager() const;
    [[nodiscard]] ProgramStateRef get_state() const { return m_state; }
    [[nodiscard]] const clang::Decl* get_current_decl() const;
    [[nodiscard]] const StackFrame* get_current_stack_frame() const;
    [[nodiscard]] const LocationContext* get_location_context() const {
        return m_location_context;
    }
    [[nodiscard]] SymbolManager& get_symbol_manager() const {
        return m_sym_manager;
    }

    void set_current_state(ProgramStateRef state) {
        m_state = std::move(state);
    }
    void set_current_stack_frame(const StackFrame* frame);
    void set_location_context(const LocationContext* loc_ctx) {
        m_location_context = loc_ctx;
    }
}; // class CheckerContext

} // namespace analyzer
} // namespace knight