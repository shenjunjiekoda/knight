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

#include "analyzer/core/analysis_manager.hpp"
#include "analyzer/core/checker_manager.hpp"
#include "analyzer/core/engine/intraprocedural_fixpoint.hpp"
#include "analyzer/core/location_manager.hpp"
#include "analyzer/core/proc_cfg.hpp"
#include "analyzer/core/program_state.hpp"
#include "analyzer/core/region/region.hpp"
#include "analyzer/core/symbol_manager.hpp"
#include "analyzer/tooling/context.hpp"
#include "analyzer/tooling/diagnostic.hpp"
#include "analyzer/tooling/factory.hpp"
#include "common/util/vfs.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/DeclGroup.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/Support/Casting.h>
#include "common/util/log.hpp"

#include <memory>
#include <utility>

namespace knight {

class KnightASTConsumer : public clang::ASTConsumer {
  public:
    KnightASTConsumer(KnightContext& ctx,
                      analyzer::AnalysisManager& analysis_manager,
                      analyzer::CheckerManager& checker_manager,
                      KnightFactory::CheckerRefs checkers,
                      KnightFactory::AnalysisRefs analysis)
        : m_ctx(ctx),
          m_analysis_manager(analysis_manager),
          m_checker_manager(checker_manager),
          m_checkers(std::move(checkers)),
          m_analysis(std::move(analysis)) {}

    // TODO(engine): add datadflow engine to run analysis and checkers here? on
    // the decl_group or tu?
    bool HandleTopLevelDecl(clang::DeclGroupRef decl_group) override {
        for (auto* decl : decl_group) {
            auto* function =
                llvm::dyn_cast_or_null< clang::FunctionDecl >(decl);
            if (function == nullptr || !function->hasBody()) {
                continue;
            }

            llvm::outs() << "[*] Processing function: ";
            if (m_ctx.get_current_options().use_color) {
                llvm::outs().changeColor(llvm::raw_ostream::Colors::GREEN);
            }
            function->printName(llvm::outs());
            llvm::outs().resetColor();
            llvm::outs() << "\n";

            const auto* frame = m_location_manager.create_top_frame(function);
            if (m_ctx.get_current_options().view_cfg) {
                if (m_ctx.get_current_options().use_color) {
                    llvm::outs().changeColor(llvm::raw_ostream::Colors::RED);
                }
                llvm::outs() << "viewing CFG:\n";
                llvm::outs().resetColor();
                frame->get_cfg()->view();
            }

            if (m_ctx.get_current_options().dump_cfg) {
                if (m_ctx.get_current_options().use_color) {
                    llvm::outs().changeColor(llvm::raw_ostream::Colors::RED);
                }
                llvm::outs() << "dumping CFG:\n";
                llvm::outs().resetColor();

                frame->get_cfg()->dump(llvm::outs(),
                                       m_ctx.get_current_options().use_color);
            }

            analyzer::IntraProceduralFixpointIterator
                engine(m_ctx,
                       m_analysis_manager,
                       m_checker_manager,
                       m_location_manager,
                       m_analysis_manager.get_state_manager(),
                       frame);
            engine.run();
        }

        return true;
    }

  private:
    KnightContext& m_ctx;
    analyzer::AnalysisManager& m_analysis_manager;
    analyzer::CheckerManager& m_checker_manager;
    KnightFactory::CheckerRefs m_checkers;
    KnightFactory::AnalysisRefs m_analysis;
    analyzer::LocationManager m_location_manager;
}; // class KnightASTConsumer

class KnightASTConsumerFactory {
  private:
    KnightContext& m_ctx;
    std::unique_ptr< KnightFactory > m_factory;
    std::unique_ptr< analyzer::AnalysisManager > m_analysis_manager;
    std::unique_ptr< analyzer::CheckerManager > m_checker_manager;

  public:
    explicit KnightASTConsumerFactory(
        KnightContext& ctx,
        std::unique_ptr< analyzer::AnalysisManager > external_analysis_manager =
            nullptr,
        std::unique_ptr< analyzer::CheckerManager > external_checker_manager =
            nullptr);

    /// \brief Create an AST consumer for the given file.
    [[nodiscard]] std::unique_ptr< clang::ASTConsumer > create_ast_consumer(
        clang::CompilerInstance& ci, llvm::StringRef file);

    /// \brief Get the list of enabled checks.
    [[nodiscard]] std::vector<
        std::pair< analyzer::CheckerID, llvm::StringRef > >
    get_enabled_checks() const;

    /// \brief Get the list of directly enabled analyses.
    [[nodiscard]] std::vector<
        std::pair< analyzer::AnalysisID, llvm::StringRef > >
    get_directly_enabled_analyses() const;

    /// \brief Get the list of enabled core analyses.
    [[nodiscard]] std::vector<
        std::pair< analyzer::AnalysisID, llvm::StringRef > >
    get_enabled_core_analyses() const;

}; // class KnightASTConsumerFactory

class KnightDriver {
  private:
    KnightContext& m_ctx;
    const clang::tooling::CompilationDatabase& m_cdb;
    std::vector< std::string > m_input_files;
    llvm::IntrusiveRefCntPtr< llvm::vfs::OverlayFileSystem > m_base_fs;

  public:
    KnightDriver(
        KnightContext& ctx,
        const clang::tooling::CompilationDatabase& cdb,
        std::vector< std::string > input_files,
        llvm::IntrusiveRefCntPtr< llvm::vfs::OverlayFileSystem > base_fs)
        : m_ctx(ctx),
          m_cdb(cdb),
          m_input_files(std::move(input_files)),
          m_base_fs(std::move(base_fs)) {}

  public:
    std::vector< KnightDiagnostic > run();

    void handle_diagnostics(const std::vector< KnightDiagnostic >& diagnostics,
                            bool try_fix);

}; // class KnightDriver

} // namespace knight