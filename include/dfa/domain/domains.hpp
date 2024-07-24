#pragma once

// #ifndef DOMAIN_DEF
// /// Create a new domain metadata.
// ///
// /// @param KIND The kind of the domain.
// /// @param NAME The name of the domain.
// /// @param ID The unique ID of the domain.
// /// @param DESC The description of the domain.
// #define DOMAIN_DEF(KIND, NAME, ID, DESC)
// #endif

// DOMAIN_DEF(DemoItvDom, "DemoItvDom", 0, "A demo interval domain.")

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
#define DOMAIN_DEF(KIND, NAME, ID, DESC)                                       \
    case DomainKind::KIND:                                                     \
        return ID;
#include "domains.def"
    default:
        return 0;
    }
}

} // namespace knight::dfa
