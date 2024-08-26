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

using DomID = uint8_t;

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

enum class DomainKind {
    None,
#define DOMAIN_DEF(KIND, NAME, ID, DESC) KIND,
#include "dfa/def/domains.def"
}; // enum class DomainKind

inline constexpr llvm::StringRef get_domain_name(DomainKind kind) {
    switch (kind) {
#undef DOMAIN_DEF
#define DOMAIN_DEF(KIND, NAME, ID, DESC) \
    case DomainKind::KIND:               \
        return NAME;
#include "dfa/def/domains.def"
        default:
            return "Unknown";
    }
}

inline constexpr llvm::StringRef get_domain_desc(DomainKind kind) {
    switch (kind) {
#undef DOMAIN_DEF
#define DOMAIN_DEF(KIND, NAME, ID, DESC) \
    case DomainKind::KIND:               \
        return DESC;
#include "dfa/def/domains.def"
        default:
            return "Unknown";
    }
}

inline constexpr uint8_t get_domain_id(DomainKind kind) {
    switch (kind) {
#undef DOMAIN_DEF
#define DOMAIN_DEF(KIND, NAME, ID, DESC) \
    case DomainKind::KIND:               \
        return ID;
#include "dfa/def/domains.def"
        default:
            return 0;
    }
}

inline constexpr DomainKind get_domain_kind(DomID id) {
    switch (id) {
#undef DOMAIN_DEF
#define DOMAIN_DEF(KIND, NAME, ID, DESC) \
    case ID:                             \
        return DomainKind::KIND;
#include "dfa/def/domains.def"
        default:
            return DomainKind::None;
    }
}

inline constexpr llvm::StringRef get_domain_name_by_id(DomID id) {
    return get_domain_name(get_domain_kind(id));
}

// NOLINTEND(cppcoreguidelines-macro-usage)

} // namespace knight::dfa
