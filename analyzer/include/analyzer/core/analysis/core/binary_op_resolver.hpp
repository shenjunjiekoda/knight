//===- binary_op_resolver.hpp ------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the uanry op resolver.
//
//===------------------------------------------------------------------===//

#pragma once

#include "analyzer/core/analysis/core/symbol_resolver.hpp"
#include "analyzer/core/analysis/core/unary_op_resolver.hpp"
#include "analyzer/core/analysis_context.hpp"
#include "clang/AST/Expr.h"

namespace knight::analyzer {

namespace internal {

constexpr unsigned BinaryOperationContextAlignment = 128U;

struct alignas(BinaryOperationContextAlignment) BinaryOperationContext {
    clang::BinaryOperator::Opcode op{};
    // lhs_expr has value or lhs_sexpr has value
    std::optional< const clang::Expr* > lhs_expr;
    std::optional< SExprRef > lhs_sexpr;

    // rhs_expr has value or rhs_sexpr has value
    std::optional< const clang::Expr* > rhs_expr;
    std::optional< SExprRef > rhs_sexpr;

    clang::QualType result_type;
    const clang::Stmt* result_stmt{};
}; // struct BinaryOperationContext

} // namespace internal

class BinaryOpResolver {
    friend class UnaryOpResolver;

  private:
    AnalysisContext* m_ctx;
    const SymbolResolver* m_sym_resolver;

  public:
    explicit BinaryOpResolver(AnalysisContext* ctx,
                              const SymbolResolver* sym_resolver)
        : m_ctx(ctx), m_sym_resolver(sym_resolver) {}

  public:
    void resolve(const clang::BinaryOperator*) const;

  private:
    void handle_binary_operation(internal::BinaryOperationContext) const;
    void handle_assign_binary_operation(internal::BinaryOperationContext) const;
    void handle_int_binary_operation(internal::BinaryOperationContext) const;
    void handle_int_non_assign_binary_operation(
        internal::BinaryOperationContext) const;
    void handle_ref_binary_operation(internal::BinaryOperationContext) const;
    void handle_ptr_binary_operation(internal::BinaryOperationContext) const;

}; // class BinaryOpResolver

} // namespace knight::analyzer