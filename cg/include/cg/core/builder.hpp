//===- builder.hpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the core builder for the Knight cg tool.
//
//===------------------------------------------------------------------===//

#pragma once

#include <clang/AST/RecursiveASTVisitor.h>

#include "cg/core/cg.hpp"
#include "cg/db/db.hpp"
#include "common/util/log.hpp"

#include <unordered_map>
#include <unordered_set>

namespace knight {

class CGContext;

namespace cg {

class CGBuilder : public clang::RecursiveASTVisitor< CGBuilder > {
  public:
    using Base = clang::RecursiveASTVisitor< CGBuilder >;

  private:
    knight::CGContext& m_ctx;
    Database m_db;

    std::string m_current_function_name;

  public:
    explicit CGBuilder(knight::CGContext& ctx);

    [[nodiscard]] bool shouldVisitTemplateInstantiations() const { // NOLINT
        return true;
    }
    [[nodiscard]] bool shouldWalkTypesOfTypeLocs() const { // NOLINT
        return false;
    }
    [[nodiscard]] bool shouldVisitImplicitCode() const;
    [[nodiscard]] bool shouldVisitLambdaBody() const { return true; } // NOLINT

    [[nodiscard]] bool VisitCallExpr(const clang::CallExpr* call);
    [[nodiscard]] bool VisitCXXConstructExpr(
        const clang::CXXConstructExpr* ctor_call);
    [[nodiscard]] bool VisitFunctionDecl(const clang::FunctionDecl* function);
}; // class CGBuilder

} // namespace cg
} // namespace knight