//===- core_module.cpp ------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements core module.
//
//===------------------------------------------------------------------===//

#include "analyzer/core/analysis/analyses.hpp"
#include "analyzer/core/analysis/core/numerical_analysis.hpp"
#include "analyzer/core/analysis/core/symbol_resolver.hpp"
#include "analyzer/tooling/factory.hpp"
#include "analyzer/tooling/module.hpp"

namespace knight {

constexpr const char* ModName = "core";

class CoreModule : public KnightModule {
    void add_to_factory(KnightFactory& factory) override {
        // Register analyses.
        factory.register_analysis< analyzer::SymbolResolver >();
        factory.register_analysis< analyzer::NumericalAnalysis >();

        // Register checkers.
    }
}; // class CoreModule

/// \brief Register the AbseilModule using this statically initialized variable.
static KnightModuleRegistry::Add< CoreModule > X( // NOLINT
    ModName,
    "Add core analyses and checkers.");

/// \brief This anchor is used to force the linker to link in the generated
/// object file and thus register the CoreModule.
volatile int CoreModuleAnchorSource = 0; // NOLINT

} // namespace knight