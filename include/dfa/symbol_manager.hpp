//===- symbol_manager.hpp 000------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the symbol manager.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/analysis_manager.hpp"
#include "dfa/location_context.hpp"
#include "dfa/region/region.hpp"
#include "dfa/stack_frame.hpp"
#include "symbol.hpp"

namespace knight::dfa {

class SymbolManager {
  private:
    llvm::BumpPtrAllocator& m_allocator;
    llvm::FoldingSet< SymExpr > m_sexpr_set;
    SymID m_sym_cnt = 0U;

  public:
    explicit SymbolManager(llvm::BumpPtrAllocator& allocator)
        : m_allocator(allocator) {}

    [[nodiscard]] const ScalarInt* get_scalar_int(const llvm::APSInt& value,
                                                  clang::QualType type) {
        return get_persistent_sepxr< ScalarInt >(value, type);
    }

    [[nodiscard]] const ScalarFloat* get_scalar_float(
        const llvm::APFloat& value, clang::QualType type) {
        return get_persistent_sepxr< ScalarFloat >(value, type);
    }

    [[nodiscard]] const RegionSymVal* get_region_sym_val(
        const TypedRegion* typed_region, const LocationContext* loc_ctx) {
        m_sym_cnt++;
        const auto* space = typed_region->get_memory_space();
        bool is_external = space == nullptr || space->is_stack_local();
        return get_persistent_sepxr< RegionSymVal >(m_sym_cnt,
                                                    typed_region,
                                                    loc_ctx,
                                                    is_external);
    }

    [[nodiscard]] const SymbolConjured* get_symbol_conjured(
        const clang::Stmt* stmt,
        clang::QualType type,
        const StackFrame* frame,
        const void* tag = nullptr) {
        m_sym_cnt++;
        return get_persistent_sepxr< SymbolConjured >(m_sym_cnt,
                                                      stmt,
                                                      type,
                                                      frame,
                                                      tag);
    }

    [[nodiscard]] const SymbolConjured* get_symbol_conjured(
        const clang::Expr* expr,
        const StackFrame* frame,
        const void* tag = nullptr) {
        return get_symbol_conjured(expr, expr->getType(), frame, tag);
    }

    [[nodiscard]] const BinarySymExpr* get_binary_sym_expr(
        const SymExpr* lhs,
        const SymExpr* rhs,
        clang::BinaryOperator::Opcode op,
        const clang::QualType& type) {
        return get_persistent_sepxr< BinarySymExpr >(lhs, rhs, op, type);
    }

    [[nodiscard]] SExprRef normalize(SExprRef sexpr) {
        if (const auto* binary_sexpr = llvm::dyn_cast< BinarySymExpr >(sexpr)) {
            SExprRef lhs = binary_sexpr->get_lhs();
            SExprRef rhs = binary_sexpr->get_rhs();
            if (lhs->get_kind() == SymExprKind::Int &&
                rhs->get_kind() == SymExprKind::Int) {
                auto lhs_val = llvm::cast< ScalarInt >(lhs)->get_value();
                auto rhs_val = llvm::cast< ScalarInt >(rhs)->get_value();
                switch (binary_sexpr->get_opcode()) {
                    case clang::BO_Add:
                        return get_scalar_int(lhs_val + rhs_val,
                                              sexpr->get_type());
                    case clang::BO_Sub:
                        return get_scalar_int(lhs_val - rhs_val,
                                              sexpr->get_type());
                    case clang::BO_Mul:
                        return get_scalar_int(lhs_val * rhs_val,
                                              sexpr->get_type());
                    case clang::BO_Div:
                        return get_scalar_int(lhs_val / rhs_val,
                                              sexpr->get_type());
                    case clang::BO_Rem:
                        return get_scalar_int(lhs_val % rhs_val,
                                              sexpr->get_type());
                    case clang::BO_And:
                        return get_scalar_int(lhs_val & rhs_val,
                                              sexpr->get_type());
                    case clang::BO_Or:
                        return get_scalar_int(lhs_val | rhs_val,
                                              sexpr->get_type());
                    case clang::BO_Xor:
                        return get_scalar_int(lhs_val ^ rhs_val,
                                              sexpr->get_type());
                    default:
                        break;
                }
            }
        }
        return sexpr;
    }

  private:
    template < typename STy, typename... Args >
    [[nodiscard]] const STy* get_persistent_sepxr(Args&&... args) {
        llvm::FoldingSetNodeID id;
        STy::profile(id, std::forward< Args >(args)...);

        void* insert_pos; // NOLINT
        auto* region = cast_or_null< STy >(
            m_sexpr_set.FindNodeOrInsertPos(id, insert_pos));
        if (region == nullptr) {
            region = new (m_allocator) // NOLINT
                STy(std::forward< Args >(args)...);
            m_sexpr_set.InsertNode(region, insert_pos);
        }
        return static_cast< const STy* >(region);
    }
}; // class SymbolManager

} // namespace knight::dfa
