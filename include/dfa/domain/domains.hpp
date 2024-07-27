//===- domains.hpp ----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the domain metadatas.
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/ADT/StringRef.h>

#ifdef DOMAIN_DEF
#    undef DOMAIN_DEF
#endif

namespace knight::dfa {

enum class DomainKind {
#define DOMAIN_DEF(KIND, NAME, ID, DESC) KIND,
#include "domains.def"
}; // enum class DomainKind

inline llvm::StringRef get_domain_name(DomainKind kind) {
    switch (kind) {
#undef DOMAIN_DEF
#define DOMAIN_DEF(KIND, NAME, ID, DESC)                                       \
    case DomainKind::KIND:                                                     \
        return NAME;
#include "domains.def"
    default:
        return "Unknown";
    }
}

inline llvm::StringRef get_domain_desc(DomainKind kind) {
    switch (kind) {
#undef DOMAIN_DEF
#define DOMAIN_DEF(KIND, NAME, ID, DESC)                                       \
    case DomainKind::KIND:                                                     \
        return DESC;
#include "domains.def"
    default:
        return "Unknown";
    }
}

inline uint8_t get_domain_id(DomainKind kind) {
    switch (kind) {
#undef DOMAIN_DEF
#define DOMAIN_DEF(KIND, NAME, ID, DESC)                                       \
    case DomainKind::KIND:                                                     \
        return ID;
#include "domains.def"
    default:
        return 0;
    }
}

} // namespace knight::dfa
