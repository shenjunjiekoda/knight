//===- context.hpp ----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the knight context by the knight library and
//  command-line tools.
//
//===------------------------------------------------------------------===//

#pragma once

#include "tooling/options.hpp"
#include "util/globs.hpp"
#include "clang/Basic/SourceManager.h"

#include <llvm/ADT/StringRef.h>

#include <clang/Basic/LangOptions.h>
#include <clang/AST/ASTContext.h>
#include <clang/Tooling/Core/Diagnostic.h>

#include <unordered_map>
#include <memory>
#include <string>

namespace knight {

class KnightContext {

  private:
    /// \brief The diagnostic engine used to diagnose errors.
    clang::DiagnosticsEngine* m_diag_engine;

    std::unique_ptr< KnightOptionsProvider > m_opts_provider;

    /// \brief The current file context.
    std::string m_current_file;
    KnightOptions m_current_options;
    clang::ASTContext* m_current_ast_ctx;
    std::unique_ptr< Globs > m_current_check_matcher;
    std::unique_ptr< Globs > m_current_analysis_matcher;
    std::string m_current_build_dir;

  public:
    KnightContext(std::unique_ptr< KnightOptionsProvider > opts_provider);
    ~KnightContext();

    /// \brief The reported diagnostic IDs and their corresponding checkers.
    std::unordered_map< unsigned, std::string > m_diag_id_to_checker_name;

    /// \brief set the external diagnostic engine.
    void set_diagnostic_engine(clang::DiagnosticsEngine* external_diag_engine);

    /// \brief get the current file.
    llvm::StringRef get_current_file() const { return m_current_file; }
    /// \brief set the current file when handle a new TU.
    void set_current_file(llvm::StringRef file);

    /// Gets the language options from the AST context.
    const clang::LangOptions& get_lang_options() const {
        return m_current_ast_ctx->getLangOpts();
    }

    clang::ASTContext& get_ast_context() const { return *m_current_ast_ctx; }

    clang::SourceManager& get_source_manager() const {
        return m_current_ast_ctx->getSourceManager();
    }

    /// \brief set the current clang AST context
    void set_current_ast_context(clang::ASTContext* ast_ctx);

    /// \brief set the current build directory.
    void set_current_build_dir(llvm::StringRef build_dir) {
        m_current_build_dir = build_dir;
    }

    /// \brief get the enabled status of checker.
    /// \returns \c true if the checker is enabled, \c false otherwise.
    bool is_check_enabled(llvm::StringRef checker) const;

    /// \brief get the directly enabled status of checker by the options.
    ///
    /// \returns \c true if the checker is enabled, \c false otherwise.
    bool is_analysis_directly_enabled(llvm::StringRef analysis) const;

    /// \brief Returns the corresponding checker name for the given diagnostic ID.
    /// \returns The checker name if found, or an empty optional if not found.
    std::optional< std::string > get_check_name(unsigned diag_id);

    /// \brief get the options for the given file.
    KnightOptions get_options_for(llvm::StringRef file) const;

    /// \brief diagnoser
    /// @{
    clang::DiagnosticBuilder diagnose(
        llvm::StringRef checker,
        clang::SourceLocation loc,
        llvm::StringRef info,
        clang::DiagnosticIDs::Level diag_level = clang::DiagnosticIDs::Warning);

    clang::DiagnosticBuilder diagnose(
        llvm::StringRef checker,
        llvm::StringRef info,
        clang::DiagnosticIDs::Level diag_level = clang::DiagnosticIDs::Warning);

    clang::DiagnosticBuilder diagnose(const clang::tooling::Diagnostic& d);
    /// @}

}; // class KnightContext

} // namespace knight