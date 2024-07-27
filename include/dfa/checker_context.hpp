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

#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclBase.h"

namespace knight {

class KnightContext;

namespace dfa {

class CheckerContext {
  private:
    KnightContext& m_ctx;

  public:
    CheckerContext(KnightContext& ctx) : m_ctx(ctx) {}

    KnightContext& get_knight_context() const { return m_ctx; }
    clang::ASTContext& get_ast_context() const;
    clang::SourceManager& get_source_manager() const;
}; // class CheckerContext

} // namespace dfa
} // namespace knight