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

struct alignas(AssignEventAlignSmallSize) ZVarAssignZVar {
    ZVariable x;
    ZVariable y;
};

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                     const ZVarAssignZVar& e) {
    os << e.x << " = " << e.y;
    return os;
}

struct alignas(AssignEventAlignSize) ZVarAssignZNum {
    ZVariable x;
    ZNum y;
};

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                     const ZVarAssignZNum& e) {
    os << e.x << " = " << e.y;
    return os;
}

struct alignas(AssignEventAlignBigSize) ZVarAssignZLinearExpr {
    ZVariable x;
    ZLinearExpr y;
};

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                     const ZVarAssignZLinearExpr& e) {
    os << e.x << " = " << e.y;
    return os;
}

struct alignas(AssignEventAlignSize) ZVarAssignBinaryVarVar {
    clang::BinaryOperatorKind op;
    ZVariable x;
    ZVariable y;
    ZVariable z;
};

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                     const ZVarAssignBinaryVarVar& e) {
    os << e.x << " = " << e.y << " "
       << clang::BinaryOperator::getOpcodeStr(e.op) << " " << e.z;
    return os;
}

struct alignas(AssignEventAlignSize) ZVarAssignBinaryVarNum {
    clang::BinaryOperatorKind op;
    ZVariable x;
    ZVariable y;
    ZNum z;
};

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                     const ZVarAssignBinaryVarNum& e) {
    os << e.x << " = " << e.y << " "
       << clang::BinaryOperator::getOpcodeStr(e.op) << " " << e.z;
    return os;
}

struct alignas(AssignEventAlignSize) ZVarAssignZCast {
    unsigned dst_bit_width;
    clang::QualType dst_type;
    ZVariable x;
    ZVariable y;
};

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                     const ZVarAssignZCast& e) {
    os << e.x << " = (" << e.dst_type << ")" << e.y;
    return os;
}

struct alignas(AssignEventAlignBigSize) LinearAssignEvent {
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

}; // struct LinearAssignEvent

struct alignas(AssignEventAlignSize) PredicateZVarZNum {
    clang::BinaryOperatorKind op;
    ZVariable x;
    ZNum y;

}; // struct PredicateZVarZVar

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                     const PredicateZVarZNum& e) {
    os << e.x << " " << clang::BinaryOperator::getOpcodeStr(e.op) << " " << e.y;
    return os;
}

struct alignas(AssignEventAlignSize) PredicateZVarZVar {
    clang::BinaryOperatorKind op;
    ZVariable x;
    ZVariable y;
}; // struct PredicateZVarZVar

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                     const PredicateZVarZVar& e) {
    os << e.x << " " << clang::BinaryOperator::getOpcodeStr(e.op) << " " << e.y;
    return os;
}

struct alignas(AssignEventAlignBigSize) GeneralLinearConstraint {
    ZLinearConstraint cstr;

}; // struct GeneralLinearConstraint

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                     const GeneralLinearConstraint& e) {
    os << e.cstr;
    return os;
}

struct LinearAssumptionEvent { // NOLINT(altera-struct-pack-align)
    static EventKind get_kind() { return EventKind::LinearAssumptionEvent; }

    using AssumpT = std::variant< PredicateZVarZNum,
                                  PredicateZVarZVar,
                                  GeneralLinearConstraint >;

    AssumpT assumption;
    ProgramStateRef& state;
    LinearAssumptionEvent(AssumpT assumption, ProgramStateRef& state)
        : assumption(std::move(assumption)), state(state) {}

}; // struct LinearPredicateEvent

} // namespace knight::dfa