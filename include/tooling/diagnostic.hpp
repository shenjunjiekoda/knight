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

#include "clang/Basic/Diagnostic.h"
#include <clang/Tooling/Core/Diagnostic.h>

namespace knight {

class KnightContext;

struct KnightDiagnostic : clang::tooling::Diagnostic {
    KnightDiagnostic(llvm::StringRef checker,
                     Level diag_level,
                     llvm::StringRef build_dir);
}; // struct KnightDiagnostic

struct KnightDiagnosticConsumer : public clang::DiagnosticConsumer {
    KnightDiagnosticConsumer(KnightContext& context,
                             clang::DiagnosticsEngine* diag_engine);

    void HandleDiagnostic(clang::DiagnosticsEngine::Level diag_level,
                          const clang::Diagnostic& info) override;

    // Retrieve the diagnostics that were captured.
    std::vector< KnightDiagnostic > take_diags();

  private:
    KnightContext& m_context;
    clang::DiagnosticsEngine* diag_engine;
    std::vector< KnightDiagnostic > m_diags;
}; // struct KnightDiagnosticConsumer

} // namespace knight