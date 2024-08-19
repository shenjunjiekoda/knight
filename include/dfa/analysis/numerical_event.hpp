//===- numerical_event.hpp --------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the numerical event class.
//
//===------------------------------------------------------------------===//

#pragma once

#include <utility>
#include <variant>
#include "dfa/analysis/events.hpp"
#include "dfa/constraint/linear.hpp"
#include "dfa/program_state.hpp"
#include "llvm/Support/raw_ostream.h"

namespace knight::dfa {

constexpr unsigned AssignEventAlignSmallSize = 32U;
constexpr unsigned AssignEventAlignSize = 64U;
constexpr unsigned AssignEventAlignBigSize = 128U;

struct ZVarAssignZVar {
    ZVariable x;
    ZVariable y;

    void dump(llvm::raw_ostream& os) const { os << x << " = " << y; }
} __attribute__((aligned(AssignEventAlignSmallSize)));

struct ZVarAssignZNum {
    ZVariable x;
    ZNum y;

    void dump(llvm::raw_ostream& os) const { os << x << " = " << y; }
} __attribute__((aligned(AssignEventAlignSize)));

struct ZVarAssignZLinearExpr {
    ZVariable x;
    ZLinearExpr y;
    void dump(llvm::raw_ostream& os) const { os << x << " = " << y; }
} __attribute__((aligned(AssignEventAlignBigSize)));

struct QVarAssignQVar {
    QVariable x;
    QVariable y;
    void dump(llvm::raw_ostream& os) const { os << x << " = " << y; }
} __attribute__((aligned(AssignEventAlignSmallSize)));

struct QVarAssignQNum {
    QVariable x;
    QNum y;
    void dump(llvm::raw_ostream& os) const { os << x << " = " << y; }
} __attribute__((aligned(AssignEventAlignSize)));

struct QVarAssignQLinearExpr {
    QVariable x;
    QLinearExpr y;
    void dump(llvm::raw_ostream& os) const { os << x << " = " << y; }
} __attribute__((aligned(AssignEventAlignBigSize)));

struct LinearAssignEvent {
    static EventKind get_kind() { return EventKind::LinearAssignEvent; }

    using AsignT = std::variant< ZVarAssignZVar,
                                 ZVarAssignZNum,
                                 ZVarAssignZLinearExpr,
                                 QVarAssignQVar,
                                 QVarAssignQNum,
                                 QVarAssignQLinearExpr >;

    AsignT assign;
    ProgramStateRef state;
    AnalysisContext* ctx;

    LinearAssignEvent(AsignT assign,
                      ProgramStateRef state,
                      AnalysisContext* ctx)
        : assign(std::move(assign)), state(std::move(state)), ctx(ctx) {}

} __attribute__((packed))
__attribute__((aligned(AssignEventAlignBigSize))); // struct LinearAssignEvent

} // namespace knight::dfa