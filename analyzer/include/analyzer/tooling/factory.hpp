//===- factory.hpp ----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the knight factory which is used to create knight
//  analyses and checkers of modules.
//
//===------------------------------------------------------------------===//

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringRef.h>

#include "analyzer/core/analysis/analyses.hpp"
#include "analyzer/core/analysis/analysis_base.hpp"
#include "analyzer/core/analysis_manager.hpp"
#include "analyzer/core/checker/checker_base.hpp"
#include "analyzer/core/checker/checkers.hpp"
#include "analyzer/core/checker_manager.hpp"
#include "analyzer/support/analysis.hpp"
#include "analyzer/support/checker.hpp"

#include <unordered_map>

namespace knight {

class KnightContext;

namespace analyzer {

class AnalysisManager;
class CheckerManager;

} // namespace analyzer

/// A collection of \c KnightCheckFactory instances.
///
/// All knight modules register their check factories with an instance of
/// this object.
class KnightFactory {
  public:
    using Analysis = analyzer::AnalysisBase;
    using UniqueAnalysisRef = analyzer::UniqueAnalysisRef;
    using AnalysisRef = analyzer::AnalysisRef;
    using AnalysisRefs = analyzer::AnalysisRefs;

    using Checker = analyzer::CheckerBase;
    using UniqueCheckerRef = analyzer::UniqueCheckerRef;
    using CheckerRef = analyzer::CheckerRef;
    using CheckerRefs = analyzer::CheckerRefs;

    using AnalysisRegistryFn =
        std::function< UniqueAnalysisRef(analyzer::AnalysisManager&,
                                         KnightContext&) >;
    using CheckerRegistryFn =
        std::function< UniqueCheckerRef(analyzer::CheckerManager&,
                                        KnightContext&) >;

    using AnalysisRegistryFnMap =
        llvm::DenseMap< std::pair< analyzer::AnalysisID, llvm::StringRef >,
                        AnalysisRegistryFn >;
    using CheckerRegistryFnMap =
        llvm::DenseMap< std::pair< analyzer::CheckerID, llvm::StringRef >,
                        CheckerRegistryFn >;

  private:
    AnalysisRegistryFnMap m_analysis_registry;
    CheckerRegistryFnMap m_checker_registry;
    analyzer::AnalysisManager& m_analysis_mgr;
    analyzer::CheckerManager& m_checker_mgr;

  public:
    KnightFactory(analyzer::AnalysisManager& analysis_mgr,
                  analyzer::CheckerManager& checker_mgr);

  public:
    /// \brief Add analysis registry function with analysis id and name.
    void add_analysis_create_fn(analyzer::AnalysisID id,
                                llvm::StringRef name,
                                AnalysisRegistryFn registry);

    /// \brief Add checker registry function with checker id and name.
    void add_checker_create_fn(analyzer::CheckerID id,
                               llvm::StringRef name,
                               CheckerRegistryFn registry);

    /// \brief Registers the analysis with the name.
    /// TODO: add concept
    template < typename ANALYSIS >
    void register_analysis() {
        analyzer::AnalysisKind kind = ANALYSIS::get_kind();
        if constexpr (analyzer::is_dependent_analysis< ANALYSIS >::value) {
            ANALYSIS::add_dependencies(m_analysis_mgr);
        }
        add_analysis_create_fn(analyzer::get_analysis_id(kind),
                               analyzer::get_analysis_name(kind),
                               ANALYSIS::register_analysis);
    }

    /// \brief Registers the checker with the name.
    /// TODO: add concept
    template < typename CHECKER >
    void register_checker() {
        analyzer::CheckerKind kind = CHECKER::get_kind();
        if constexpr (analyzer::is_dependent_checker< CHECKER >::value) {
            CHECKER::add_dependencies(m_checker_mgr);
        }
        add_checker_create_fn(analyzer::get_checker_id(kind),
                              analyzer::get_checker_name(kind),
                              CHECKER::register_checker);
    }

    /// \brief Create instances of analyses that are required.
    AnalysisRefs create_analyses(analyzer::AnalysisManager& mgr,
                                 KnightContext* context) const;

    /// \brief Create instances of checkers that are required.
    CheckerRefs create_checkers(analyzer::CheckerManager& mgr,
                                KnightContext* context) const;

    [[nodiscard]] const AnalysisRegistryFnMap& analysis_registries() const {
        return m_analysis_registry;
    }

    [[nodiscard]] bool analysis_registry_empty() const {
        return m_analysis_registry.empty();
    }

    [[nodiscard]] const CheckerRegistryFnMap& checker_registries() const {
        return m_checker_registry;
    }

    [[nodiscard]] bool checker_registry_empty() const {
        return m_checker_registry.empty();
    }

}; // class KnightFactory

} // namespace knight