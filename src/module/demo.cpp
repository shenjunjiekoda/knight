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

#include "dfa/analysis/analyses.hpp"
#include "dfa/analysis/demo_analysis.hpp"
#include "dfa/checker/demo_checker.hpp"
#include "tooling/factory.hpp"
#include "tooling/module.hpp"

namespace knight {

constexpr const char* MOD_NAME = "demo";

class DemoModule : public KnightModule {
    void add_to_factory(KnightFactory& factory) override {
        // Register analyses.
        factory.register_analysis< dfa::DemoAnalysis >();

        // Register checkers.
        factory.register_checker< dfa::DemoChecker >();
    }
}; // class DemoModule

/// \brief Register the AbseilModule using this statically initialized variable.
static KnightModuleRegistry::Add< DemoModule > X(
    MOD_NAME, "Add demo analyses and checkers.");

/// \brief This anchor is used to force the linker to link in the generated
/// object file and thus register the DemoModule.
volatile int DemoModuleAnchorSource = 0;

} // namespace knight