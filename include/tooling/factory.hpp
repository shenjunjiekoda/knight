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

#include "dfa/analysis/analysis_base.hpp"
#include "dfa/analysis_manager.hpp"
#include "dfa/checker/checker_base.hpp"
#include "dfa/checker_manager.hpp"

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
    using UniqueAnalysisRef = std::unique_ptr< Analysis >;
    using AnalysisRef = std::unique_ptr< Analysis >;
    using AnalysisRefs = std::vector< AnalysisRef >;

    using Checker = dfa::CheckerBase;
    using UniqueCheckerRef = std::unique_ptr< Checker >;
    using CheckerRef = Checker*;
    using CheckerRefs = std::vector< CheckerRef >;

    using AnalysisRegistryFn = std::function< UniqueAnalysisRef() >;
    using CheckerRegistryFn = std::function< UniqueCheckerRef() >;

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
    /// \brief Add analysis registry function with analysis id.
    void add_analysis_create_fn(dfa::AnalysisID id,
                                llvm::StringRef name,
                                AnalysisRegistryFn registry);

    /// \brief Add checker registry function with checker id.
    void add_checker_create_fn(dfa::CheckerID id,
                               llvm::StringRef name,
                               CheckerRegistryFn registry);

    /// \brief Registers the analysis with the name.
    template < typename ANALYSIS > void register_analysis(dfa::AnalysisID id) {
        add_analysis_create_fn(id,
                               []() { return std::make_unique< ANALYSIS >(); });
    }

    /// \brief Registers the checker with the name.
    template < typename CHECKER > void register_checker(dfa::CheckerID id) {
        add_checker_create_fn(id,
                              []() { return std::make_unique< CHECKER >(); });
    }

    /// \brief Create instances of analyses that are required.
    AnalysisRefs create_analyses(KnightContext* context) const;

    /// \brief Create instances of checkers that are required.
    CheckerRefs create_checkers(KnightContext* context) const;

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