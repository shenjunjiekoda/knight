//===- factory.cpp ----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the knight factory which is used to create knight
//  analyses and checkers of modules.
//
//===------------------------------------------------------------------===//

#include "analyzer/tooling/factory.hpp"
#include "analyzer/core/analysis_manager.hpp"
#include "analyzer/core/checker_manager.hpp"
#include "analyzer/tooling/context.hpp"

namespace knight {

KnightFactory::KnightFactory(analyzer::AnalysisManager& analysis_mgr,
                             analyzer::CheckerManager& checker_mgr)
    : m_analysis_mgr(analysis_mgr), m_checker_mgr(checker_mgr) {}

void KnightFactory::add_analysis_create_fn(analyzer::AnalysisID id,
                                           llvm::StringRef name,
                                           AnalysisRegistryFn registry) {
    m_analysis_registry[{id, name}] = std::move(registry);
}

void KnightFactory::add_checker_create_fn(analyzer::AnalysisID id,
                                          llvm::StringRef name,
                                          CheckerRegistryFn registry) {
    m_checker_registry[{id, name}] = std::move(registry);
}

KnightFactory::AnalysisRefs KnightFactory::create_analyses(
    analyzer::AnalysisManager& mgr, KnightContext* context) const {
    AnalysisRefs analyses;
    const auto& lo = context->get_lang_options();
    for (const auto& [analysis, registry] : m_analysis_registry) {
        if (m_analysis_mgr.is_analysis_required(analysis.first)) {
            auto analysis = registry(mgr, *context);
            analyses.push_back(analysis.get());
            m_analysis_mgr.enable_analysis(std::move(analysis));
        }
    }
    return analyses;
}

KnightFactory::CheckerRefs KnightFactory::create_checkers(
    analyzer::CheckerManager& mgr, KnightContext* context) const {
    CheckerRefs checkers;
    const auto& lo = context->get_lang_options();
    for (const auto& [checker, registry] : m_checker_registry) {
        if (m_checker_mgr.is_checker_required(checker.first)) {
            auto checker = registry(mgr, *context);
            checkers.push_back(checker.get());
            m_checker_mgr.enable_checker(std::move(checker));
        }
    }
    return checkers;
}

} // namespace knight