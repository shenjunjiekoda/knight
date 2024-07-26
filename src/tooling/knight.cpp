//===- knight.cpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the core driver for the Knight tool.
//
//===------------------------------------------------------------------===//

#include "tooling/knight.hpp"
#include "dfa/analysis_manager.hpp"
#include "tooling/module.hpp"
#include "tooling/factory.hpp"

LLVM_INSTANTIATE_REGISTRY(knight::KnightModuleRegistry);

namespace knight {

KnightASTConsumerFactory::KnightASTConsumerFactory(
    KnightContext& ctx,
    std::unique_ptr< dfa::AnalysisManager > external_analysis_manager,
    std::unique_ptr< dfa::CheckerManager > external_checker_manager)
    : m_ctx(ctx) {

    if (external_analysis_manager != nullptr) {
        m_analysis_manager = std::move(external_analysis_manager);
    } else {
        m_analysis_manager = std::make_unique< dfa::AnalysisManager >(m_ctx);
    }
    if (external_checker_manager != nullptr) {
        m_checker_manager = std::move(external_checker_manager);
    } else {
        m_checker_manager =
            std::make_unique< dfa::CheckerManager >(m_ctx, *m_analysis_manager);
    }

    m_factory = std::make_unique< KnightFactory >(*m_analysis_manager,
                                                  *m_checker_manager);
    for (auto entry : KnightModuleRegistry::entries()) {
        entry.instantiate()->add_to_factory(*m_factory);
    }
}

std::unique_ptr< clang::ASTConsumer > KnightASTConsumerFactory::
    create_ast_consumer(clang::CompilerInstance& ci, llvm::StringRef file) {
    auto& source_mgr = ci.getSourceManager();
    auto& file_mgr = source_mgr.getFileManager();
    m_ctx.set_current_file(file);
    m_ctx.set_current_ast_context(&ci.getASTContext());

    auto working_dir =
        file_mgr.getVirtualFileSystem().getCurrentWorkingDirectory();
    if (working_dir) {
        m_ctx.set_current_build_dir(working_dir.get());
    }

    // TODO: add checkers and analyses register and enabled logic here.

    auto checkers = m_factory->create_checkers(&m_ctx);
    auto analyses = m_factory->create_analyses(&m_ctx);
    return std::make_unique< KnightASTConsumer >(m_ctx,
                                                 *m_analysis_manager,
                                                 *m_checker_manager,
                                                 std::move(checkers),
                                                 std::move(analyses));
}

std::vector< std::pair< dfa::CheckerID, llvm::StringRef > >
KnightASTConsumerFactory::get_enabled_checks() const {
    std::vector< std::pair< dfa::CheckerID, llvm::StringRef > > enabled_checks;
    for (const auto& [checker, registry] : m_factory->checker_registries()) {
        if (m_ctx.is_check_enabled(checker.second)) {
            enabled_checks.emplace_back(checker);
        }
    }
    return enabled_checks;
}

std::vector< std::pair< dfa::AnalysisID, llvm::StringRef > >
KnightASTConsumerFactory::get_directly_enabled_analyses() const {
    std::vector< std::pair< dfa::AnalysisID, llvm::StringRef > >
        enabled_analyses;
    for (const auto& [analysis, registry] : m_factory->analysis_registries()) {
        if (m_ctx.is_analysis_directly_enabled(analysis.second)) {
            enabled_analyses.emplace_back(analysis);
        }
    }
    return enabled_analyses;
}

} // namespace knight