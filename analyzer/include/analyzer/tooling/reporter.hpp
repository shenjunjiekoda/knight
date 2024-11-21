//===- reporter.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the reporter for knight diagnostics.
//
//===------------------------------------------------------------------===//

#pragma once

#include "analyzer/tooling/context.hpp"
#include "analyzer/tooling/diagnostic.hpp"
#include "common/util/vfs.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceManager.h>

namespace knight {

enum class FixKind {
    None,
    FixIt,
}; // enum FixKind

class DiagnosticReporter {
  private:
    KnightContext& m_ctx;

    clang::FileManager m_file_manager;

    llvm::IntrusiveRefCntPtr< clang::DiagnosticOptions > m_diag_opts;
    clang::DiagnosticConsumer* m_consumer;
    clang::DiagnosticsEngine m_diag_engine;
    clang::SourceManager m_source_manager;

    FixKind m_fix_kind;
    llvm::StringMap<
        std::pair< llvm::StringRef, clang::tooling::Replacements > >
        m_file_to_build_dir_and_replaces;

    clang::LangOptions m_lang_opts;

    unsigned m_total_fixes;
    unsigned m_applied_fixes;

  public:
    DiagnosticReporter(FixKind kind,
                       KnightContext& ctx,
                       fs::FileSystemRef base_vfs);

  public:
    clang::SourceManager& get_source_manager();

    void report(const KnightDiagnostic& diagnostic);

    void apply_fixes();

  private:
    static const llvm::StringMap< clang::tooling::Replacements >*
    get_replacements(const clang::tooling::Diagnostic& diagnostic,
                     bool fix_note);

    clang::SourceLocation get_composed_loc(llvm::StringRef file,
                                           unsigned offset);

    clang::CharSourceRange get_char_range(
        const clang::tooling::FileByteRange& byte_range);

    void report_fix(const clang::DiagnosticBuilder& diag_builder,
                    const llvm::StringMap< clang::tooling::Replacements >&
                        file_replacements);

    void report_note(const clang::tooling::DiagnosticMessage& diag_msg);
}; // class DiagnosticReporter

} // namespace knight
