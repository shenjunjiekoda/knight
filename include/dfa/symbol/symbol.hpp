//===- symbol.hpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the Symbol.
//
//===------------------------------------------------------------------===//

#pragma once

#include <type_traits>

#include "dfa/stack_frame.hpp"
#include "support/dumpable.hpp"
#include "support/symbol.hpp"

#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <type_traits>

namespace knight::dfa {

/// \brief The kind of symbol expression.(Integer for now.)
enum class SymExprKind {
    None,
    // symbol leaf node
    SYM_BEGIN,
    RegionSymbolVal,
    RegionSymbolExtent,
    SymbolConjured,
    SYM_END,

    // cast
    CastSym,

    // unary op
    UnarySymEx,

    // binary op
    BINARY_BEGIN,
    IntSym,
    SymInt,
    SymSym,
    BINARY_END,

}; // enum class SymKind

bool is_valid_type_for_sym_expr(clang::QualType type);

class SymExpr;
class MemRegion;
class TypedRegion;

using SymbolRef = const SymExpr*;

class SymIterator {
  private:
    std::vector< const SymExpr* > m_sym_exprs;

  public:
    SymIterator() = default;
    explicit SymIterator(const SymExpr* sym_expr);

    SymIterator& operator++();
    SymbolRef operator*();

    bool operator==(const SymIterator& other) const;
    bool operator!=(const SymIterator& other) const;

  private:
    void expand();
}; // class SymIterator

/// Numerical symbol(Integer for now).
class SymExpr {
  protected:
    SymExprKind m_kind;
    mutable unsigned m_complexity{0U};

  protected:
    explicit SymExpr(SymExprKind kind) : m_kind(kind) {}

  public:
    virtual ~SymExpr() = default;

    [[nodiscard]] SymExprKind get_kind() const { return m_kind; }

    [[nodiscard]] virtual clang::QualType get_type() const = 0;

    [[nodiscard]] virtual unsigned get_worst_complexity() const = 0;

    [[nodiscard]] llvm::iterator_range< SymIterator > get_symbols() const {
        return llvm::make_range(SymIterator(this), SymIterator());
    }

    virtual const MemRegion* get_src_region() const { return nullptr; }

    virtual bool is_leaf() const { return false; }

    virtual void Profile(llvm::FoldingSetNodeID& profile) const = 0; // NOLINT

    virtual void dump(llvm::raw_ostream& os) const {}
}; // class SymExpr

using SymID = unsigned;

class Sym : public SymExpr {
  private:
    SymID m_id;

  protected:
    Sym(SymID id, SymExprKind kind) : SymExpr(kind), m_id(id) {}

  public:
    ~Sym() override = default;
    [[nodiscard]] SymID get_id() const { return m_id; }

    [[nodiscard]] virtual llvm::StringRef get_kind_name() const = 0;

    [[nodiscard]] unsigned get_worst_complexity() const override { return 1U; }

    [[nodiscard]] bool is_leaf() const override { return true; }

    [[nodiscard]] static bool classof(const SymExpr* sym_expr) {
        return sym_expr->get_kind() > SymExprKind::SYM_BEGIN &&
               sym_expr->get_kind() < SymExprKind::SYM_END;
    }
}; // class Sym

class RegionSymVal : public Sym {
  private:
    const TypedRegion* m_region;

  public:
    RegionSymVal(SymID id, const TypedRegion* region);
    ~RegionSymVal() override = default;

    [[gnu::returns_nonnull]] const TypedRegion* get_region() const;

    static void profile(llvm::FoldingSetNodeID& id, const TypedRegion* region) {
        id.AddInteger(static_cast< unsigned >(SymExprKind::RegionSymbolVal));
        id.AddPointer(region);
    }

    void Profile(llvm::FoldingSetNodeID& profile) const override { // NOLINT
        RegionSymVal::profile(profile, m_region);
    }

    [[nodiscard]] llvm::StringRef get_kind_name() const override {
        return "reg_$";
    }

    void dump(llvm::raw_ostream& os) const override;
    [[gnu::returns_nonnull, nodiscard]] const MemRegion* get_src_region()
        const override;
    [[nodiscard]] clang::QualType get_type() const override;

    static bool classof(const SymExpr* sym_expr) {
        return sym_expr->get_kind() == SymExprKind::RegionSymbolVal;
    }

    static bool classof(const Sym* sym) {
        return sym->get_kind() == SymExprKind::RegionSymbolVal;
    }

}; // class RegionSymVal

class RegionSymExtent : public Sym {
  private:
    const MemRegion* m_region;

  public:
    RegionSymExtent(SymID id, const MemRegion* region);
    ~RegionSymExtent() override = default;

    [[gnu::returns_nonnull]] const MemRegion* get_region() const;

    static void profile(llvm::FoldingSetNodeID& id, const MemRegion* region) {
        id.AddInteger(static_cast< unsigned >(SymExprKind::RegionSymbolExtent));
        id.AddPointer(region);
    }

    void Profile(llvm::FoldingSetNodeID& profile) const override { // NOLINT
        RegionSymExtent::profile(profile, m_region);
    }

    [[nodiscard]] llvm::StringRef get_kind_name() const override {
        return "extent_$";
    }

    void dump(llvm::raw_ostream& os) const override;
    [[nodiscard]] clang::QualType get_type() const override;

    static bool classof(const SymExpr* sym_expr) {
        return sym_expr->get_kind() == SymExprKind::RegionSymbolExtent;
    }

    static bool classof(const Sym* sym) {
        return sym->get_kind() == SymExprKind::RegionSymbolExtent;
    }

}; // class RegionSymExtent

class SymbolConjured : public Sym {
  private:
    const clang::Stmt* m_stmt;
    clang::QualType m_type;
    unsigned m_visit_cnt;
    const StackFrame* m_frame;
    const void* tag;

  public:
    SymbolConjured(SymID id,
                   const clang::Stmt* stmt,
                   clang::QualType type,
                   unsigned visit_cnt,
                   const StackFrame* frame,
                   const void* tag = nullptr)
        : Sym(id, SymExprKind::SymbolConjured),
          m_stmt(stmt),
          m_type(type),
          m_visit_cnt(visit_cnt),
          m_frame(frame),
          tag(tag) {
        knight_assert_msg(is_valid_type_for_sym_expr(type), "Invalid type");
    }

    ~SymbolConjured() override = default;

    [[gnu::returns_nonnull]] const clang::Stmt* get_stmt() const {
        return m_stmt;
    }

    [[nodiscard]] clang::QualType get_type() const override { return m_type; }

    [[nodiscard]] unsigned get_visit_cnt() const { return m_visit_cnt; }

    [[gnu::returns_nonnull]] const StackFrame* get_frame() const {
        return m_frame;
    }

    [[nodiscard]] const void* get_tag() const { return tag; }

    static void profile(llvm::FoldingSetNodeID& id,
                        const clang::Stmt* stmt,
                        clang::QualType type,
                        unsigned visit_cnt,
                        const StackFrame* frame,
                        const void* tag = nullptr) {
        id.AddInteger(static_cast< unsigned >(SymExprKind::SymbolConjured));
        id.AddPointer(stmt);
        id.Add(type);
        id.AddInteger(visit_cnt);
        id.AddPointer(frame);
        id.AddPointer(tag);
    }

    void Profile(llvm::FoldingSetNodeID& profile) const override { // NOLINT
        SymbolConjured::profile(profile,
                                m_stmt,
                                m_type,
                                m_visit_cnt,
                                m_frame,
                                tag);
    }

    static bool classof(const SymExpr* sym_expr) {
        return sym_expr->get_kind() == SymExprKind::SymbolConjured;
    }

    static bool classof(const Sym* sym) {
        return sym->get_kind() == SymExprKind::SymbolConjured;
    }

    [[nodiscard]] llvm::StringRef get_kind_name() const override {
        return "conj_$";
    }

    void dump(llvm::raw_ostream& os) const override;

}; // class SymbolConjured

class CastSymExpr : public SymExpr {
  private:
    const SymExpr* m_operand;

    /// \brief cast source type
    clang::QualType m_src;
    /// \brief cast destination type
    clang::QualType m_dst;

  public:
    CastSymExpr(const SymExpr* operand,
                clang::QualType src,
                clang::QualType dst)
        : SymExpr(SymExprKind::CastSym),
          m_operand(operand),
          m_src(src),
          m_dst(dst) {
        knight_assert_msg(is_valid_type_for_sym_expr(src),
                          "Invalid source type");
        knight_assert_msg(is_valid_type_for_sym_expr(dst),
                          "Invalid destination type");
    }

    [[nodiscard]] unsigned get_worst_complexity() const override;

    [[nodiscard]] bool is_leaf() const override { return true; }

    [[nodiscard]] const SymExpr* get_operand() const { return m_operand; }

    [[nodiscard]] clang::QualType get_src_type() const { return m_src; }

    [[nodiscard]] clang::QualType get_dst_type() const { return m_dst; }

    [[nodiscard]] clang::QualType get_type() const override { return m_dst; }

    void dump(llvm::raw_ostream& os) const override;

    static void profile(llvm::FoldingSetNodeID& id,
                        const SymExpr* operand,
                        clang::QualType src,
                        clang::QualType dst) {
        id.AddInteger(static_cast< unsigned >(SymExprKind::CastSym));
        id.AddPointer(operand);
        id.Add(src);
        id.Add(dst);
    }

    void Profile(llvm::FoldingSetNodeID& profile) const override { // NOLINT
        CastSymExpr::profile(profile, m_operand, m_src, m_dst);
    }

    static bool classof(const SymExpr* sym_expr) {
        return sym_expr->get_kind() == SymExprKind::CastSym;
    }

}; // class CastSymExpr

class UnarySymExpr : public SymExpr {
  private:
    const SymExpr* m_operand;
    clang::UnaryOperator::Opcode m_opcode;
    clang::QualType m_type;

  public:
    UnarySymExpr(const SymExpr* operand,
                 clang::UnaryOperator::Opcode opcode,
                 clang::QualType type)
        : SymExpr(SymExprKind::UnarySymEx),
          m_operand(operand),
          m_opcode(opcode),
          m_type(type) {
        knight_assert_msg(opcode == clang::UO_Minus || opcode == clang::UO_Not,
                          "Invalid unary operator");
        knight_assert_msg(is_valid_type_for_sym_expr(type), "Invalid type");
    }
    ~UnarySymExpr() override = default;

  public:
    [[nodiscard]] const SymExpr* get_operand() const { return m_operand; }

    [[nodiscard]] clang::UnaryOperator::Opcode get_opcode() const {
        return m_opcode;
    }

    [[nodiscard]] clang::QualType get_type() const override { return m_type; }

    [[nodiscard]] unsigned get_worst_complexity() const override;

    [[nodiscard]] bool is_leaf() const override { return true; }

    void dump(llvm::raw_ostream& os) const override;

    static void profile(llvm::FoldingSetNodeID& id,
                        const SymExpr* operand,
                        clang::UnaryOperator::Opcode opcode,
                        clang::QualType type) {
        id.AddInteger(static_cast< unsigned >(SymExprKind::UnarySymEx));
        id.AddPointer(operand);
        id.AddInteger(static_cast< unsigned >(opcode));
        id.Add(type);
    }

    void Profile(llvm::FoldingSetNodeID& profile) const override { // NOLINT
        UnarySymExpr::profile(profile, m_operand, m_opcode, m_type);
    }

    static bool classof(const SymExpr* sym_expr) {
        return sym_expr->get_kind() == SymExprKind::UnarySymEx;
    }

}; // class UnarySymExpr

template < typename LHSTy, typename RHSTy, SymExprKind Kind >
class BinarySymExpr : public SymExpr {
  private:
    LHSTy m_lhs;
    RHSTy m_rhs;
    clang::BinaryOperator::Opcode m_opcode;
    clang::QualType m_type;

  public:
    BinarySymExpr(LHSTy lhs,
                  RHSTy rhs,
                  clang::BinaryOperator::Opcode opcode,
                  clang::QualType type)
        : SymExpr(Kind),
          m_lhs(lhs),
          m_rhs(rhs),
          m_opcode(opcode),
          m_type(type) {
        knight_assert_msg(is_valid_type_for_sym_expr(type), "Invalid type");
    }

    ~BinarySymExpr() override = default;

  public:
    [[nodiscard]] LHSTy get_lhs() const { return m_lhs; }

    [[nodiscard]] RHSTy get_rhs() const { return m_rhs; }

    [[nodiscard]] clang::BinaryOperator::Opcode get_opcode() const {
        return m_opcode;
    }

    [[nodiscard]] clang::QualType get_type() const override { return m_type; }

    [[nodiscard]] unsigned get_worst_complexity() const override {
        if (m_complexity > 0U) {
            return m_complexity;
        }
        unsigned lhs_complexity = 0U;
        unsigned rhs_complexity = 0U;
        if constexpr (is_sym_expr< LHSTy >::value) {
            lhs_complexity = m_lhs->get_worst_complexity();
        }
        if constexpr (is_sym_expr< RHSTy >::value) {
            rhs_complexity = m_rhs->get_worst_complexity();
        }

        switch (m_opcode) {
            using enum clang::BinaryOperatorKind;
            case BO_Add:
            case BO_Sub:
                return std::max(lhs_complexity, rhs_complexity);
            default:
                break;
        }
        if (lhs_complexity == 0U) {
            lhs_complexity = 1U;
        }
        if (rhs_complexity == 0U) {
            rhs_complexity = 1U;
        }
        return lhs_complexity * rhs_complexity;
    }

    [[nodiscard]] bool is_leaf() const override { return true; }

    static void profile(llvm::FoldingSetNodeID& id,
                        SymExprKind kind,
                        LHSTy lhs,
                        RHSTy rhs,
                        clang::BinaryOperator::Opcode opcode,
                        clang::QualType type) {
        id.AddInteger(static_cast< unsigned >(kind));
        id.AddPointer(lhs);
        id.AddPointer(rhs);
        id.AddInteger(static_cast< unsigned >(opcode));
        id.Add(type);
    }

    void Profile(llvm::FoldingSetNodeID& profile) const override { // NOLINT
        BinarySymExpr::profile(profile, m_kind, m_lhs, m_rhs, m_opcode, m_type);
    }

    void dump(llvm::raw_ostream& os) const override {
        DumpableTrait< LHSTy >::dump(os, m_lhs);
        os << " " << clang::BinaryOperator::getOpcodeStr(m_opcode) << " ";
        DumpableTrait< RHSTy >::dump(os, m_rhs);
    }

    static bool classof(const SymExpr* sym_expr) {
        return sym_expr->get_kind() >= SymExprKind::BINARY_BEGIN &&
               sym_expr->get_kind() <= SymExprKind::BINARY_END;
    }

    static bool classof(const Sym* sym) {
        return sym->get_kind() >= SymExprKind::BINARY_BEGIN &&
               sym->get_kind() <= SymExprKind::BINARY_END;
    }

}; // class BinarySymExpr

using IntSymExpr = BinarySymExpr< int, SymbolRef, SymExprKind::IntSym >;
using SymIntExpr = BinarySymExpr< SymbolRef, int, SymExprKind::SymInt >;
using SymSymExpr = BinarySymExpr< SymbolRef, SymbolRef, SymExprKind::SymSym >;

} // namespace knight::dfa