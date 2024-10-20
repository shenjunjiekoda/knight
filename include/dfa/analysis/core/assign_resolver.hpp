//===- assign_resolver.hpp --------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the assign resolver.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/analysis/core/symbol_resolver.hpp"
#include "dfa/analysis_context.hpp"
namespace knight::dfa {

namespace internal {

constexpr unsigned AssignmentContextAlignment = 64U;

struct alignas(AssignmentContextAlignment)
    AssignmentContext { // NOLINT(altera-struct-pack-align)
    std::optional< const TypedRegion* > treg = std::nullopt;
    std::optional< const clang::Stmt* > stmt = std::nullopt;
    std::optional< const clang::Expr* > rhs_expr = std::nullopt;
    SExprRef lhs_sexpr{};
    SExprRef rhs_sexpr{};
    clang::BinaryOperator::Opcode op = clang::BinaryOperator::Opcode::BO_Assign;

    void dump(llvm::raw_ostream& os) const {
        os << "AssignmentContext: ";
        os << "treg:{";
        if (treg) {
            treg.value()->dump(os);
        } else {
            os << "null";
        }
        os << "}, stmt:{";
        if (stmt) {
            stmt.value()->dump();
        } else {
            os << "null";
        }
        os << "}, rhs_expr:{";
        if (rhs_expr) {
            rhs_expr.value()->dump();
        } else {
            os << "null";
        }
        os << "}, lhs_sexpr:{";
        lhs_sexpr->dump(os);
        os << "}, rhs_sexpr:{";
        rhs_sexpr->dump(os);
        os << "}, op:" << clang::BinaryOperator::getOpcodeStr(op);
    }
}; // struct AssignmentContext

} // namespace internal

class AssignResolver {
  private:
    AnalysisContext* m_ctx;
    const SymbolResolver* m_sym_resolver;

  public:
    AssignResolver(AnalysisContext* ctx, const SymbolResolver* sym_resolver)
        : m_ctx(ctx), m_sym_resolver(sym_resolver) {}

  public:
    [[nodiscard]] ProgramStateRef resolve(internal::AssignmentContext) const;

  private:
    void handle_int_assign(internal::AssignmentContext assign_ctx,
                           SymbolRef res_sym,
                           bool is_direct_assign,
                           clang::BinaryOperator::Opcode op,
                           ProgramStateRef& state,
                           SExprRef& binary_sexpr) const;
    void handle_ptr_assign(internal::AssignmentContext assign_ctx,
                           SymbolRef res_sym,
                           bool is_direct_assign,
                           clang::BinaryOperator::Opcode op,
                           ProgramStateRef& state,
                           SExprRef& binary_sexpr) const;
}; // class AssignResolver

} // namespace knight::dfa