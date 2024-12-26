//===- driver.hpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the core driver for the Knight cg tool.
//
//===------------------------------------------------------------------===//

#pragma once

#include "cg/core/builder.hpp"
#include "clang/AST/ASTContext.h"
#include "common/util/vfs.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/DeclGroup.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include "common/util/log.hpp"

#include <memory>
#include <utility>

namespace knight {

struct CGContext {
    bool use_color = true;
    bool show_process_sys_function = false;
    bool skip_system_header = false;
    bool skip_implicit_code = false;

    llvm::IntrusiveRefCntPtr< llvm::vfs::OverlayFileSystem > overlay_fs;
    const clang::tooling::CompilationDatabase* cdb;
    std::vector< std::string > input_files;

    std::string knight_dir;
    unsigned db_busy_timeout;
    clang::ASTContext* ast_ctx = nullptr;
    std::string file;
};

class KnightASTConsumer : public clang::ASTConsumer {
  public:
    explicit KnightASTConsumer(CGContext& ctx) : m_ctx(ctx), m_builder(ctx) {}

    bool HandleTopLevelDecl(clang::DeclGroupRef decl_group) override;

  private:
    CGContext& m_ctx;
    cg::CGBuilder m_builder;
}; // class KnightASTConsumer

class KnightIRConsumer {
  private:
    CGContext& m_ctx;

  public:
    explicit KnightIRConsumer(CGContext& ctx);

    /// \brief Create an AST consumer for the given file.
    [[nodiscard]] std::unique_ptr< clang::ASTConsumer > create_ast_consumer(
        clang::CompilerInstance& ci, llvm::StringRef file);
}; // class KnightASTConsumerFactory

class KnightCGBuilder {
  private:
    CGContext& m_ctx;

  public:
    explicit KnightCGBuilder(CGContext& ctx) : m_ctx(ctx) {}

  public:
    void build();
}; // class KnightDriver

} // namespace knight
