//===- reporter.cpp ---------------------------------------------------===//
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

#include "tooling/reporter.hpp"
#include "tooling/diagnostic.hpp"
#include "clang/Basic/LangOptions.h"
#include "clang/Tooling/Core/Replacement.h"
#include "llvm/Support/raw_ostream.h"

#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/DiagnosticFrontend.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Format/Format.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/Support/Process.h>

namespace knight {

DiagnosticReporter::DiagnosticReporter(FixKind kind,
                                       KnightContext& ctx,
                                       fs::FileSystemRef base_vfs)
    : m_ctx(ctx), m_fix_kind(kind), m_total_fixes(0U), m_applied_fixes(0U),
      m_file_manager({}, std::move(base_vfs)),
      m_diag_opts(new clang::DiagnosticOptions()),
      m_consumer(new clang::TextDiagnosticPrinter(llvm::outs(), &*m_diag_opts)),
      m_diag_engine(llvm::IntrusiveRefCntPtr< clang::DiagnosticIDs >(
                        new clang::DiagnosticIDs),
                    m_diag_opts,
                    m_consumer),
      m_source_manager(m_diag_engine, m_file_manager) {
    m_diag_opts->ShowColors = m_ctx.get_current_options().use_color;
    m_consumer->BeginSourceFile(m_lang_opts);
    if (m_diag_opts->ShowColors &&
        !llvm::sys::Process::StandardOutIsDisplayed()) {
        llvm::sys::Process::UseANSIEscapeCodes(true);
    }
}

clang::SourceManager& DiagnosticReporter::get_source_manager() {
    return m_source_manager;
}

const llvm::StringMap< clang::tooling::Replacements >* DiagnosticReporter::
    get_replacements(const clang::tooling::Diagnostic& diagnostic,
                     bool fix_note) {
    const llvm::StringMap< clang::tooling::Replacements >* res = nullptr;

    if (!diagnostic.Message.Fix.empty()) {
        res = &diagnostic.Message.Fix;
    } else if (fix_note) {
        for (const auto& note : diagnostic.Notes) {
            if (!note.Fix.empty()) {
                if (res != nullptr) {
                    return nullptr;
                }
                res = &note.Fix;
            }
        }
    }
    return res;
}

clang::SourceLocation DiagnosticReporter::get_composed_loc(llvm::StringRef file,
                                                           unsigned offset) {
    if (file.empty()) {
        return {};
    }

    auto File = m_source_manager.getFileManager().getOptionalFileRef(file);
    if (!File) {
        return {};
    }

    auto file_id =
        m_source_manager.getOrCreateFileID(*File, clang::SrcMgr::C_User);
    return m_source_manager.getLocForStartOfFile(file_id).getLocWithOffset(
        offset);
}

clang::CharSourceRange DiagnosticReporter::get_char_range(
    const clang::tooling::FileByteRange& byte_range) {
    llvm::SmallString< 256 > abs_path{byte_range.FilePath};
    m_file_manager.makeAbsolutePath(abs_path);

    auto begin_loc = get_composed_loc(abs_path, byte_range.FileOffset);
    return clang::CharSourceRange::getCharRange(begin_loc,
                                                begin_loc.getLocWithOffset(
                                                    byte_range.Length));
}

void DiagnosticReporter::report_fix(
    const clang::DiagnosticBuilder& diag_builder,
    const llvm::StringMap< clang::tooling::Replacements >& file_replacements) {
    using namespace clang;
    for (const auto& [file, replacements] : file_replacements) {
        for (const auto& Repl : replacements) {
            if (!Repl.isApplicable()) {
                continue;
            }
            tooling::FileByteRange byte_range;
            byte_range.FilePath = Repl.getFilePath().str();
            byte_range.FileOffset = Repl.getOffset();
            byte_range.Length = Repl.getLength();

            diag_builder
                << FixItHint::CreateReplacement(get_char_range(byte_range),
                                                Repl.getReplacementText());
        }
    }
}

void DiagnosticReporter::report_note(
    const clang::tooling::DiagnosticMessage& diag_msg) {

    auto note_loc = get_composed_loc(diag_msg.FilePath, diag_msg.FileOffset);

    auto builder =
        m_diag_engine
            .Report(note_loc,
                    m_diag_engine
                        .getCustomDiagID(clang::DiagnosticsEngine::Note, "%0"))
        << diag_msg.Message;

    for (const auto& byte_range : diag_msg.Ranges) {
        builder << get_char_range(byte_range);
    }

    report_fix(builder, diag_msg.Fix);
}

void DiagnosticReporter::report(const KnightDiagnostic& diagnostic) {
    using namespace clang;
    const auto& msg = diagnostic.Message;
    auto loc = get_composed_loc(msg.FilePath, msg.FileOffset);

    // Contains a pair for each attempted fix: location and whether the fix was
    // applied successfully.
    llvm::SmallVector< std::pair< clang::SourceLocation, bool >, 4 > fix_locs;
    {
        auto diag_level = static_cast< clang::DiagnosticsEngine::Level >(
            diagnostic.DiagLevel);
        std::string diag_name = diagnostic.DiagnosticName;

        auto diag_builder =
            m_diag_engine.Report(loc,
                                 m_diag_engine.getCustomDiagID(diag_level,
                                                               "%0 [%1]"))
            << msg.Message << diag_name;

        for (const auto& range : diagnostic.Message.Ranges) {
            diag_builder << get_char_range(range);
        }

        const llvm::StringMap< clang::tooling::Replacements >* chosen_fix =
            nullptr;
        if (m_fix_kind != FixKind::None) {
            if ((chosen_fix = get_replacements(diagnostic, true))) {

                for (const auto& [file, replacements] : *chosen_fix) {
                    for (const auto& replacement : replacements) {
                        ++m_total_fixes;
                        bool CanBeApplied = false;
                        if (!replacement.isApplicable()) {
                            continue;
                        }

                        SourceLocation fix_loc;

                        llvm::SmallString< 256 > abs_path_fix =
                            replacement.getFilePath();
                        m_file_manager.makeAbsolutePath(abs_path_fix);
                        tooling::Replacement replace(abs_path_fix,
                                                     replacement.getOffset(),
                                                     replacement.getLength(),
                                                     replacement
                                                         .getReplacementText());

                        auto& [build_dir, replaces] =
                            m_file_to_build_dir_and_replaces
                                [replacement.getFilePath()];

                        llvm::Error err = replaces.add(replace);
                        if (err) {
                            llvm::errs()
                                << "Trying to resolve conflict: "
                                << llvm::toString(std::move(err)) << "\n";

                            unsigned new_offset =
                                replaces.getShiftedCodePosition(
                                    replace.getOffset());
                            unsigned new_len =
                                replaces.getShiftedCodePosition(
                                    replace.getOffset() + replace.getLength()) -
                                new_offset;
                            if (new_len == replace.getLength()) {
                                replace = tooling::
                                    Replacement(replace.getFilePath(),
                                                new_offset,
                                                new_len,
                                                replace.getReplacementText());
                                replaces = replaces.merge(
                                    clang::tooling::Replacements(replace));
                                CanBeApplied = true;
                                ++m_applied_fixes;
                            } else {
                                llvm::errs()
                                    << "Can't resolve conflict, skipping "
                                       "the replacement.\n";
                            }
                        } else {
                            CanBeApplied = true;
                            ++m_applied_fixes;
                        }
                        fix_loc = get_composed_loc(abs_path_fix,
                                                   replacement.getOffset());
                        fix_locs.push_back(
                            std::make_pair(fix_loc, CanBeApplied));
                        build_dir = diagnostic.BuildDirectory;
                    }
                }
            }
        }
        report_fix(diag_builder, diagnostic.Message.Fix);
    }
    for (auto& [fix_loc, can_apply] : fix_locs) {
        m_diag_engine.Report(fix_loc,
                             can_apply ? diag::note_fixit_applied
                                       : diag::note_fixit_failed);
    }
    for (const auto& note : diagnostic.Notes) {
        report_note(note);
    }
}

void DiagnosticReporter::apply_fixes() {
    using namespace clang;

    if (m_total_fixes == 0U) {
        return;
    }

    auto& vfs = m_file_manager.getVirtualFileSystem();
    auto origin_cwd = vfs.getCurrentWorkingDirectory();

    bool any_not_written = false;
    for (const auto& [file, pair] : m_file_to_build_dir_and_replaces) {
        const auto& [build_dir, replaces] = pair;

        Rewriter rewriter(m_source_manager, m_lang_opts);

        (void)vfs.setCurrentWorkingDirectory(build_dir);

        auto buffer = m_source_manager.getFileManager().getBufferForFile(file);
        if (!buffer) {
            llvm::errs() << "error when accessing file: " << file << ": "
                         << buffer.getError().message() << "\n";
            continue;
        }
        auto code = buffer.get()->getBuffer();

        auto fmt = format::getStyle("none", file, "none");
        if (!fmt) {
            llvm::errs() << llvm::toString(fmt.takeError()) << "\n";
            continue;
        }

        auto replacements_expected =
            format::cleanupAroundReplacements(code, replaces, *fmt);
        if (!replacements_expected) {
            llvm::errs() << "error when applying replacements: "
                         << llvm::toString(replacements_expected.takeError())
                         << "\n";
            continue;
        }

        if (auto replacements_formatted =
                format::formatReplacements(code, replaces, *fmt)) {
            replacements_expected = std::move(replacements_formatted);
            knight_assert_msg(replacements_expected,
                              "format replacements failed");
        } else {
            llvm::errs() << "error when formatting replacements: "
                         << llvm::toString(replacements_formatted.takeError())
                         << "\n";
            continue;
        }

        if (!clang::tooling::applyAllReplacements(replacements_expected.get(),
                                                  rewriter)) {
            llvm::errs() << "error when applying replacements\n";
        }
        any_not_written |= rewriter.overwriteChangedFiles();
    }

    if (any_not_written) {
        llvm::errs() << "failed to apply some suggested fixes.\n";
    } else {
        llvm::outs() << "applied " << m_applied_fixes << " out of "
                     << m_total_fixes << " suggested fixes.\n";
    }

    if (origin_cwd) {
        (void)vfs.setCurrentWorkingDirectory(*origin_cwd);
    }
}

} // namespace knight