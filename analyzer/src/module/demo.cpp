//===- demo_module.cpp ------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements demo module for testing purpose.
//
//===------------------------------------------------------------------===//

#include "analyzer/core/analysis/analyses.hpp"
#include "analyzer/core/analysis/demo/demo_analysis.hpp"
#include "analyzer/core/checker/demo/demo_checker.hpp"
#include "analyzer/tooling/factory.hpp"
#include "analyzer/tooling/module.hpp"

namespace knight {

constexpr const char* ModName = "demo";

class DemoModule : public KnightModule {
    void add_to_factory(KnightFactory& factory) override {
        // Register analyses.
        factory.register_analysis< analyzer::DemoAnalysis >();
        // Register checkers.
        factory.register_checker< analyzer::DemoChecker >();
    }
}; // class DemoModule

/// \brief Register the AbseilModule using this statically initialized variable.
static KnightModuleRegistry::Add< DemoModule > X( // NOLINT
    ModName,
    "Add demo analyses and checkers.");

/// \brief This anchor is used to force the linker to link in the generated
/// object file and thus register the DemoModule.
volatile int DemoModuleAnchorSource = 0; // NOLINT

} // namespace knight