//===- proc_cfg.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file offers the simple CFG wrapper of a clang cfg.
//
//===------------------------------------------------------------------===//

#pragma once

#include <clang/Analysis/CFG.h>
#include <clang/AST/Decl.h>

#include <llvm/ADT/BitVector.h>
#include <llvm/Support/raw_ostream.h>

#include "support/dumpable.hpp"
#include "support/graph.hpp"

namespace knight::dfa {

/// \brief The simple procedural CFG wrapper of a clang::CFG.
class ProcCFG {
  public:
    using GraphRef = ProcCFG*;
    using ClangCFGRef = std::unique_ptr< clang::CFG >;
    using FunctionRef = const clang::Decl*;
    using NodeRef = const clang::CFGBlock*;
    using StmtRef = const clang::Stmt*;
    using PredNodeIterator = clang::CFGBlock::const_pred_iterator;
    using SuccNodeIterator = clang::CFGBlock::const_succ_iterator;
    using StmtToBlockMap = std::unordered_map< StmtRef, NodeRef >;

  private:
    /// \brief internal clang cfg
    ClangCFGRef m_cfg;
    /// \brief the function of the cfg
    FunctionRef m_proc;
    /// \brief the stmt and its corresponding block
    StmtToBlockMap m_stmt_to_block;
    /// \brief the cfg-syntax-level reachable blocks
    llvm::BitVector m_reachable_block;

  public:
    /// \brief build a procedural CFG from a clang function declaration.
    static GraphRef build(const clang::FunctionDecl* function);

    /// \brief build a procedural CFG from a clang declaration and its body.
    static GraphRef build(FunctionRef function,
                          clang::Stmt* build_scope,
                          clang::ASTContext& ctx);

  public:
    /// \brief get the entry node of the CFG.
    static NodeRef entry(GraphRef cfg);

    /// \brief get the exit node of the CFG.
    static NodeRef exit(GraphRef cfg);

    /// \brief iterate over all the nodes of the CFG.
    /// @{
    static PredNodeIterator pred_begin(NodeRef node) {
        return node->pred_begin();
    }
    static PredNodeIterator pred_end(NodeRef node) { return node->pred_end(); }
    static SuccNodeIterator succ_begin(NodeRef node) {
        return node->succ_begin();
    }
    static SuccNodeIterator succ_end(NodeRef node) { return node->succ_end(); }
    /// }@

    /// \brief dump the procedural CFG for debugging.
    void dump(llvm::raw_ostream& os, bool show_colors = false) const;

  private:
    /// \brief private constructor
    ProcCFG(FunctionRef proc,
            ClangCFGRef cfg,
            StmtToBlockMap stmt_to_block,
            llvm::BitVector reachable_block);
}; // class ProcCFG

static_assert(is_dumpable< ProcCFG >::value, "ProcCFG must be dumpable");
static_assert(isa_graph< ProcCFG >::value, "ProcCFG must be a graph");

} // namespace knight::dfa