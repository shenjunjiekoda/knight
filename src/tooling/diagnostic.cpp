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
#include "tooling/context.hpp"
#include "tooling/knight.hpp"
#include "util/assert.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Lex/Lexer.h>
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

KnightDiagnosticConsumer::KnightDiagnosticConsumer(KnightContext& context)
    : m_context(context) {}

void KnightDiagnosticConsumer::HandleDiagnostic(
    clang::DiagnosticsEngine::Level diag_level,
    const clang::Diagnostic& diagnostic) {
    using namespace clang;
    DiagnosticConsumer::HandleDiagnostic(diag_level, diagnostic);
    if (diag_level == DiagnosticsEngine::Note) {
        knight_assert_msg(!m_diags.empty(),
                          "A diagnostic note can only be appended to a "
                          "message.");
    } else {
        auto check = m_context.get_check_name(diagnostic.getID());

        if (!check) {
            // This is a compiler diagnostic without a warning option. Assign
            // check name based on its level.
            switch (diag_level) {
            case DiagnosticsEngine::Error:
            case DiagnosticsEngine::Fatal:
                check = "clang-error";
                break;
            case DiagnosticsEngine::Warning:
                check = "clang-warning";
                break;
            case DiagnosticsEngine::Remark:
                check = "clang-remark";
                break;
            default:
                check = "clang-unknown";
                break;
            }
        }

        KnightDiagnostic::Level level = KnightDiagnostic::Warning;
        switch (diag_level) {
        default:
            level = KnightDiagnostic::Warning;
            break;
        case DiagnosticsEngine::Error:
            [[fallthrough]];
        case DiagnosticsEngine::Fatal:
            level = KnightDiagnostic::Error;
            break;
        case DiagnosticsEngine::Remark:
            level = KnightDiagnostic::Remark;
            break;
        }

        m_diags.emplace_back(*check, level, m_context.get_cuurent_build_dir());
    }
    KnightDiagnosticRenderer renderer(m_context.get_lang_options(),
                                      &m_context.get_diagnostic_engine()
                                           ->getDiagnosticOptions(),
                                      m_diags.back());
    SmallString< 100 > msg;
    diagnostic.FormatDiagnostic(msg);
    FullSourceLoc loc;
    if (diagnostic.hasSourceManager()) {
        loc = FullSourceLoc(diagnostic.getLocation(),
                            diagnostic.getSourceManager());
    } else if (m_context.get_diagnostic_engine()->hasSourceManager()) {
        loc = FullSourceLoc(diagnostic.getLocation(),
                            m_context.get_diagnostic_engine()
                                ->getSourceManager());
    }
    renderer.emitDiagnostic(loc,
                            diag_level,
                            msg,
                            diagnostic.getRanges(),
                            diagnostic.getFixItHints());

    m_context.get_diagnostic_engine()->Clear();
}

std::vector< KnightDiagnostic > KnightDiagnosticConsumer::take_diags() {
    std::stable_sort(m_diags.begin(), m_diags.end(), Less());
    auto last = std::unique(m_diags.begin(), m_diags.end(), Equal());
    m_diags.erase(last, m_diags.end());
    return std::move(m_diags);
}

void KnightDiagnosticRenderer::emitDiagnosticMessage(
    clang::FullSourceLoc loc,
    clang::PresumedLoc ploc,
    clang::DiagnosticsEngine::Level level,
    llvm::StringRef msg,
    llvm::ArrayRef< clang::CharSourceRange > ranges,
    clang::DiagOrStoredDiag diag) {
    using namespace clang;
    // Remove check name from the message.
    std::string check_name_in_msg = " [" + m_diag.DiagnosticName + "]";
    msg.consume_back(check_name_in_msg);

    auto knight_msg =
        loc.isValid() ? tooling::DiagnosticMessage(msg, loc.getManager(), loc)
                      : tooling::DiagnosticMessage(msg);

    // Make sure that if a TokenRange is received from the check it is unfurled
    // into a real CharRange for the diagnostic printer later.
    // Whatever we store here gets decoupled from the current SourceManager, so
    // we **have to** know the exact position and length of the highlight.
    auto to_char_range = [this, &loc](const CharSourceRange& range) {
        if (range.isCharRange()) {
            return range;
        }
        knight_assert(range.isTokenRange());
        SourceLocation token_end = Lexer::getLocForEndOfToken(range.getEnd(),
                                                              0,
                                                              loc.getManager(),
                                                              LangOpts);
        return CharSourceRange::getCharRange(range.getBegin(), token_end);
    };

    auto valid_ranges =
        llvm::make_filter_range(ranges, [](const CharSourceRange& range) {
            return range.getAsRange().isValid();
        });

    if (level == DiagnosticsEngine::Note) {
        m_diag.Notes.push_back(knight_msg);
        for (const auto& range : valid_ranges) {
            m_diag.Notes.back().Ranges.emplace_back(loc.getManager(),
                                                    to_char_range(range));
        }
        return;
    }

    knight_assert_msg(m_diag.Message.Message.empty(),
                      "Overwriting a diagnostic message");
    m_diag.Message = knight_msg;

    for (const auto& SourceRange : valid_ranges) {
        m_diag.Message.Ranges.emplace_back(loc.getManager(),
                                           to_char_range(SourceRange));
    }
}

void KnightDiagnosticRenderer::emitCodeContext(
    clang::FullSourceLoc loc,
    clang::DiagnosticsEngine::Level level,
    llvm::SmallVectorImpl< clang::CharSourceRange >& ranges,
    clang::ArrayRef< clang::FixItHint > hints) {
    using namespace clang;
    knight_assert(loc.isValid());
    tooling::DiagnosticMessage* diag_msg = level == DiagnosticsEngine::Note
                                               ? &m_diag.Notes.back()
                                               : &m_diag.Message;

    for (const auto& FixIt : hints) {
        CharSourceRange Range = FixIt.RemoveRange;
        knight_assert_msg(Range.getBegin().isValid() &&
                              Range.getEnd().isValid(),
                          "Invalid range in the fix-it hint.");
        knight_assert_msg(Range.getBegin().isFileID() &&
                              Range.getEnd().isFileID(),
                          "Only file locations supported in fix-it hints.");

        tooling::Replacement replacement(loc.getManager(),
                                         Range,
                                         FixIt.CodeToInsert);
        auto Err = diag_msg->Fix[replacement.getFilePath()].add(replacement);
        if (Err) {
            llvm::errs() << "Fix conflicts with existing fix! "
                         << llvm::toString(std::move(Err)) << "\n";
            knight_assert(false && "Fix conflicts with existing fix!");
        }
    }
}

} // namespace knight