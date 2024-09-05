//===- globs.hpp ------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the utility functions for globs.
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Regex.h>

#include <vector>

namespace knight {

constexpr unsigned GlobAlignment = 32U;

/// \brief A utility class for matching comma-separated
/// strings against globs.
///
/// negative glob starts with '-'
class Globs {
  public:
    struct alignas(GlobAlignment) Glob {
        bool is_negative;
        llvm::Regex regex;
    }; // struct Glob

  private:
    std::vector< Glob > m_globs;
    mutable llvm::StringMap< bool > m_cache;

  public:
    explicit Globs(llvm::StringRef globs);
    bool matches(llvm::StringRef str) const;

}; // class Globs

} // namespace knight