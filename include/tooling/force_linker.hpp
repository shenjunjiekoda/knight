//===- force_linker.hpp -----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file is used to force the linker to link the modules
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/Support/Compiler.h>

namespace knight {

extern volatile int DemoModuleAnchorSource;
static int LLVM_ATTRIBUTE_UNUSED DemoModuleAnchorDestination =
    DemoModuleAnchorSource;

} // namespace knight