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

#include "dfa/analysis/analyses.hpp"
#include "dfa/analysis/debug/state_printer.hpp"
#include "tooling/factory.hpp"
#include "tooling/module.hpp"

namespace knight {

constexpr const char* ModName = "debug";

class DebugModule : public KnightModule {
    void add_to_factory(KnightFactory& factory) override {
        // Register analyses.
        factory.register_analysis< dfa::StatePrinter >();

        // Register checkers.
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