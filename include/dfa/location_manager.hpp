//===- location_manager.hpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file offers the location manager.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/location_context.hpp"
#include "dfa/stack_frame.hpp"

namespace knight::dfa {

class LocationManager {
  private:
    std::unordered_map< const clang::Decl*, ProcCFG::GraphUniqueRef >
        m_decl_to_cfg;

    llvm::BumpPtrAllocator m_allocator;
    llvm::FoldingSet< StackFrame > m_stack_frames;
    llvm::FoldingSet< LocationContext > m_location_contexts;

  public:
    LocationManager() = default;

  public:
    ProcCFG::GraphRef get_cfg(const clang::Decl* decl) const {
        auto it = m_decl_to_cfg.find(decl);
        if (it == m_decl_to_cfg.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    const StackFrame* create_top_frame(ProcCFG::DeclRef decl);
    const StackFrame* create_from_node(StackFrame* parent,
                                       ProcCFG::NodeRef node,
                                       ProcCFG::StmtRef callsite_expr,
                                       unsigned stmt_idx);
    const LocationContext* create_location_context(
        const StackFrame* stack_frame,
        int element_id,
        const clang::CFGBlock* block);
    const LocationContext* create_location_context(
        const StackFrame* stack_frame, const clang::CFGBlock* block) {
        return create_location_context(stack_frame, -1, block);
    }

  private:
    void ensure_cfg_created(ProcCFG::DeclRef decl);
}; // class LocationManager

} // namespace knight::dfa