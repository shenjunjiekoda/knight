//===- stack_frame.hpp ------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file offers the stack frame of current analysis or checker.
//
//===------------------------------------------------------------------===//

#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclBase.h>
#include <llvm/ADT/FoldingSet.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/raw_ostream.h>

#include "dfa/proc_cfg.hpp"
#include "util/assert.hpp"

namespace knight::dfa {

class StackFrameManager;

std::optional< ProcCFG::DeclRef > get_called_decl(const ProcCFG::StmtRef& call);

struct CallSiteInfo {
    ProcCFG::StmtRef callsite_expr;
    ProcCFG::NodeRef node;
    unsigned stmt_idx; // index of the call site in the node
};                     // struct CallSiteInfo

class StackFrame : public llvm::FoldingSetNode {
  private:
    StackFrameManager* m_manager;
    const clang::Decl* m_decl;
    StackFrame* m_parent{};
    CallSiteInfo m_call_site_info;

  public:
    StackFrame(StackFrameManager* manager,
               const clang::Decl* decl,
               StackFrame* parent,
               CallSiteInfo call_site_info);
    StackFrame(const StackFrame&);
    StackFrame& operator=(const StackFrame&) = delete;
    ~StackFrame() = default;

  public:
    ProcCFG::GraphRef get_cfg() const;

    clang::ASTContext& get_ast_context() const {
        return m_decl->getASTContext();
    }
    const clang::Decl* get_decl() const { return m_decl; }
    StackFrameManager* get_manager() const;
    StackFrame* get_parent() const { return m_parent; }
    bool isTopFrame() const { return m_parent == nullptr; }

    const CallSiteInfo& get_call_site_info() const {
        knight_assert_msg(!isTopFrame(), "top frame has no call site info");
        return m_call_site_info;
    }

    ProcCFG::StmtRef get_callsite_expr() const;
    ProcCFG::NodeRef get_callsite_node() const;
    clang::CFGElement get_callsite_cfg_element() const;

    bool is_ancestor_of(const StackFrame* other) const;

  public:
    void Profile(llvm::FoldingSetNodeID& id) const;
    void dump(llvm::raw_ostream& os) const;

}; // class StackFrame

class StackFrameManager {
  private:
    std::unordered_map< const clang::Decl*, ProcCFG::GraphUniqueRef >
        m_decl_to_cfg;

    llvm::BumpPtrAllocator m_allocator;
    llvm::FoldingSet< StackFrame > m_stack_frames;

  public:
    StackFrameManager() = default;

  public:
    ProcCFG::GraphRef get_cfg(const clang::Decl* decl) const {
        auto it = m_decl_to_cfg.find(decl);
        if (it == m_decl_to_cfg.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    StackFrame* create_top_frame(ProcCFG::DeclRef decl);
    StackFrame* create_from_node(StackFrame* parent,
                                 ProcCFG::NodeRef node,
                                 ProcCFG::StmtRef callsite_expr,
                                 unsigned stmt_idx);

  private:
    void ensure_cfg_created(ProcCFG::DeclRef decl);
}; // class StackFrameManager

} // namespace knight::dfa