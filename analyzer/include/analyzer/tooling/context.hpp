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

#include "analyzer/tooling/options.hpp"
#include "common/util/globs.hpp"

#include <llvm/ADT/StringRef.h>

#include <clang/AST/ASTContext.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Tooling/Core/Diagnostic.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace knight {

class KnightContext {
  private:
    /// \brief The diagnostic engine used to diagnose errors.
    clang::DiagnosticsEngine* m_diag_engine{};

    std::unique_ptr< KnightOptionsProvider > m_opts_provider;

    /// \brief The current file context.
    std::string m_current_file;
    KnightOptions m_current_options;
    clang::ASTContext* m_current_ast_ctx{};
    std::unique_ptr< Globs > m_current_check_matcher;
    std::unique_ptr< Globs > m_current_analysis_matcher;
    std::string m_current_build_dir;

    llvm::BumpPtrAllocator m_alloc;

  public:
    /// \brief The reported diagnostic IDs and their corresponding checkers.
    std::unordered_map< unsigned, std::string > m_diag_id_to_checker_name;

  public:
    explicit KnightContext(
        std::unique_ptr< KnightOptionsProvider > opts_provider);
    ~KnightContext();

    /// \brief Get the diagnostic engine.
    [[nodiscard]] clang::DiagnosticsEngine* get_diagnostic_engine() const {
        return m_diag_engine;
    }

    /// \brief Get the external diagnostic engine.
    void set_diagnostic_engine(clang::DiagnosticsEngine* external_diag_engine);

    /// \brief Get the allocator.
    llvm::BumpPtrAllocator& get_allocator() { return m_alloc; }

    /// \brief Set the current file.
    [[nodiscard]] llvm::StringRef get_current_file() const {
        return m_current_file;
    }
    /// \brief Set the current file when handle a new TU.
    void set_current_file(llvm::StringRef file);

    /// \brief Get the language options from the AST context.
    [[nodiscard]] const clang::LangOptions& get_lang_options() const {
        return m_current_ast_ctx->getLangOpts();
    }

    /// \brief Get the current ast context.
    [[nodiscard]] clang::ASTContext* get_ast_context() const {
        return m_current_ast_ctx;
    }

    /// \brief Get the current source manager.
    [[nodiscard]] clang::SourceManager& get_source_manager() const {
        return m_current_ast_ctx->getSourceManager();
    }

    /// \brief Get the current build directory.
    [[nodiscard]] const std::string& get_cuurent_build_dir() const {
        return m_current_build_dir;
    }

    /// \breif Get the current options
    [[nodiscard]] const KnightOptions& get_current_options() const {
        return m_current_options;
    }

    /// \brief Set the current clang AST context
    void set_current_ast_context(clang::ASTContext* ast_ctx);

    /// \brief Set the current build directory.
    void set_current_build_dir(llvm::StringRef build_dir) {
        m_current_build_dir = build_dir;
    }

    /// \brief Get the enabled status of checker.
    /// \returns \c true if the checker is enabled, \c false otherwise.
    [[nodiscard]] bool is_check_enabled(llvm::StringRef checker) const;

    /// \brief Get the directly enabled status of analysis by the options.
    ///
    /// \returns \c true if the analysis is enabled, \c false otherwise.
    [[nodiscard]] bool is_analysis_directly_enabled(
        llvm::StringRef analysis) const;

    /// \breif Get the enabled status of core analyses
    ///
    /// \returns \c true if the analysis is enabled, \c false otherwise.
    [[nodiscard]] bool is_core_analysis_enabled(llvm::StringRef analysis) const;

    /// \brief Returns the corresponding checker name for the given diagnostic
    /// ID. \returns The checker name if found, or an empty optional if not
    /// found.
    [[nodiscard]] std::optional< std::string > get_check_name(unsigned diag_id);

    /// \brief Get the options for the given file.
    [[nodiscard]] KnightOptions get_options_for(llvm::StringRef file) const;

    /// \brief Diagnosing methods.
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