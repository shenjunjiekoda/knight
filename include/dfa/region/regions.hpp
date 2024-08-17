

//===- regions.hpp ----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the region metadatas.
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/ADT/StringRef.h>

#ifdef REGION_DEF
#    undef REGION_DEF
#endif

namespace knight::dfa {

enum class RegionKind : unsigned {
    None,
#define REGION_DEF(KIND, DESC, PARENT) KIND, // NOLINT
#include "dfa/def/regions.def"
}; // enum class RegionKind

inline llvm::StringRef get_region_kind_name(RegionKind kind) {
    switch (kind) {
#undef REGION_DEF
// NOLINTNEXTLINE
#define REGION_DEF(KIND, DESC, PARENT) \
    case RegionKind::KIND:             \
        return #KIND;
#include "dfa/def/regions.def"
        default:
            return "Unknown";
    }
}

inline llvm::StringRef get_region_kind_desc(RegionKind kind) {
    switch (kind) {
#undef REGION_DEF
// NOLINTNEXTLINE
#define REGION_DEF(KIND, DESC, PARENT) \
    case RegionKind::KIND:             \
        return DESC;
#include "dfa/def/regions.def"
        default:
            return "Unknown";
    }
}

inline RegionKind get_parent_kind(RegionKind kind) {
    switch (kind) {
        using enum RegionKind;
#undef REGION_DEF
// NOLINTNEXTLINE
#define REGION_DEF(KIND, DESC, PARENT) \
    case RegionKind::KIND:             \
        return PARENT;
#include "dfa/def/regions.def"
        default:
            return None;
    }
}

} // namespace knight::dfa
