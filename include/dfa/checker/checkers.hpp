//===- checkers.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the checker metadatas.
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/ADT/StringRef.h>

#ifdef CHECKER_DEF
#    undef CHECKER_DEF
#endif

namespace knight::dfa {

using CheckerID = uint8_t;

enum class CheckerKind {
    None,
#define CHECKER_DEF(KIND, NAME, ID, DESC) KIND,
#include "checkers.def"
}; // enum class CheckerKind

inline llvm::StringRef get_checker_name(CheckerKind kind) {
    switch (kind) {
#undef CHECKER_DEF
#define CHECKER_DEF(KIND, NAME, ID, DESC) \
    case CheckerKind::KIND:               \
        return NAME;
#include "checkers.def"
        default:
            return "Unknown";
    }
}

inline llvm::StringRef get_checker_desc(CheckerKind kind) {
    switch (kind) {
#undef CHECKER_DEF
#define CHECKER_DEF(KIND, NAME, ID, DESC) \
    case CheckerKind::KIND:               \
        return DESC;
#include "checkers.def"
        default:
            return "Unknown";
    }
}

inline CheckerID get_checker_id(CheckerKind kind) {
    switch (kind) {
#undef CHECKER_DEF
#define CHECKER_DEF(KIND, NAME, ID, DESC) \
    case CheckerKind::KIND:               \
        return ID;
#include "checkers.def"
        default:
            return 0;
    }
}

inline CheckerKind get_checker_kind(CheckerID id) {
    switch (id) {
#undef CHECKER_DEF
#define CHECKER_DEF(KIND, NAME, ID, DESC) \
    case ID:                              \
        return CheckerKind::KIND;
#include "checkers.def"
        default:
            return CheckerKind::None;
    }
}

inline llvm::StringRef get_checker_name_by_id(CheckerID id) {
    return get_checker_name(get_checker_kind(id));
}

} // namespace knight::dfa
