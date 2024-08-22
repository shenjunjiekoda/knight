//===- location_context.hpp
//------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file offers the location context.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/stack_frame.hpp"

#include <llvm/ADT/FoldingSet.h>

namespace knight::dfa {

class LocationContext : public llvm::FoldingSetNode {
  private:
    const LocationManager* m_location_manager;
    const StackFrame* m_stack_frame;
    /// \brief -1 or >= 0, -1 means the start point of the block
    int m_element_id;
    const clang::CFGBlock* m_block;

  public:
    LocationContext([[gnu::nonnull]] const LocationManager* manager,
                    [[gnu::nonnull]] const StackFrame* stack_frame,
                    int element_id,
                    [[gnu::nonnull]] const clang::CFGBlock* block);

    ~LocationContext() = default;

  public:
    [[gnu::returns_nonnull, nodiscard]] const StackFrame* get_stack_frame()
        const {
        return m_stack_frame;
    }

    [[gnu::returns_nonnull, nodiscard]] const LocationManager*
    get_location_manager() const;
    [[nodiscard]] bool is_element() const { return m_element_id >= 0; }
    [[nodiscard]] bool is_block_start() const { return m_element_id == -1; }
    [[nodiscard]] int get_element_id() const { return m_element_id; }

    [[gnu::returns_nonnull, nodiscard]] const clang::CFGBlock* get_block()
        const {
        return m_block;
    }

    [[nodiscard]] clang::CFGElement get_element() const {
        knight_assert(is_element());
        return m_block->Elements[m_element_id];
    }

    [[nodiscard]] bool is_stmt() const {
        return get_element().getKind() == clang::CFGElement::Statement;
    }

    template < typename ElemTy >
    [[nodiscard]] std::optional< ElemTy > get_elem_as() const {
        return get_element().getAs< ElemTy >();
    }

    [[nodiscard]] const clang::Stmt* get_stmt() const {
        auto stmt_opt = get_elem_as< clang::CFGStmt >();
        if (stmt_opt) {
            return stmt_opt.value().getStmt();
        }
        return nullptr;
    }
    static void profile(llvm::FoldingSetNodeID& id,
                        const StackFrame* stack_frame,
                        unsigned element_id,
                        const clang::CFGBlock* block);

    void Profile(llvm::FoldingSetNodeID& id) const; // NOLINT

}; // class LocationContext

} // namespace knight::dfa