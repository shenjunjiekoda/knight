//===- clang.hpp -------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines some llvm related utilities.
//
//===------------------------------------------------------------------===//

#pragma once

#include <regex>
#include <string>

#include <clang/AST/Decl.h>
#include <clang/Basic/TargetInfo.h>

#include "common/util/log.hpp"

namespace knight::clang_util {

std::string get_clang_include_dir();

std::string get_mangled_name(const clang::NamedDecl* named_decl,
                             const clang::TargetInfo* target_info = nullptr);

} // namespace knight::clang_util