//===- debug_module.cpp ------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements debug module.
//
//===------------------------------------------------------------------===//

#include "analyzer/core/analysis/analyses.hpp"
#include "analyzer/core/analysis/debug/state_printer.hpp"
#include "analyzer/core/checker/debug/inspection.hpp"
#include "analyzer/tooling/factory.hpp"
#include "analyzer/tooling/module.hpp"

namespace knight {

constexpr const char* ModName = "debug";

class DebugModule : public KnightModule {
    void add_to_factory(KnightFactory& factory) override {
        // Register analyses.
        factory.register_analysis< analyzer::StatePrinter >();

        // Register checkers.
        factory.register_checker< analyzer::InspectionChecker >();
    }
}; // class DebugModule

/// \brief Register the AbseilModule using this statically initialized variable.
static KnightModuleRegistry::Add< DebugModule > X( // NOLINT
    ModName,
    "Add debug analyses and checkers.");

/// \brief This anchor is used to force the linker to link in the generated
/// object file and thus register the DebugModule.
volatile int DebugModuleAnchorSource = 0; // NOLINT

} // namespace knight