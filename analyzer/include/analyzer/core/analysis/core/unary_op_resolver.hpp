//===- unary_op_resolver.hpp ------------------------------------------===//
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
#include "analyzer/core/analysis_context.hpp"
#include "clang/AST/Expr.h"

namespace knight::analyzer {

class UnaryOpResolver {
  private:
    AnalysisContext* m_ctx;
    const SymbolResolver* m_sym_resolver;

  public:
    explicit UnaryOpResolver(AnalysisContext* ctx,
                             const SymbolResolver* sym_resolver)
        : m_ctx(ctx), m_sym_resolver(sym_resolver) {}

  public:
    void resolve(const clang::UnaryOperator*) const;

  private:
    void handle_int_unary_operation(const clang::UnaryOperator*) const;
    void handle_ptr_unary_operation(const clang::UnaryOperator*) const;

}; // class UnaryOpResolver

} // namespace knight::analyzer