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
#include "dfa/constraint/linear.hpp"
#include "dfa/region/region.hpp"
#include "dfa/stack_frame.hpp"

#include <clang/AST/Expr.h>

#include <queue>

namespace knight::dfa {

bool is_valid_type_for_sym_expr(clang::QualType type) {
    return !type.isNull() && !type->isVoidType();
}

void profile_symbol(llvm::FoldingSetNodeID& id, SymbolRef sym) {
    sym->Profile(id);
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
    os << get_kind_name() << get_id() << "<" << get_type() << ' '
       << get_region() << '>';
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
    os << get_kind_name() << get_id() << "{" << get_type();
    if (m_stmt != nullptr) {
        os << " at ";
        m_stmt->printPretty(os,
                            nullptr,
                            m_frame->get_ast_context().getPrintingPolicy());
    } else {
        os << " at unknown stmt";
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
    // TODO: handle more cases
    if (const auto* sym = dyn_cast< Sym >(this);
        sym != nullptr && sym->get_type()->isIntegralOrEnumerationType()) {
        return ZLinearExpr(ZVariable(sym));
    }
    if (const auto* scalar_int = dyn_cast< ScalarInt >(this)) {
        return ZLinearExpr(scalar_int->get_value());
    }
    return std::nullopt;
}

std::optional< ZLinearConstraint > SymExpr::get_as_zconstraint() const {
    // TODO: handle more cases
    return std::nullopt;
}

std::optional< QLinearExpr > SymExpr::get_as_qexpr() const {
    // TODO: handle more cases
    if (const auto* sym = dyn_cast< Sym >(this);
        sym != nullptr && sym->get_type()->isFloatingType()) {
        return QLinearExpr(QVariable(sym));
    }
    if (const auto* scalar_fp = dyn_cast< ScalarFloat >(this)) {
        return QLinearExpr(scalar_fp->get_value());
    }
    return std::nullopt;
}

std::optional< QLinearConstraint > SymExpr::get_as_qconstraint() const {
    // TODO: handle more cases
    return std::nullopt;
}

} // namespace knight::dfa