//===- events.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the event metadatas.
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/ADT/StringRef.h>

#ifdef EVENT_DEF
#    undef EVENT_DEF
#endif

namespace knight::analyzer {

using EventID = uint8_t;

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
enum class EventKind {
    None,
#define EVENT_DEF(KIND, NAME, ID, DESC) KIND,
#include "analyzer/core/def/events.def"
}; // enum class EventKind

inline llvm::StringRef get_event_name(EventKind kind) {
    switch (kind) {
#undef EVENT_DEF
#define EVENT_DEF(KIND, NAME, ID, DESC) \
    case EventKind::KIND:               \
        return NAME;
#include "analyzer/core/def/events.def"
        default:
            return "Unknown";
    }
}

inline llvm::StringRef get_event_desc(EventKind kind) {
    switch (kind) {
#undef EVENT_DEF
#define EVENT_DEF(KIND, NAME, ID, DESC) \
    case EventKind::KIND:               \
        return DESC;
#include "analyzer/core/def/events.def"
        default:
            return "Unknown";
    }
}

inline EventID get_event_id(EventKind kind) {
    switch (kind) {
#undef EVENT_DEF
#define EVENT_DEF(KIND, NAME, ID, DESC) \
    case EventKind::KIND:               \
        return ID;
#include "analyzer/core/def/events.def"
        default:
            return 0;
    }
}

inline EventKind get_event_kind(EventID id) {
    switch (id) {
#undef EVENT_DEF
#define EVENT_DEF(KIND, NAME, ID, DESC) \
    case ID:                            \
        return EventKind::KIND;
#include "analyzer/core/def/events.def"
        default:
            return EventKind::None;
    }
}

inline llvm::StringRef get_event_name_by_id(EventID id) {
    return get_event_name(get_event_kind(id));
}

// NOLINTEND(cppcoreguidelines-macro-usage)

} // namespace knight::analyzer
