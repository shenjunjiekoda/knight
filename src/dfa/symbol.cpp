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
#include <clang/AST/OperationKinds.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APSInt.h>

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

std::optional< RegionRef > SymExpr::get_as_region() const {
    if (const auto* reg_sym = dyn_cast< RegionDef >(this)) {
        return reg_sym->get_region();
    }
    return std::nullopt;
}

std::optional< SymbolRef > SymExpr::get_as_symbol() const {
    if (const auto* sym = dyn_cast< Sym >(this)) {
        return sym;
    }
    return std::nullopt;
}

RegionDef::RegionDef(SymID id,
                     RegionRef region,
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

RegionRef RegionDef::get_region() const {
    return m_region;
}

void RegionDef::dump(llvm::raw_ostream& os) const {
    os << get_kind_name() << get_id() << "<" << get_type() << ' ';
    get_region()->dump(os);
    if (auto loc = get_loc_ctx()->get_source_location()) {
        auto& mgr = get_region()->get_ast_ctx().getSourceManager();
        if (loc->isFileID()) {
            auto ploc = mgr.getPresumedLoc(*loc);
            if (ploc.isValid()) {
                os << " @" << ploc.getLine() << ':' << ploc.getColumn();
            }
        }
    }
    os << '>';
}

clang::QualType RegionDef::get_type() const {
    return get_region()->get_value_type();
}

RegionSymExtent::RegionSymExtent(SymID id, RegionRef region)
    : Sym(id, SymExprKind::RegionSymbolExtent), m_region(region) {}

RegionRef RegionSymExtent::get_region() const {
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
                            m_frame->get_ast_context().getPrintingPolicy(),
                            0,
                            "\n",
                            &m_frame->get_ast_context());
    } else {
        os << "`unknown stmt`";
    }
    os << "}";
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

bool Scalar::is_integer() const {
    return get_kind() == SymExprKind::Int;
}

bool Scalar::is_region() const {
    return get_kind() == SymExprKind::Region;
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

const TypedRegion* RegionAddr::get_region() const {
    return m_region;
}

clang::QualType RegionAddr::get_type() const {
    return m_region->get_value_type();
}

void RegionAddr::dump(llvm::raw_ostream& os) const {
    os << "&" << *m_region;
}

std::optional< ZLinearConstraint > SymExpr::get_as_zconstraint() const {
    if (!get_type()->isBooleanType()) {
        return std::nullopt;
    }
    if (const auto* scalar_int = dyn_cast< ScalarInt >(this)) {
        return scalar_int->get_value() == 0 ? ZLinearConstraint::contradiction()
                                            : ZLinearConstraint::tautology();
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

} // namespace knight::dfa