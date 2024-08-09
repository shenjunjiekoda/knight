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

namespace knight {

// NOLINTBEGIN
extern volatile int DemoModuleAnchorSource;
static int DemoModuleAnchorDestination [[maybe_unused]] =
    DemoModuleAnchorSource;

extern volatile int CoreModuleAnchorSource;
static int CoreModuleAnchorDestination [[maybe_unused]] =
    CoreModuleAnchorSource;
// NOLINTEND

} // namespace knight