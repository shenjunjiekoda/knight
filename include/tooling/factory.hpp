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

#include <vector>
#include <functional>
#include <memory>

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/DenseMap.h>

#include "dfa/analysis/analyses.hpp"
#include "dfa/analysis/analysis_base.hpp"
#include "dfa/analysis_manager.hpp"
#include "dfa/checker/checker_base.hpp"
#include "dfa/checker/checkers.hpp"
#include "dfa/checker_manager.hpp"
#include "support/checker.hpp"
#include "support/analysis.hpp"

#include <unordered_map>

namespace knight {

class KnightContext;

namespace dfa {

class AnalysisManager;
class CheckerManager;

} // namespace dfa

/// A collection of \c KnightCheckFactory instances.
///
/// All knight modules register their check factories with an instance of
/// this object.
class KnightFactory {
  public:
    using Analysis = dfa::AnalysisBase;
    using UniqueAnalysisRef = dfa::UniqueAnalysisRef;
    using AnalysisRef = dfa::AnalysisRef;
    using AnalysisRefs = dfa::AnalysisRefs;

    using Checker = dfa::CheckerBase;
    using UniqueCheckerRef = dfa::UniqueCheckerRef;
    using CheckerRef = dfa::CheckerRef;
    using CheckerRefs = dfa::CheckerRefs;

    using AnalysisRegistryFn =
        std::function< UniqueAnalysisRef(dfa::AnalysisManager&,
                                         KnightContext&) >;
    using CheckerRegistryFn =
        std::function< UniqueCheckerRef(dfa::CheckerManager&, KnightContext&) >;

    using AnalysisRegistryFnMap =
        llvm::DenseMap< std::pair< dfa::AnalysisID, llvm::StringRef >,
                        AnalysisRegistryFn >;
    using CheckerRegistryFnMap =
        llvm::DenseMap< std::pair< dfa::CheckerID, llvm::StringRef >,
                        CheckerRegistryFn >;

  private:
    AnalysisRegistryFnMap m_analysis_registry;
    CheckerRegistryFnMap m_checker_registry;
    dfa::AnalysisManager& m_analysis_mgr;
    dfa::CheckerManager& m_checker_mgr;

  public:
    KnightFactory(dfa::AnalysisManager& analysis_mgr,
                  dfa::CheckerManager& checker_mgr);

  public:
    /// \brief Add analysis registry function with analysis id and name.
    void add_analysis_create_fn(dfa::AnalysisID id,
                                llvm::StringRef name,
                                AnalysisRegistryFn registry);

    /// \brief Add checker registry function with checker id and name.
    void add_checker_create_fn(dfa::CheckerID id,
                               llvm::StringRef name,
                               CheckerRegistryFn registry);

    /// \brief Registers the analysis with the name.
    /// TODO: add concept
    template < typename ANALYSIS > void register_analysis() {
        dfa::AnalysisKind kind = ANALYSIS::get_kind();
        if constexpr (dfa::is_dependent_analysis< ANALYSIS >::value) {
            ANALYSIS::add_dependencies(m_analysis_mgr);
        }
        add_analysis_create_fn(dfa::get_analysis_id(kind),
                               dfa::get_analysis_name(kind),
                               ANALYSIS::register_analysis);
    }

    /// \brief Registers the checker with the name.
    /// TODO: add concept
    template < typename CHECKER > void register_checker() {
        dfa::CheckerKind kind = CHECKER::get_kind();
        if constexpr (dfa::is_dependent_checker< CHECKER >::value) {
            CHECKER::add_dependencies(m_checker_mgr);
        }
        add_checker_create_fn(dfa::get_checker_id(kind),
                              dfa::get_checker_name(kind),
                              CHECKER::register_checker);
    }

    /// \brief Create instances of analyses that are required.
    AnalysisRefs create_analyses(dfa::AnalysisManager& mgr,
                                 KnightContext* context) const;

    /// \brief Create instances of checkers that are required.
    CheckerRefs create_checkers(dfa::CheckerManager& mgr,
                                KnightContext* context) const;

    const AnalysisRegistryFnMap& analysis_registries() const {
        return m_analysis_registry;
    }

    bool analysis_registry_empty() const { return m_analysis_registry.empty(); }

    const CheckerRegistryFnMap& checker_registries() const {
        return m_checker_registry;
    }

    bool checker_registry_empty() const { return m_checker_registry.empty(); }

}; // class KnightFactory

} // namespace knight