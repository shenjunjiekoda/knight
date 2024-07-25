//===- diagnostic.cpp -------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the diagnostic used by the knight tool.
//
//===------------------------------------------------------------------===//

#include "tooling/diagnostic.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Tooling/Core/Diagnostic.h>

#include <llvm/ADT/STLExtras.h>

#include <algorithm>

namespace knight {

namespace {

struct Less {
    bool operator()(const KnightDiagnostic& lhs,
                    const KnightDiagnostic& rhs) const {
        const auto& l = lhs.Message;
        const auto& r = rhs.Message;

        return std::tie(l.FilePath,
                        l.FileOffset,
                        lhs.DiagnosticName,
                        l.Message) < std::tie(r.FilePath,
                                              r.FileOffset,
                                              rhs.DiagnosticName,
                                              r.Message);
    }
};
struct Equal {
    bool operator()(const KnightDiagnostic& lhs,
                    const KnightDiagnostic& rhs) const {
        Less less;
        return !less(lhs, rhs) && !less(rhs, lhs);
    }
};

} // end anonymous namespace

KnightDiagnostic::KnightDiagnostic(llvm::StringRef checker,
                                   Level diag_level,
                                   llvm::StringRef build_dir)
    : clang::tooling::Diagnostic(checker, diag_level, build_dir) {}

void KnightDiagnosticConsumer::HandleDiagnostic(
    clang::DiagnosticsEngine::Level diag_level, const clang::Diagnostic& info) {
    clang::DiagnosticConsumer::HandleDiagnostic(diag_level, info);
}

std::vector< KnightDiagnostic > KnightDiagnosticConsumer::take_diags() {
    std::stable_sort(m_diags.begin(), m_diags.end(), Less());
    auto last = std::unique(m_diags.begin(), m_diags.end(), Equal());
    m_diags.erase(last, m_diags.end());
    return std::move(m_diags);
}

} // namespace knight