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

#include <optional>
#include <type_traits>
#include <utility>

#include "dfa/constraint/linear.hpp"
#include "dfa/location_context.hpp"
#include "dfa/stack_frame.hpp"
#include "support/dumpable.hpp"
#include "support/symbol.hpp"

#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/APSInt.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

/// \brief The kind of symbol expression.
enum class SymExprKind {
    None,

    SCALAR_BEGIN,
    Int,
    Float,
    // TODO(scalar-sexpr): add more scalar types here
    // TODO(scalar-sexpr): Shall we add a region type here?
    SCALAR_END,

    // symbol leaf node
    SYM_BEGIN,
    RegionSymbolVal,    // region val
    RegionSymbolExtent, // region extent
    SymbolConjured,     // stmt val
    SYM_END,

    // cast
    CastSym,

    // unary op
    UnarySymEx,

    // binary op
    BinarySymEx

}; // enum class SymKind

bool is_valid_type_for_sym_expr(clang::QualType type);

class SymExpr;
class MemRegion;
class TypedRegion;
class Sym;

using SExprRef = const SymExpr*;
using SymbolRef = const Sym*;

class SymIterator {
  private:
    std::vector< const Sym* > m_syms;

  public:
    SymIterator() = default;
    explicit SymIterator(SExprRef sym_expr);

    SymIterator& operator++();
    const Sym* operator*();

    bool operator==(const SymIterator& other) const;
    bool operator!=(const SymIterator& other) const;

  private:
    void resolve(SExprRef sym_expr);
}; // class SymIterator

/// Numerical symbol(Integer for now).
class SymExpr : public llvm::FoldingSetNode {
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

    virtual std::optional< const MemRegion* > get_as_region() const {
        return std::nullopt;
    }

    std::optional< ZLinearExpr > get_as_zexpr() const;
    std::optional< ZLinearConstraint > get_as_zconstraint() const;
    std::optional< QLinearExpr > get_as_qexpr() const;
    std::optional< QLinearConstraint > get_as_qconstraint() const;

    virtual bool is_leaf() const { return false; }

    virtual void Profile(llvm::FoldingSetNodeID& profile) const = 0; // NOLINT

    virtual void dump(llvm::raw_ostream& os) const {}
}; // class SymExpr

class Scalar : public SymExpr {
  protected:
    explicit Scalar(SymExprKind kind) : SymExpr(kind) {}

  public:
    ~Scalar() override = default;

    [[nodiscard]] bool is_leaf() const override { return true; }
    [[nodiscard]] virtual bool is_integer() const { return false; }
    [[nodiscard]] virtual bool is_float() const { return false; }
    [[nodiscard]] unsigned get_worst_complexity() const override { return 0U; }
    [[nodiscard]] static bool classof(const SymExpr* sym_expr) {
        return sym_expr->get_kind() >= SymExprKind::SCALAR_BEGIN &&
               sym_expr->get_kind() <= SymExprKind::SCALAR_END;
    }
}; // class Scalar

class ScalarInt : public Scalar {
  private:
    llvm::APSInt m_value;
    clang::QualType m_type;

  public:
    explicit ScalarInt(llvm::APSInt value, clang::QualType type)
        : Scalar(SymExprKind::Int), m_value(std::move(value)), m_type(type) {}

    [[nodiscard]] llvm::APSInt get_value() const { return m_value; }
    [[nodiscard]] bool is_integer() const override { return true; }
    [[nodiscard]] static bool classof(const SymExpr* sym_expr) {
        return sym_expr->get_kind() == SymExprKind::Int;
    }
    [[nodiscard]] static bool classof(const Scalar* scalar) {
        return scalar->get_kind() == SymExprKind::Int;
    }

    [[nodiscard]] clang::QualType get_type() const override { return m_type; }

    static void profile(llvm::FoldingSetNodeID& id,
                        const llvm::APSInt& value,
                        clang::QualType type) {
        id.AddInteger(static_cast< unsigned >(SymExprKind::Int));
        id.Add(value);
        id.Add(type);
    }

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        ScalarInt::profile(id, m_value, m_type);
    }
}; // class Integer

class ScalarFloat : public Scalar {
  private:
    llvm::APFloat m_value;
    clang::QualType m_type;

  public:
    explicit ScalarFloat(llvm::APFloat value, clang::QualType type)
        : Scalar(SymExprKind::Float), m_value(std::move(value)), m_type(type) {}

    [[nodiscard]] llvm::APFloat get_value() const { return m_value; }
    [[nodiscard]] bool is_float() const override { return true; }
    [[nodiscard]] static bool classof(const SymExpr* sym_expr) {
        return sym_expr->get_kind() == SymExprKind::Float;
    }
    [[nodiscard]] static bool classof(const Scalar* scalar) {
        return scalar->get_kind() == SymExprKind::Float;
    }

    [[nodiscard]] clang::QualType get_type() const override { return m_type; }
    static void profile(llvm::FoldingSetNodeID& id,
                        const llvm::APFloat& value,
                        clang::QualType type) {
        id.AddInteger(static_cast< unsigned >(SymExprKind::Float));
        id.Add(value);
        id.Add(type);
    }

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        ScalarFloat::profile(id, m_value, m_type);
    }
}; // class Float

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
        return sym_expr->get_kind() >= SymExprKind::SYM_BEGIN &&
               sym_expr->get_kind() <= SymExprKind::SYM_END;
    }
}; // class Sym

/// \brief A symbol representing a region value.
class RegionSymVal : public Sym {
  private:
    /// \brief The region that the symbol represents.
    const TypedRegion* m_region;

    /// \brief The location context where the region is defined.
    const LocationContext* m_loc_ctx;

    /// \brief whether the region val is external or not, which
    ///  means it is not defined in the current analysis context.
    bool m_is_external;

  public:
    RegionSymVal(SymID id,
                 const TypedRegion* region,
                 const LocationContext* loc_ctx,
                 bool is_external);
    ~RegionSymVal() override = default;

    [[gnu::returns_nonnull]] const TypedRegion* get_region() const;
    [[nodiscard]] bool is_external() const { return m_is_external; }

    static void profile(llvm::FoldingSetNodeID& id,
                        [[maybe_unused]] SymID sid,
                        const TypedRegion* region,
                        [[maybe_unused]] const LocationContext* loc_ctx,
                        bool is_external) {
        id.AddInteger(static_cast< unsigned >(SymExprKind::RegionSymbolVal));
        id.AddPointer(region);
        id.AddBoolean(is_external);
    }

    void Profile(llvm::FoldingSetNodeID& profile) const override { // NOLINT
        RegionSymVal::profile(profile,
                              get_id(),
                              m_region,
                              m_loc_ctx,
                              m_is_external);
    }

    [[nodiscard]] llvm::StringRef get_kind_name() const override {
        return "reg_$";
    }

    void dump(llvm::raw_ostream& os) const override;
    [[nodiscard]] std::optional< const MemRegion* > get_as_region()
        const override;
    [[gnu::returns_nonnull, nodiscard]] const LocationContext* get_loc_ctx()
        const {
        return m_loc_ctx;
    }

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
    const StackFrame* m_frame;
    const void* m_tag;

  public:
    SymbolConjured(SymID id,
                   const clang::Stmt* stmt,
                   clang::QualType type,
                   const StackFrame* frame,
                   const void* tag = nullptr)
        : Sym(id, SymExprKind::SymbolConjured),
          m_stmt(stmt),
          m_type(type),
          m_frame(frame),
          m_tag(tag) {
        knight_assert_msg(is_valid_type_for_sym_expr(type), "Invalid type");
    }

    ~SymbolConjured() override = default;

    [[gnu::returns_nonnull]] const clang::Stmt* get_stmt() const {
        return m_stmt;
    }

    [[nodiscard]] clang::QualType get_type() const override { return m_type; }

    [[gnu::returns_nonnull]] const StackFrame* get_frame() const {
        return m_frame;
    }

    [[nodiscard]] const void* get_tag() const { return m_tag; }

    static void profile(llvm::FoldingSetNodeID& id,
                        [[maybe_unused]] SymID sid,
                        const clang::Stmt* stmt,
                        clang::QualType type,
                        const StackFrame* frame,
                        const void* tag = nullptr) {
        id.AddInteger(static_cast< unsigned >(SymExprKind::SymbolConjured));
        id.AddPointer(stmt);
        id.Add(type);
        id.AddPointer(frame);
        id.AddPointer(tag);
    }

    void Profile(llvm::FoldingSetNodeID& profile) const override { // NOLINT
        SymbolConjured::profile(profile,
                                get_id(),
                                m_stmt,
                                m_type,
                                m_frame,
                                m_tag);
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

class BinarySymExpr : public SymExpr {
  private:
    const SymExpr* m_lhs;
    const SymExpr* m_rhs;
    clang::BinaryOperator::Opcode m_opcode;
    clang::QualType m_type;

  public:
    BinarySymExpr(const SymExpr* lhs,
                  const SymExpr* rhs,
                  clang::BinaryOperator::Opcode opcode,
                  clang::QualType type)
        : SymExpr(SymExprKind::BinarySymEx),
          m_lhs(lhs),
          m_rhs(rhs),
          m_opcode(opcode),
          m_type(type) {
        knight_assert_msg(is_valid_type_for_sym_expr(type), "Invalid type");
    }

    ~BinarySymExpr() override = default;

  public:
    [[nodiscard]] const SymExpr* get_lhs() const { return m_lhs; }

    [[nodiscard]] const SymExpr* get_rhs() const { return m_rhs; }

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
        lhs_complexity = m_lhs->get_worst_complexity();
        rhs_complexity = m_rhs->get_worst_complexity();

        switch (m_opcode) {
            using enum clang::BinaryOperatorKind;
            case BO_Add:
            case BO_Sub:
                return std::max(lhs_complexity, rhs_complexity);
            default:
                break;
        }
        lhs_complexity = std::max(lhs_complexity, 1U);
        rhs_complexity = std::max(rhs_complexity, 1U);
        return lhs_complexity * rhs_complexity;
    }

    [[nodiscard]] bool is_leaf() const override { return true; }

    static void profile(llvm::FoldingSetNodeID& id,
                        const SymExpr* lhs,
                        const SymExpr* rhs,
                        clang::BinaryOperator::Opcode opcode,
                        clang::QualType type) {
        id.AddPointer(lhs);
        id.AddPointer(rhs);
        id.AddInteger(static_cast< unsigned >(opcode));
        id.Add(type);
    }

    void Profile(llvm::FoldingSetNodeID& profile) const override { // NOLINT
        BinarySymExpr::profile(profile, m_lhs, m_rhs, m_opcode, m_type);
    }

    void dump(llvm::raw_ostream& os) const override {
        m_lhs->dump(os);
        os << " " << clang::BinaryOperator::getOpcodeStr(m_opcode) << " ";
        m_rhs->dump(os);
    }

    static bool classof(const SymExpr* sym_expr) {
        return sym_expr->get_kind() >= SymExprKind::BinarySymEx;
    }

    static bool classof(const Sym* sym) {
        return sym->get_kind() >= SymExprKind::BinarySymEx;
    }

}; // class BinarySymExpr

} // namespace knight::dfa