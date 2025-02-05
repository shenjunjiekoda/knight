//===- pointer_event.hpp ----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the pointer event class.
//
//===------------------------------------------------------------------===//

#pragma once

#include "analyzer/core/analysis/events.hpp"
#include "analyzer/core/program_state.hpp"

namespace knight::analyzer {

struct PointerAssignEvent { // NOLINT(altera-struct-pack-align)
    static EventKind get_kind() { return EventKind::PointerAssignEvent; }

    RegionRef src_region;
    RegionRef dst_region;

    ProgramStateRef& state;
    PointerAssignEvent(RegionRef src_region,
                       RegionRef dst_region,
                       ProgramStateRef& state)
        : src_region(src_region), dst_region(dst_region), state(state) {}

}; // struct PointerAssignEvent

} // namespace knight::analyzer