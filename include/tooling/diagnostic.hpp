//===- diagnostic.hpp -------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the diagnostic used by the knight tool.
//
//===------------------------------------------------------------------===//

#pragma once

#include <clang/Basic/Diagnostic.h>
#include <clang/Frontend/DiagnosticRenderer.h>
#include <clang/Tooling/Core/Diagnostic.h>

#include "util/assert.hpp"

namespace knight {

class KnightContext;

// NOLINTNEXTLINE(altera-struct-pack-align)
struct KnightDiagnostic : clang::tooling::Diagnostic {
    KnightDiagnostic(llvm::StringRef checker,
                     Level diag_level,
                     llvm::StringRef build_dir);
}; // struct KnightDiagnostic

// NOLINTNEXTLINE(altera-struct-pack-align)
struct KnightDiagnosticConsumer : public clang::DiagnosticConsumer {
    explicit KnightDiagnosticConsumer(KnightContext& context);

    void HandleDiagnostic(clang::DiagnosticsEngine::Level diag_level,
                          const clang::Diagnostic& diagnostic) override;

    // Retrieve the diagnostics that were captured.
    std::vector< KnightDiagnostic > take_diags();

  private:
    KnightContext& m_context;
    std::vector< KnightDiagnostic > m_diags;
}; // struct KnightDiagnosticConsumer

class KnightDiagnosticRenderer : public clang::DiagnosticRenderer {
  private:
    KnightDiagnostic& m_diag;

  public:
    KnightDiagnosticRenderer(const clang::LangOptions& lang_opts,
                             clang::DiagnosticOptions* diag_opts,
                             KnightDiagnostic& diag)
        : clang::DiagnosticRenderer(lang_opts, diag_opts), m_diag(diag) {}

  protected:
    void emitDiagnosticMessage(clang::FullSourceLoc loc,
                               clang::PresumedLoc ploc,
                               clang::DiagnosticsEngine::Level level,
                               llvm::StringRef msg,
                               llvm::ArrayRef< clang::CharSourceRange > ranges,
                               clang::DiagOrStoredDiag diag) override;
    void emitDiagnosticLoc(
        clang::FullSourceLoc loc,
        clang::PresumedLoc ploc,
        clang::DiagnosticsEngine::Level level,
        llvm::ArrayRef< clang::CharSourceRange > ranges) override {}

    void emitCodeContext(
        clang::FullSourceLoc loc,
        clang::DiagnosticsEngine::Level level,
        llvm::SmallVectorImpl< clang::CharSourceRange >& ranges,
        clang::ArrayRef< clang::FixItHint > hints) override;

    void emitIncludeLocation(clang::FullSourceLoc loc,
                             clang::PresumedLoc ploc) override {}

    void emitImportLocation(clang::FullSourceLoc loc,
                            clang::PresumedLoc ploc,
                            llvm::StringRef module_name) override {}

    void emitBuildingModuleLocation(clang::FullSourceLoc loc,
                                    clang::PresumedLoc ploc,
                                    llvm::StringRef module_name) override {}

    void endDiagnostic(clang::DiagOrStoredDiag diag,
                       clang::DiagnosticsEngine::Level level) override {
        knight_assert_msg(!m_diag.Message.Message.empty(),
                          "Message has not been set");
    }

}; // class KnightDiagnosticRenderer

} // namespace knight