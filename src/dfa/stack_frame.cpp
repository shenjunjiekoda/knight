//===- stack_frame.cpp ------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the stack frame of current analysis or checker.
//
//===------------------------------------------------------------------===//

#include "dfa/stack_frame.hpp"
#include "clang/AST/ExprCXX.h"
#include "llvm/ADT/FoldingSet.h"

namespace knight::dfa {

std::optional< ProcCFG::DeclRef > get_called_decl(
    const ProcCFG::StmtRef& call) {
    if (const auto* call_expr = dyn_cast< const clang::CallExpr >(call)) {
        if (const auto* callee = call_expr->getCalleeDecl()) {
            return callee;
        }
    }

    if (const auto* ctor_call_expr =
            dyn_cast< const clang::CXXConstructExpr >(call)) {
        if (const auto* callee = ctor_call_expr->getConstructor()) {
            return callee;
        }
    }
    return std::nullopt;
}

StackFrame::StackFrame(StackFrameManager* manager,
                       const clang::Decl* decl,
                       StackFrame* parent,
                       CallSiteInfo call_site_info)
    : m_manager(manager), m_decl(decl), m_parent(parent),
      m_call_site_info(std::move(call_site_info)) {}

StackFrame::StackFrame(const StackFrame& other) = default;

void StackFrame::Profile(llvm::FoldingSetNodeID& id) const {
    id.AddPointer(m_decl);
    id.AddPointer(m_parent);
    if (m_parent == nullptr) {
        return;
    }
    id.AddPointer(m_call_site_info.callsite_expr);
    id.AddPointer(m_call_site_info.node);
    id.AddInteger(m_call_site_info.stmt_idx);
}

StackFrameManager* StackFrame::get_manager() const {
    return m_manager;
}

ProcCFG::GraphRef StackFrame::get_cfg() const {
    return m_manager->get_cfg(m_decl);
}

ProcCFG::StmtRef StackFrame::get_callsite_expr() const {
    knight_assert_msg(!isTopFrame(), "top frame has no call site info");
    return m_call_site_info.callsite_expr;
}

ProcCFG::NodeRef StackFrame::get_callsite_node() const {
    knight_assert_msg(!isTopFrame(), "top frame has no call site info");
    return m_call_site_info.node;
}

clang::CFGElement StackFrame::get_callsite_cfg_element() const {
    knight_assert_msg(!isTopFrame(), "top frame has no call site info");
    return (*m_call_site_info.node)[m_call_site_info.stmt_idx];
}

bool StackFrame::is_ancestor_of(const StackFrame* other) const {
    do {
        const auto* pa = other->m_parent;
        if (pa == this) {
            return true;
        }
        other = pa;
    } while (!other->isTopFrame());
    return false;
}

void StackFrame::dump(llvm::raw_ostream& os) const {
    unsigned frame_depth = 0U;
    for (const auto* frame = this; !frame->isTopFrame();
         frame = frame->m_parent) {
        ++frame_depth;
    };

    unsigned frame_idx = 0U;
    for (const auto* frame = this; !frame->isTopFrame();
         frame = frame->m_parent) {
        if (frame_idx == 0U) {
            os << "->";
        } else {
            os << "  ";
        }
        os << " #" << (frame_depth - frame_idx - 1) << ": ";
        get_callsite_expr()->printPretty(os,
                                         nullptr,
                                         get_ast_context().getPrintingPolicy());
        os << ", calling: `";
        if (const auto* named = dyn_cast< clang::NamedDecl >(m_decl)) {
            named->printName(os);
        } else {
            os << "anonymous code";
        }
        os << "'\n";
        ++frame_idx;
    }
}

StackFrame* StackFrameManager::create_top_frame(ProcCFG::DeclRef decl) {
    llvm::FoldingSetNodeID id;
    StackFrame frame(this, decl, nullptr, CallSiteInfo());
    frame.Profile(id);

    void* insert_pos;
    auto* res = m_stack_frames.FindNodeOrInsertPos(id, insert_pos);
    if (res == nullptr) {
        res = m_allocator.Allocate< StackFrame >();
        new (res) StackFrame(this, decl, nullptr, CallSiteInfo());
        m_stack_frames.InsertNode(res, insert_pos);
    }
    ensure_cfg_created(decl);

    return res;
}

StackFrame* StackFrameManager::create_from_node(StackFrame* parent,
                                                ProcCFG::NodeRef node,
                                                ProcCFG::StmtRef callsite_expr,
                                                unsigned stmt_idx) {
    llvm::FoldingSetNodeID id;
    auto called_decl_opt = get_called_decl(callsite_expr);
    knight_assert_msg(called_decl_opt.has_value(), "invalid call site");
    StackFrame frame(this,
                     *called_decl_opt,
                     parent,
                     CallSiteInfo(callsite_expr, node, stmt_idx));
    frame.Profile(id);

    void* insert_pos;
    auto* res = m_stack_frames.FindNodeOrInsertPos(id, insert_pos);
    if (res == nullptr) {
        res = m_allocator.Allocate< StackFrame >();
        new (res) StackFrame(frame);
        m_stack_frames.InsertNode(res, insert_pos);
    }
    ensure_cfg_created(*called_decl_opt);

    return res;
}

void StackFrameManager::ensure_cfg_created(ProcCFG::DeclRef decl) {
    if (m_decl_to_cfg.contains(decl)) {
        return;
    }
    m_decl_to_cfg[decl] = ProcCFG::build(decl);
}

} // namespace knight::dfa