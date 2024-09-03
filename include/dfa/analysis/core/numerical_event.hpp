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

#include "dfa/analysis/events.hpp"
#include "dfa/constraint/linear.hpp"
#include "dfa/program_state.hpp"
#include "util/log.hpp"

#include <clang/AST/OperationKinds.h>

#include <utility>
#include <variant>

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

struct ZVarAssignBinaryVarVar {
    clang::BinaryOperatorKind op;
    ZVariable x;
    ZVariable y;
    ZVariable z;
    void dump(llvm::raw_ostream& os) const {
        os << x << " = " << y << " " << clang::BinaryOperator::getOpcodeStr(op)
           << " " << z;
    }
} __attribute__((aligned(AssignEventAlignSize)));

struct ZVarAssignBinaryVarNum {
    clang::BinaryOperatorKind op;
    ZVariable x;
    ZVariable y;
    ZNum z;
    void dump(llvm::raw_ostream& os) const {
        os << x << " = " << y << " " << clang::BinaryOperator::getOpcodeStr(op)
           << " " << z;
    }
} __attribute__((aligned(AssignEventAlignSize)));

struct ZVarAssignZCast {
    unsigned dst_bit_width;
    clang::QualType dst_type;
    ZVariable x;
    ZVariable y;
    void dump(llvm::raw_ostream& os) const {
        os << x << " = (" << dst_type << ")" << y;
    }
} __attribute__((aligned(AssignEventAlignSize)));

struct LinearAssignEvent {
    static EventKind get_kind() { return EventKind::LinearAssignEvent; }

    using AssignT = std::variant< ZVarAssignZVar,
                                  ZVarAssignZNum,
                                  ZVarAssignZLinearExpr,
                                  ZVarAssignBinaryVarVar,
                                  ZVarAssignBinaryVarNum,
                                  ZVarAssignZCast >;

    AssignT assign;
    ProgramStateRef& state;

    LinearAssignEvent(AssignT assign, ProgramStateRef& state)
        : assign(std::move(assign)), state(state) {}

} __attribute__((packed))
__attribute__((aligned(AssignEventAlignBigSize))); // struct LinearAssignEvent

struct PredicateZVarZNum {
    clang::BinaryOperatorKind op;
    ZVariable x;
    ZNum y;

    void dump(llvm::raw_ostream& os) const {
        os << x << " " << clang::BinaryOperator::getOpcodeStr(op) << " " << y;
    }
} __attribute__((aligned(AssignEventAlignSize)))
__attribute__((packed)); // struct PredicateZVarZVar

struct PredicateZVarZVar {
    clang::BinaryOperatorKind op;
    ZVariable x;
    ZVariable y;

    void dump(llvm::raw_ostream& os) const {
        os << x << " " << clang::BinaryOperator::getOpcodeStr(op) << " " << y;
    }
} __attribute__((aligned(AssignEventAlignSize)))
__attribute__((packed)); // struct PredicateZVarZVar

struct GeneralLinearConstraint {
    ZLinearConstraint cstr;

    void dump(llvm::raw_ostream& os) const { os << cstr; }
} __attribute__((
    aligned(AssignEventAlignBigSize))); // struct GeneralLinearConstraint

struct LinearAssumptionEvent {
    static EventKind get_kind() { return EventKind::LinearAssumptionEvent; }

    using AssumpT = std::variant< PredicateZVarZNum,
                                  PredicateZVarZVar,
                                  GeneralLinearConstraint >;

    AssumpT assumption;
    ProgramStateRef& state;
    LinearAssumptionEvent(AssumpT assumption, ProgramStateRef& state)
        : assumption(std::move(assumption)), state(state) {}

} __attribute__((packed)); // struct LinearPredicateEvent

} // namespace knight::dfa