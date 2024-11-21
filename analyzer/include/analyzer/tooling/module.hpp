//===- module.hpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the knight module which contains different analyses
//  and checkers.
//
//===------------------------------------------------------------------===//

#pragma once

#include "analyzer/tooling/options.hpp"

#include <llvm/Support/Registry.h>

namespace knight {

class KnightFactory;

/// A knight module groups a number of \c analyses and \c checkers
/// with a unified group name.
class KnightModule {
  public:
    virtual ~KnightModule() {}

    /// \brief register all \c factories belonging to this module.
    virtual void add_to_factory(KnightFactory& factory) = 0;

    /// \brief Gets default options for checkers and analyses defined
    /// in this module.
    virtual KnightOptions get_module_options() { return {}; }
}; // class KnightModule

using KnightModuleRegistry = llvm::Registry< KnightModule >;

} // namespace knight