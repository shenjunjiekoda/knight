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

class LocationManager;

std::optional< ProcCFG::DeclRef > get_called_decl(const ProcCFG::StmtRef& call);

constexpr unsigned CallSiteInfoAlignment = 32;

struct CallSiteInfo {
    ProcCFG::StmtRef callsite_expr;
    ProcCFG::NodeRef node;
    unsigned stmt_idx; // index of the call site in the node
} __attribute__((aligned(CallSiteInfoAlignment)))
__attribute__((packed)); // struct CallSiteInfo

class StackFrame : public llvm::FoldingSetNode {
  private:
    LocationManager* m_manager;
    const clang::Decl* m_decl;
    StackFrame* m_parent{};
    CallSiteInfo m_call_site_info;

  public:
    StackFrame(LocationManager* manager,
               [[gnu::nonnull]] const clang::Decl* decl,
               StackFrame* parent,
               CallSiteInfo call_site_info);
    StackFrame(const StackFrame&) = delete;
    StackFrame& operator=(const StackFrame&) = delete;
    ~StackFrame() = default;

  public:
    [[nodiscard]] ProcCFG::GraphRef get_cfg() const;

    [[nodiscard]] clang::ASTContext& get_ast_context() const {
        return m_decl->getASTContext();
    }

    [[gnu::returns_nonnull, nodiscard]] const clang::Decl* get_decl() const {
        return m_decl;
    }

    [[gnu::returns_nonnull, nodiscard]] LocationManager* get_manager() const;

    [[nodiscard]] StackFrame* get_parent() const { return m_parent; }
    [[nodiscard]] bool is_top_frame() const { return m_parent == nullptr; }

    [[nodiscard]] const CallSiteInfo& get_call_site_info() const {
        knight_assert_msg(!is_top_frame(), "top frame has no call site info");
        return m_call_site_info;
    }

    [[nodiscard]] ProcCFG::StmtRef get_callsite_expr() const;
    [[nodiscard]] ProcCFG::NodeRef get_callsite_node() const;
    [[nodiscard]] clang::CFGElement get_callsite_cfg_element() const;

    bool is_ancestor_of(const StackFrame* other) const;

  public:
    static void profile(llvm::FoldingSetNodeID& id,
                        const clang::Decl* decl,
                        StackFrame* parent,
                        const CallSiteInfo& call_site_info);
    void Profile(llvm::FoldingSetNodeID& id) const; // NOLINT
    void dump(llvm::raw_ostream& os) const;

}; // class StackFrame

} // namespace knight::dfa