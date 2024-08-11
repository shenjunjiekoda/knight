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

#include "symbol.hpp"

namespace knight::dfa {

class SymbolManager {
  private:
    llvm::BumpPtrAllocator& m_allocator;
    llvm::FoldingSet< SymExpr > m_sexpr_set;

  public:
    explicit SymbolManager(llvm::BumpPtrAllocator& allocator)
        : m_allocator(allocator) {}

    const ScalarInt* get_scalar_int(const llvm::APSInt& value,
                                    clang::QualType type) {
        return get_persistent_sepxr< ScalarInt >(value, type);
    }

    const ScalarFloat* get_scalar_float(const llvm::APFloat& value,
                                        clang::QualType type) {
        return get_persistent_sepxr< ScalarFloat >(value, type);
    }

  private:
    template < typename STy, typename... Args >
    const STy* get_persistent_sepxr(Args&&... args) {
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
