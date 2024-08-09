//===- context.cpp ----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the knight context by the knight library and
//  command-line tools.
//
//===------------------------------------------------------------------===//

#include "tooling/context.hpp"
#include "util/assert.hpp"

#include <clang/AST/ASTDiagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

#include <memory>

namespace knight {

KnightContext::KnightContext(
    std::unique_ptr< KnightOptionsProvider > opts_provider)
    : m_opts_provider(std::move(opts_provider)) {
    set_current_file("");
}

KnightContext::~KnightContext() = default;

void KnightContext::set_diagnostic_engine(
    clang::DiagnosticsEngine* external_diag_engine) {
    this->m_diag_engine = external_diag_engine;
}

KnightOptions KnightContext::get_options_for(llvm::StringRef file) const {
    return m_opts_provider->get_options_for(file.str());
}

void KnightContext::set_current_file(llvm::StringRef file) {
    m_current_file = file.str();
    m_current_options = get_options_for(file);
    m_current_check_matcher =
        std::make_unique< Globs >(m_current_options.checkers);
    m_current_analysis_matcher =
        std::make_unique< Globs >(m_current_options.analyses);
}

clang::DiagnosticBuilder KnightContext::diagnose(
    llvm::StringRef checker,
    clang::SourceLocation loc,
    llvm::StringRef info,
    clang::DiagnosticIDs::Level diag_level) {
    knight_assert_msg(loc.isValid(), "Invalid location");

    auto fmt = (info + " [" + checker + "]").str();
    const unsigned custom_diag_id =
        m_diag_engine->getDiagnosticIDs()->getCustomDiagID(diag_level, fmt);

    m_diag_id_to_checker_name[custom_diag_id] = checker;
    return m_diag_engine->Report(loc, custom_diag_id);
}

void KnightContext::set_current_ast_context(clang::ASTContext* ast_ctx) {
    m_current_ast_ctx = ast_ctx;
    m_diag_engine->setSourceManager(&ast_ctx->getSourceManager());
    m_diag_engine->SetArgToStringFn(&clang::FormatASTNodeDiagnosticArgument,
                                    ast_ctx);
}

bool KnightContext::is_check_enabled(llvm::StringRef checker) const {
    knight_assert_msg(m_current_check_matcher != nullptr,
                      "checker matcher is null");
    return m_current_check_matcher->matches(checker);
}

bool KnightContext::is_analysis_directly_enabled(
    llvm::StringRef analysis) const {
    knight_assert_msg(m_current_analysis_matcher != nullptr,
                      "analysis matcher is null");
    return m_current_analysis_matcher->matches(analysis);
}

bool KnightContext::is_core_analysis_enabled(llvm::StringRef analysis) const {
    // TODO: shall we use a more restrictive way to check core analysis?
    return analysis.starts_with("core-");
}

std::optional< std::string > KnightContext::get_check_name(unsigned diag_id) {
    auto it = m_diag_id_to_checker_name.find(diag_id);
    if (it != m_diag_id_to_checker_name.end()) {
        return it->second;
    }

    const std::string clang_warning_option = std::string(
        m_diag_engine->getDiagnosticIDs()->getWarningOptionForDiag(diag_id));
    if (!clang_warning_option.empty()) {
        return "knight-diagnostic-" + clang_warning_option;
    }

    return std::nullopt;
}

clang::DiagnosticBuilder KnightContext::diagnose(
    llvm::StringRef checker,
    llvm::StringRef info,
    clang::DiagnosticIDs::Level diag_level) {
    auto fmt = (info + " [" + checker + "]").str();
    const unsigned custom_diag_id =
        m_diag_engine->getDiagnosticIDs()->getCustomDiagID(diag_level, fmt);

    m_diag_id_to_checker_name[custom_diag_id] = checker;
    return m_diag_engine->Report(custom_diag_id);
}

clang::DiagnosticBuilder KnightContext::diagnose(
    const clang::tooling::Diagnostic& d) {
    auto& src_mgr = m_diag_engine->getSourceManager();
    auto& file_mgr = src_mgr.getFileManager();

    auto file_entry = llvm::cantFail(file_mgr.getFileRef(d.Message.FilePath));
    auto fid = src_mgr.getOrCreateFileID(file_entry, clang::SrcMgr::C_User);

    auto file_start = src_mgr.getLocForStartOfFile(fid);
    auto loc_in_file = file_start.getLocWithOffset(
        static_cast< clang::SourceLocation::IntTy >(d.Message.FileOffset));

    return diagnose(d.DiagnosticName,
                    loc_in_file,
                    d.Message.Message,
                    static_cast< clang::DiagnosticIDs::Level >(d.DiagLevel));
}

} // namespace knight