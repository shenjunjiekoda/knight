//===- knight.hpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the core driver for the Knight tool.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/analysis_manager.hpp"
#include "dfa/checker_manager.hpp"
#include "tooling/context.hpp"
#include "tooling/factory.hpp"
#include "util/vfs.hpp"

#include <clang/Frontend/CompilerInstance.h>
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/DeclGroup.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>

namespace knight {

class KnightASTConsumer : public clang::ASTConsumer {
  public:
    KnightASTConsumer(KnightContext& ctx,
                      dfa::AnalysisManager& analysis_manager,
                      dfa::CheckerManager& checker_manager,
                      KnightFactory::CheckerRefs checkers,
                      KnightFactory::AnalysisRefs analysis)
        : m_ctx(ctx), m_analysis_manager(analysis_manager),
          m_checker_manager(checker_manager), m_checkers(std::move(checkers)),
          m_analysis(std::move(analysis)) {}

    bool HandleTopLevelDecl(clang::DeclGroupRef decl_group) override {
        llvm::outs() << "KnightASTConsumer::HandleTopLevelDecl\n";

        return true;
    }

  private:
    KnightContext& m_ctx;
    dfa::AnalysisManager& m_analysis_manager;
    dfa::CheckerManager& m_checker_manager;
    KnightFactory::CheckerRefs m_checkers;
    KnightFactory::AnalysisRefs m_analysis;
}; // class KnightASTConsumer

class KnightASTConsumerFactory {
  private:
    KnightContext& m_ctx;
    std::unique_ptr< KnightFactory > m_factory;
    std::unique_ptr< dfa::AnalysisManager > m_analysis_manager;
    std::unique_ptr< dfa::CheckerManager > m_checker_manager;

  public:
    KnightASTConsumerFactory(
        KnightContext& ctx,
        std::unique_ptr< dfa::AnalysisManager > external_analysis_manager =
            nullptr,
        std::unique_ptr< dfa::CheckerManager > external_checker_manager =
            nullptr);

    /// \brief Create an AST consumer for the given file.
    std::unique_ptr< clang::ASTConsumer > create_ast_consumer(
        clang::CompilerInstance& ci, llvm::StringRef file);

    /// \brief Get the list of enabled checks.
    std::vector< std::pair< dfa::CheckerID, llvm::StringRef > >
    get_enabled_checks() const;

    /// \brief Get the list of directly enabled analyses.
    std::vector< std::pair< dfa::AnalysisID, llvm::StringRef > >
    get_directly_enabled_analyses() const;
}; // class KnightASTConsumerFactory

} // namespace knight
