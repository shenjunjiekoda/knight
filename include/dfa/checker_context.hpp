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

namespace knight::dfa {

class CheckerContext {
  private:
    clang::ASTContext& m_ast_ctx;

  public:
    CheckerContext(clang::ASTContext& ast_ctx) : m_ast_ctx(ast_ctx) {}

    clang::ASTContext& getASTContext() const { return m_ast_ctx; }
    clang::SourceManager& getSourceManager() const {
        return m_ast_ctx.getSourceManager();
    }
}; // class CheckerContext

} // namespace knight::dfa