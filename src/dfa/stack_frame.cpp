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
#include "dfa/location_manager.hpp"

#include <clang/AST/ExprCXX.h>
#include <llvm/ADT/FoldingSet.h>

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

StackFrame::StackFrame(LocationManager* manager,
                       const clang::Decl* decl,
                       StackFrame* parent,
                       CallSiteInfo call_site_info)
    : m_manager(manager),
      m_decl(decl),
      m_parent(parent),
      m_call_site_info(call_site_info) {}

void StackFrame::profile(llvm::FoldingSetNodeID& id,
                         const clang::Decl* decl,
                         StackFrame* parent,
                         const CallSiteInfo& call_site_info) {
    id.AddPointer(decl);
    id.AddPointer(parent);
    if (parent == nullptr) {
        return;
    }
    id.AddPointer(call_site_info.callsite_expr);
    id.AddPointer(call_site_info.node);
    id.AddInteger(call_site_info.stmt_idx);
}

void StackFrame::Profile(llvm::FoldingSetNodeID& id) const {
    profile(id, m_decl, m_parent, m_call_site_info);
}

LocationManager* StackFrame::get_manager() const {
    return m_manager;
}

ProcCFG::GraphRef StackFrame::get_cfg() const {
    return m_manager->get_cfg(m_decl);
}

ProcCFG::StmtRef StackFrame::get_callsite_expr() const {
    knight_assert_msg(!is_top_frame(), "top frame has no call site info");
    return m_call_site_info.callsite_expr;
}

ProcCFG::NodeRef StackFrame::get_callsite_node() const {
    knight_assert_msg(!is_top_frame(), "top frame has no call site info");
    return m_call_site_info.node;
}

clang::CFGElement StackFrame::get_callsite_cfg_element() const {
    knight_assert_msg(!is_top_frame(), "top frame has no call site info");
    return (*m_call_site_info.node)[m_call_site_info.stmt_idx];
}

bool StackFrame::is_ancestor_of(const StackFrame* other) const {
    do {
        const auto* pa = other->m_parent;
        if (pa == this) {
            return true;
        }
        other = pa;
    } while (!other->is_top_frame());
    return false;
}

void StackFrame::dump(llvm::raw_ostream& os) const {
    unsigned frame_depth = 0U;
    for (const auto* frame = this; !frame->is_top_frame();
         frame = frame->m_parent) {
        ++frame_depth;
    };

    unsigned frame_idx = 0U;
    for (const auto* frame = this; !frame->is_top_frame();
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

} // namespace knight::dfa