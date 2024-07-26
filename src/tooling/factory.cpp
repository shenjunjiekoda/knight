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

#include "tooling/factory.hpp"
#include "tooling/context.hpp"
#include "dfa/analysis_manager.hpp"
#include "dfa/checker_manager.hpp"

namespace knight {

KnightFactory::KnightFactory(dfa::AnalysisManager& analysis_mgr,
                             dfa::CheckerManager& checker_mgr)
    : m_analysis_mgr(analysis_mgr), m_checker_mgr(checker_mgr) {}

void KnightFactory::add_analysis_create_fn(dfa::AnalysisID id,
                                           llvm::StringRef name,
                                           AnalysisRegistryFn registry) {
    m_analysis_registry[{id, name}] = registry;
    registry()->add_dependencies(m_analysis_mgr);
}

void KnightFactory::add_checker_create_fn(dfa::AnalysisID id,
                                          llvm::StringRef name,
                                          CheckerRegistryFn registry) {
    m_checker_registry[{id, name}] = registry;
    registry()->add_dependencies(m_checker_mgr);
}

KnightFactory::AnalysisRefs KnightFactory::create_analyses(
    KnightContext* context) const {
    AnalysisRefs analyses;
    const auto& LO = context->get_lang_options();
    for (const auto& [analysis, registry] : m_analysis_registry) {
        if (m_analysis_mgr.is_analysis_required(analysis.first)) {
            AnalysisRef analysis = registry();
            analyses.push_back(std::move(analysis));
        }
    }
    return analyses;
}

KnightFactory::CheckerRefs KnightFactory::create_checkers(
    KnightContext* context) const {
    CheckerRefs checkers;
    const auto& LO = context->get_lang_options();
    for (const auto& [checker, registry] : m_checker_registry) {
        if (m_checker_mgr.is_checker_required(checker.first)) {
            CheckerRef checker = registry();
            checkers.push_back(std::move(checker));
        }
    }
    return checkers;
}

} // namespace knight