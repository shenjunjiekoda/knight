//===- location_manager.cpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the location manager.
//
//===------------------------------------------------------------------===//

#include "dfa/location_manager.hpp"
#include "dfa/location_context.hpp"

namespace knight::dfa {

LocationContext::LocationContext(const LocationManager* manager,
                                 const StackFrame* stack_frame,
                                 unsigned element_id,
                                 const clang::CFGBlock* block)
    : m_location_manager(manager),
      m_stack_frame(stack_frame),
      m_element_id(element_id),
      m_block(block) {}

const LocationManager* LocationContext::get_location_manager() const {
    return m_location_manager;
}

void LocationContext::profile(llvm::FoldingSetNodeID& id,
                              const StackFrame* stack_frame,
                              unsigned element_id,
                              const clang::CFGBlock* block) {
    id.AddPointer(stack_frame);
    id.AddInteger(element_id);
    id.AddPointer(block);
}

void LocationContext::Profile(llvm::FoldingSetNodeID& id) const {
    profile(id, m_stack_frame, m_element_id, m_block);
}

const StackFrame* LocationManager::create_top_frame(ProcCFG::DeclRef decl) {
    llvm::FoldingSetNodeID id;
    StackFrame::profile(id, decl, nullptr, CallSiteInfo());

    void* insert_pos; // NOLINT
    auto* res = m_stack_frames.FindNodeOrInsertPos(id, insert_pos);
    if (res == nullptr) {
        res = m_allocator.Allocate< StackFrame >();
        new (res) StackFrame(this, decl, nullptr, CallSiteInfo());
        m_stack_frames.InsertNode(res, insert_pos);
    }
    ensure_cfg_created(decl);

    return res;
}

const StackFrame* LocationManager::create_from_node(
    StackFrame* parent,
    ProcCFG::NodeRef node,
    ProcCFG::StmtRef callsite_expr,
    unsigned stmt_idx) {
    llvm::FoldingSetNodeID id;
    auto called_decl_opt = get_called_decl(callsite_expr);
    knight_assert_msg(called_decl_opt.has_value(), "invalid call site");
    const CallSiteInfo callsite_info(callsite_expr, node, stmt_idx);
    StackFrame::profile(id, *called_decl_opt, parent, callsite_info);

    void* insert_pos; // NOLINT
    auto* res = m_stack_frames.FindNodeOrInsertPos(id, insert_pos);
    if (res == nullptr) {
        res = m_allocator.Allocate< StackFrame >();
        new (res) StackFrame(this, *called_decl_opt, parent, callsite_info);
        m_stack_frames.InsertNode(res, insert_pos);
    }
    ensure_cfg_created(*called_decl_opt);

    return res;
}

const LocationContext* LocationManager::create_location_context(
    const StackFrame* stack_frame,
    unsigned element_id,
    const clang::CFGBlock* block) {
    llvm::FoldingSetNodeID id;
    LocationContext::profile(id, stack_frame, element_id, block);

    void* insert_pos; // NOLINT
    auto* res = m_location_contexts.FindNodeOrInsertPos(id, insert_pos);
    if (res == nullptr) {
        res = m_allocator.Allocate< LocationContext >();
        new (res) LocationContext(this, stack_frame, element_id, block);
        m_location_contexts.InsertNode(res, insert_pos);
    }

    return res;
}

void LocationManager::ensure_cfg_created(ProcCFG::DeclRef decl) {
    if (m_decl_to_cfg.contains(decl)) {
        return;
    }
    m_decl_to_cfg[decl] = ProcCFG::build(decl);
}

} // namespace knight::dfa