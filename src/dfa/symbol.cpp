//===- symbol.cpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the symbols.
//
//===------------------------------------------------------------------===//

#include "dfa/symbol.hpp"
#include "clang/AST/OperationKinds.h"
#include "dfa/constraint/linear.hpp"
#include "dfa/region/region.hpp"
#include "dfa/stack_frame.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APSInt.h"

#include <clang/AST/Expr.h>

#include <optional>
#include <queue>

namespace knight::dfa {

bool is_valid_type_for_sym_expr(clang::QualType type) {
    return !type.isNull() && !type->isVoidType();
}

void profile_symbol(llvm::FoldingSetNodeID& id, SymbolRef sym) {
    sym->Profile(id);
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const SymbolRef& var) {
    var->dump(os);
    return os;
}

void SymIterator::resolve(SExprRef sym_expr) {
    std::queue< SExprRef > sym_exprs;
    sym_exprs.push(sym_expr);

    while (!sym_exprs.empty()) {
        SExprRef back = sym_exprs.back();
        sym_exprs.pop();
        switch (back->get_kind()) {
            using enum SymExprKind;
            case RegionSymbolVal:
            case RegionSymbolExtent:
            case SymbolConjured: {
                if (std::find(m_syms.begin(), m_syms.end(), back) ==
                    m_syms.end()) {
                    m_syms.push_back(cast< Sym >(back));
                }
            } break;
            case CastSym: {
                sym_exprs.push(cast< CastSymExpr >(back)->get_operand());
            } break;
            case UnarySymEx: {
                sym_exprs.push(cast< UnarySymExpr >(back)->get_operand());
            } break;
            case BinarySymEx: {
                sym_exprs.push(cast< BinarySymExpr >(back)->get_lhs());
                sym_exprs.push(cast< BinarySymExpr >(back)->get_rhs());
            } break;
            default:
                break;
        }
    }
}

SymIterator::SymIterator(const SymExpr* sym_expr) {
    resolve(sym_expr);
}

SymIterator& SymIterator::operator++() {
    assert(!m_syms.empty() &&
           "iterator incremented past the end of the symbol set");
    m_syms.pop_back();
    return *this;
}

const Sym* SymIterator::operator*() {
    assert(!m_syms.empty() && "end iterator dereferenced");
    return m_syms.back();
}

RegionSymVal::RegionSymVal(SymID id,
                           const TypedRegion* region,
                           const LocationContext* loc_ctx,
                           bool is_external)
    : Sym(id, SymExprKind::RegionSymbolVal),
      m_loc_ctx(loc_ctx),
      m_region(region),
      m_is_external(is_external) {
    knight_assert_msg(region != nullptr, "Region cannot be null");
    knight_assert_msg(is_valid_type_for_sym_expr(m_region->get_value_type()),
                      "Invalid type for region symbol value");
}

const TypedRegion* RegionSymVal::get_region() const {
    return m_region;
}

void RegionSymVal::dump(llvm::raw_ostream& os) const {
    os << get_kind_name() << get_id() << "<" << get_type() << ' ';
    get_region()->dump(os);
    os << '>';
}

std::optional< const MemRegion* > RegionSymVal::get_as_region() const {
    return m_region;
}

clang::QualType RegionSymVal::get_type() const {
    return get_region()->get_value_type();
}

RegionSymExtent::RegionSymExtent(SymID id, const MemRegion* region)
    : Sym(id, SymExprKind::RegionSymbolExtent), m_region(region) {}

const MemRegion* RegionSymExtent::get_region() const {
    return m_region;
}

void RegionSymExtent::dump(llvm::raw_ostream& os) const {
    os << get_kind_name() << get_id() << "<" << get_type() << ' '
       << get_region() << '>';
}

clang::QualType RegionSymExtent::get_type() const {
    return get_region()->get_manager().get_ast_ctx().getSizeType();
}

void SymbolConjured::dump(llvm::raw_ostream& os) const {
    os << get_kind_name() << get_id() << "(" << get_type() << "){";
    if (m_stmt != nullptr) {
        m_stmt->printPretty(os,
                            nullptr,
                            m_frame->get_ast_context().getPrintingPolicy());
    } else {
        os << "`unknown stmt`";
    }
    os << "}";
}

unsigned UnarySymExpr::get_worst_complexity() const {
    if (m_complexity == 0U) {
        m_complexity = m_operand->get_worst_complexity();
    }
    return m_complexity;
}

void UnarySymExpr::dump(llvm::raw_ostream& os) const {
    os << clang::UnaryOperator::getOpcodeStr(m_opcode);
    auto non_leaf_op = !m_operand->is_leaf();
    if (non_leaf_op) {
        os << "(";
    }
    m_operand->dump(os);
    if (non_leaf_op) {
        os << ")";
    }
}

unsigned CastSymExpr::get_worst_complexity() const {
    if (m_complexity == 0U) {
        m_complexity = get_worst_complexity();
    }
    return m_complexity;
}

void CastSymExpr::dump(llvm::raw_ostream& os) const {
    os << "cast<" << m_dst << ">(";
    m_operand->dump(os);
    os << ")";
}

std::optional< ZLinearExpr > SymExpr::get_as_zexpr() const {
    if (!get_type()->isIntegralOrEnumerationType()) {
        return std::nullopt;
    }
    if (const auto* sym = dyn_cast< Sym >(this)) {
        return ZLinearExpr(ZVariable(sym));
    }
    if (const auto* scalar_int = dyn_cast< ScalarInt >(this)) {
        return ZLinearExpr(scalar_int->get_value());
    }
    if (const auto* unary = dyn_cast< UnarySymExpr >(this)) {
        switch (unary->get_opcode()) {
            using enum clang::UnaryOperatorKind;
            case clang::UO_Plus:
                return unary->get_operand()->get_as_zexpr();
            case clang::UO_Minus: {
                if (auto zexpr_opt = unary->get_operand()->get_as_zexpr()) {
                    return -zexpr_opt.value();
                }
                break;
                default:
                    break;
            }
        }
        return std::nullopt;
    }

    if (const auto* binary = dyn_cast< BinarySymExpr >(this)) {
        auto lhs = binary->get_lhs()->get_as_zexpr();
        auto rhs = binary->get_rhs()->get_as_zexpr();
        if (!lhs || !rhs) {
            return std::nullopt;
        }
        switch (binary->get_opcode()) {
            using enum clang::BinaryOperatorKind;
            case clang::BO_Add:
                return lhs.value() + rhs.value();
            case clang::BO_Sub:
                return lhs.value() - rhs.value();
            case clang::BO_Mul:
                if (lhs->is_constant()) {
                    return lhs.value().get_constant_term() * rhs.value();
                } else if (rhs->is_constant()) {
                    return lhs.value() * rhs.value().get_constant_term();
                } else {
                    return std::nullopt;
                }
            default:
                break;
        }
    }
    return std::nullopt;
}

std::optional< ZLinearConstraint > SymExpr::get_as_zconstraint() const {
    if (!get_type()->isBooleanType()) {
        return std::nullopt;
    }
    if (const auto* scalar_int = dyn_cast< ScalarInt >(this)) {
        return scalar_int->get_value() == 0 ? ZLinearConstraint::contradiction()
                                            : ZLinearConstraint::tautology();
    }
    if (const auto* unary = dyn_cast< UnarySymExpr >(this)) {
        if (unary->get_opcode() == clang::UO_LNot) {
            if (auto zconstraint = unary->get_operand()->get_as_zconstraint()) {
                return zconstraint.value().negate();
            }
        }
        return std::nullopt;
    }
    if (const auto* binary = dyn_cast< BinarySymExpr >(this)) {
        auto lhs = binary->get_lhs()->get_as_zexpr();
        auto rhs = binary->get_rhs()->get_as_zexpr();
        if (!lhs || !rhs) {
            return std::nullopt;
        }
        switch (binary->get_opcode()) {
            using enum clang::BinaryOperatorKind;
            case clang::BO_EQ:
                return lhs.value() == rhs.value();
            case clang::BO_NE:
                return lhs.value() != rhs.value();
            case clang::BO_LT:
                return lhs.value() <= (rhs.value() - ZNum(1));
            case clang::BO_GT:
                return lhs.value() >= (rhs.value() + ZNum(1));
            case clang::BO_LE:
                return lhs.value() <= rhs.value();
            case clang::BO_GE:
                return lhs.value() >= rhs.value();
            default:
                break;
        }
    }

    return std::nullopt;
}

std::optional< QLinearExpr > SymExpr::get_as_qexpr() const {
    if (!get_type()->isFloatingType()) {
        return std::nullopt;
    }
    if (const auto* sym = dyn_cast< Sym >(this)) {
        return QLinearExpr(QVariable(sym));
    }
    if (const auto* scalar_fp = dyn_cast< ScalarFloat >(this)) {
        return QLinearExpr(scalar_fp->get_value());
    }
    if (const auto* unary = dyn_cast< UnarySymExpr >(this)) {
        switch (unary->get_opcode()) {
            using enum clang::UnaryOperatorKind;
            case clang::UO_Plus:
                return unary->get_operand()->get_as_qexpr();
            case clang::UO_Minus: {
                if (auto qexpr_opt = unary->get_operand()->get_as_qexpr()) {
                    return -qexpr_opt.value();
                }
                break;
                default:
                    break;
            }
                return std::nullopt;
        }
    }

    if (const auto* binary = dyn_cast< BinarySymExpr >(this)) {
        auto lhs = binary->get_lhs()->get_as_qexpr();
        auto rhs = binary->get_rhs()->get_as_qexpr();
        if (!lhs || !rhs) {
            return std::nullopt;
        }
        switch (binary->get_opcode()) {
            using enum clang::BinaryOperatorKind;
            case clang::BO_Add:
                return lhs.value() + rhs.value();
            case clang::BO_Sub:
                return lhs.value() - rhs.value();
            case clang::BO_Mul:
                if (lhs->is_constant()) {
                    return lhs.value().get_constant_term() * rhs.value();
                } else if (rhs->is_constant()) {
                    return lhs.value() * rhs.value().get_constant_term();
                } else {
                    return std::nullopt;
                }
            default:
                break;
        }
    }
    return std::nullopt;
}

std::optional< QLinearConstraint > SymExpr::get_as_qconstraint() const {
    if (!get_type()->isBooleanType()) {
        return std::nullopt;
    }
    if (const auto* scalar_int = dyn_cast< ScalarInt >(this)) {
        return scalar_int->get_value() == 0 ? QLinearConstraint::contradiction()
                                            : QLinearConstraint::tautology();
    }
    if (const auto* unary = dyn_cast< UnarySymExpr >(this)) {
        if (unary->get_opcode() == clang::UO_LNot) {
            if (auto qconstraint = unary->get_operand()->get_as_qconstraint()) {
                return qconstraint.value().negate();
            }
            return std::nullopt;
        }
    }
    if (const auto* binary = dyn_cast< BinarySymExpr >(this)) {
        auto lhs = binary->get_lhs()->get_as_qexpr();
        auto rhs = binary->get_rhs()->get_as_qexpr();
        if (!lhs || !rhs) {
            return std::nullopt;
        }
        switch (binary->get_opcode()) {
            using enum clang::BinaryOperatorKind;
            case clang::BO_EQ:
                return lhs.value() == rhs.value();
            case clang::BO_NE:
                return lhs.value() != rhs.value();
            case clang::BO_LT:
                return lhs.value() <= (rhs.value() - QNum(1.0));
            case clang::BO_GT:
                return lhs.value() >= (rhs.value() + QNum(1.0));
            case clang::BO_LE:
                return lhs.value() <= rhs.value();
            case clang::BO_GE:
                return lhs.value() >= rhs.value();
            default:
                break;
        }
    }

    return std::nullopt;
}

std::optional< ZNum > SymExpr::get_as_znum() const {
    if (auto zexpr_opt = get_as_zexpr()) {
        if (zexpr_opt->is_constant()) {
            return zexpr_opt.value().get_constant_term();
        }
    }

    return std::nullopt;
}

std::optional< ZVariable > SymExpr::get_as_zvariable() const {
    if (auto zexpr_opt = get_as_zexpr()) {
        return zexpr_opt->get_as_single_variable();
    }

    return std::nullopt;
}

std::optional< QNum > SymExpr::get_as_qnum() const {
    if (auto qexpr_opt = get_as_qexpr()) {
        if (qexpr_opt->is_constant()) {
            return qexpr_opt.value().get_constant_term();
        }
    }

    return std::nullopt;
}

std::optional< QVariable > SymExpr::get_as_qvariable() const {
    if (auto qexpr_opt = get_as_qexpr()) {
        return qexpr_opt->get_as_single_variable();
    }

    return std::nullopt;
}

} // namespace knight::dfa