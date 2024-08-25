//===- proc_cfg.cpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the procedural control flow graph.
//
//===------------------------------------------------------------------===//

#include "dfa/proc_cfg.hpp"
#include "util/assert.hpp"

#include "util/log.hpp"

#include <memory>

namespace knight::dfa {

namespace {

using namespace clang;

auto get_default_cfg_build_options() {
    CFG::BuildOptions cfg_opts;

    cfg_opts.AddInitializers = true;
    cfg_opts.AddImplicitDtors = true;
    cfg_opts.AddTemporaryDtors = true;
    cfg_opts.AddCXXDefaultInitExprInCtors = true;

    cfg_opts.PruneTriviallyFalseEdges = true;
    cfg_opts.AddLifetime = true;

    cfg_opts.setAllAlwaysAdd();

    return cfg_opts;
}

void construct_mapping_for_cfg_stmt(ProcCFG::NodeRef block,
                                    ProcCFG::StmtToBlockMap& stmtToBlock) {
    if (block == nullptr) {
        return;
    }
    for (const auto& elem : *block) {
        if (auto cfg_stmt_opt = elem.getAs< CFGStmt >()) {
            stmtToBlock[cfg_stmt_opt->getStmt()] = block;
        }
    }
}

void construct_mapping_for_term_condition(
    ProcCFG::NodeRef block, ProcCFG::StmtToBlockMap& stmtToBlock) {
    if (block == nullptr) {
        return;
    }
    if (const auto* term_cond_stmt = block->getTerminatorCondition()) {
        // does nothing if the key already exists in the map.
        stmtToBlock.insert({term_cond_stmt, block});
    }
}

void construct_mapping_for_term_stmt(ProcCFG::NodeRef block,
                                     ProcCFG::StmtToBlockMap& stmtToBlock) {
    if (block == nullptr) {
        return;
    }
    if (const auto* term_stmt = block->getTerminatorStmt()) {
        stmtToBlock.insert({term_stmt, block});
    }
}

/// \brief Returns a map from statements to basic blocks that contain them.
ProcCFG::StmtToBlockMap construct_stmt_block_mapping(const CFG& clang_cfg) {
    ProcCFG::StmtToBlockMap mapping;
    for (const auto* block : clang_cfg) {
        construct_mapping_for_cfg_stmt(block, mapping);
    }
    for (const auto* block : clang_cfg) {
        construct_mapping_for_cfg_stmt(block, mapping);
    }
    for (const auto* block : clang_cfg) {
        construct_mapping_for_term_stmt(block, mapping);
    }
    return mapping;
}

llvm::BitVector get_reachable_blocks(const CFG& clang_cfg) {
    llvm::BitVector reachable_blocks(clang_cfg.getNumBlockIDs(), false);
    llvm::SmallVector< ProcCFG::NodeRef > visited;
    visited.push_back(&clang_cfg.getEntry());
    while (!visited.empty()) {
        const auto* block = visited.back();
        visited.pop_back();
        if (reachable_blocks[block->getBlockID()]) {
            continue;
        }
        reachable_blocks[block->getBlockID()] = true;
        for (ProcCFG::NodeRef succ : block->succs()) {
            if (succ != nullptr) {
                visited.push_back(succ);
            }
        }
    }
    return reachable_blocks;
}

} // anonymous namespace

ProcCFG::NodeRef ProcCFG::entry(GraphRef cfg) {
    return &(cfg->m_cfg->getEntry());
}

ProcCFG::NodeRef ProcCFG::exit(GraphRef cfg) {
    return &(cfg->m_cfg->getExit());
}

ProcCFG::GraphUniqueRef ProcCFG::build(const clang::Decl* function) {
    knight_assert_msg(function != nullptr, "function provided is null");
    auto* body = function->getBody();
    knight_assert_msg(body != nullptr, "function shall have body");

    return build(function, body, function->getASTContext());
}

ProcCFG::GraphUniqueRef ProcCFG::build(FunctionRef function,
                                       clang::Stmt* build_scope,
                                       clang::ASTContext& ctx) {
    knight_assert_msg(!function->isTemplated(),
                      "templated function not supported");
    knight_assert_msg(!ctx.getLangOpts().ObjC, "objective-c not supported");

    auto cfg = clang::CFG::buildCFG(function,
                                    build_scope,
                                    &ctx,
                                    get_default_cfg_build_options());
    knight_assert_msg(cfg != nullptr, "failed to build CFG");

    auto stmt_block_mapping = construct_stmt_block_mapping(*cfg);
    auto reachable_blocks = get_reachable_blocks(*cfg);
    return std::unique_ptr< ProcCFG >(new ProcCFG(function,
                                                  std::move(cfg),
                                                  std::move(stmt_block_mapping),
                                                  std::move(reachable_blocks)));
}

void ProcCFG::dump(llvm::raw_ostream& os, bool show_colors) const {
    m_cfg->print(os, m_proc->getLangOpts(), show_colors);
}

void ProcCFG::view() const {
    m_cfg->viewCFG(m_proc->getLangOpts());
}

ProcCFG::ProcCFG(FunctionRef proc,
                 ClangCFGRef cfg,
                 StmtToBlockMap stmt_to_block,
                 llvm::BitVector reachable_block)
    : m_proc(proc),
      m_cfg(std::move(cfg)),
      m_stmt_to_block(std::move(stmt_to_block)),
      m_reachable_block(std::move(reachable_block)) {}

} // namespace knight::dfa