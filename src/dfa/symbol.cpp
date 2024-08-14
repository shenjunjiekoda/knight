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
#include "dfa/region/region.hpp"
#include "dfa/stack_frame.hpp"

#include <clang/AST/Expr.h>

namespace knight::dfa {

bool is_valid_type_for_sym_expr(clang::QualType type) {
    return !type.isNull() && !type->isVoidType();
}

void SymIterator::expand() {
    // TODO(sym): implement this
}

SymIterator::SymIterator(const SymExpr* sym_expr) {
    m_sym_exprs.push_back(sym_expr);
}

SymIterator& SymIterator::operator++() {
    assert(!m_sym_exprs.empty() &&
           "iterator incremented past the end of the symbol expression");
    expand();
    return *this;
}

SExprRef SymIterator::operator*() {
    assert(!m_sym_exprs.empty() && "end iterator dereferenced");
    return m_sym_exprs.back();
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

} // namespace knight::dfa