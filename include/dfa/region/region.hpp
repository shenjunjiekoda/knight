//===- region.hpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the memory regions.
//
//===------------------------------------------------------------------===//

#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>

#include <llvm/ADT/FoldingSet.h>
#include <llvm/ADT/PointerIntPair.h>
#include <llvm/Support/raw_ostream.h>

#include "dfa/region/regions.hpp"
#include "dfa/stack_frame.hpp"
#include "dfa/symbol.hpp"
#include "tooling/diagnostic.hpp"

namespace knight::dfa {

class RegionManager;
class MemRegion;
class MemSpaceRegion;

using MemRegionRef = const MemRegion*;
using MemSpaceRegionRef = const MemSpaceRegion*;

class MemSpaceRegion {
  public:
    enum class SpaceKind : unsigned {
        StackLocal,
        StackArg,
        Code,
        Heap,
        GlobalInternal,
        GlobalExternal,
        Unknown
    };

  protected:
    SpaceKind m_kind;
    RegionManager& m_manager;

  public:
    MemSpaceRegion(RegionManager& manager, SpaceKind kind)
        : m_manager(manager), m_kind(kind) {}
    virtual ~MemSpaceRegion() = default;

  public:
    [[nodiscard]] SpaceKind get_kind() const { return m_kind; }
    [[nodiscard]] RegionManager& get_manager() const { return m_manager; }
    virtual void Profile(llvm::FoldingSetNodeID& id) const { // NOLINT
        id.AddInteger(static_cast< unsigned >(m_kind));
    }
    virtual void dump(llvm::raw_ostream& os) const = 0;
    [[nodiscard]] bool is_stack_local() const {
        return m_kind == SpaceKind::StackLocal;
    }

}; // class MemSpaceRegion

class StackSpaceRegion : public MemSpaceRegion {
  protected:
    const StackFrame* m_frame;

  protected:
    StackSpaceRegion(RegionManager& manager,
                     SpaceKind kind,
                     const StackFrame* frame)
        : MemSpaceRegion(manager, kind), m_frame(frame) {}

  public:
    ~StackSpaceRegion() override = default;
    virtual void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        MemSpaceRegion::Profile(id);
        id.AddPointer(m_frame);
    }

    [[nodiscard]] static bool classof(MemSpaceRegionRef R) {
        return R->get_kind() == SpaceKind::StackArg ||
               R->get_kind() == SpaceKind::StackLocal;
    }

}; // class StackSpaceRegion

class StackLocalSpaceRegion : public StackSpaceRegion {
    friend class RegionManager;

  private:
    StackLocalSpaceRegion(RegionManager& manager, const StackFrame* frame)
        : StackSpaceRegion(manager, SpaceKind::StackLocal, frame) {}

  public:
    ~StackLocalSpaceRegion() override = default;

  public:
    void dump(llvm::raw_ostream& os) const override {
        os << "StackLocalSpaceRegion";
        this->m_frame->dump(os);
    }

    [[nodiscard]] static bool classof(MemSpaceRegionRef R) {
        return R->get_kind() == SpaceKind::StackLocal;
    }

    [[nodiscard]] static bool classof(const StackSpaceRegion* R) {
        return R->get_kind() == SpaceKind::StackLocal;
    }
}; // class StackLocalSpaceRegion

class StackArgSpaceRegion : public StackSpaceRegion {
    friend class RegionManager;

  private:
    StackArgSpaceRegion(RegionManager& manager, const StackFrame* frame)
        : StackSpaceRegion(manager, SpaceKind::StackArg, frame) {}

  public:
    ~StackArgSpaceRegion() override = default;

  public:
    void dump(llvm::raw_ostream& os) const override {
        os << "StackArgSpaceRegion";
        this->m_frame->dump(os);
    }

    [[nodiscard]] static bool classof(MemSpaceRegionRef R) {
        return R->get_kind() == SpaceKind::StackArg;
    }

    [[nodiscard]] static bool classof(const StackSpaceRegion* R) {
        return R->get_kind() == SpaceKind::StackArg;
    }
}; // class StackArgSpaceRegion

class CodeSpaceRegion : public MemSpaceRegion {
    friend class RegionManager;

  private:
    explicit CodeSpaceRegion(RegionManager& manager)
        : MemSpaceRegion(manager, SpaceKind::Code) {}

  public:
    ~CodeSpaceRegion() override = default;

  public:
    void dump(llvm::raw_ostream& os) const override { os << "CodeSpaceRegion"; }

    [[nodiscard]] static bool classof(MemSpaceRegionRef R) {
        return R->get_kind() == MemSpaceRegion::SpaceKind::Code;
    }

}; // class CodeSpaceRegion

class HeapSpaceRegion : public MemSpaceRegion {
    friend class RegionManager;

  private:
    explicit HeapSpaceRegion(RegionManager& manager)
        : MemSpaceRegion(manager, SpaceKind::Heap) {}

  public:
    ~HeapSpaceRegion() override = default;

  public:
    void dump(llvm::raw_ostream& os) const override { os << "HeapSpaceRegion"; }

    [[nodiscard]] static bool classof(MemSpaceRegionRef R) {
        return R->get_kind() == MemSpaceRegion::SpaceKind::Heap;
    }

}; // class HeapSpaceRegion

class GlobalSpaceRegion : public MemSpaceRegion {
    friend class RegionManager;

  protected:
    explicit GlobalSpaceRegion(RegionManager& manager, SpaceKind kind)
        : MemSpaceRegion(manager, kind) {}

  public:
    ~GlobalSpaceRegion() override = default;

  public:
    [[nodiscard]] static bool classof(MemSpaceRegionRef R) {
        return R->get_kind() == SpaceKind::GlobalExternal ||
               R->get_kind() == SpaceKind::GlobalInternal;
    }
}; // class GlobalSpaceRegion

class GlobalInternalSpaceRegion : public GlobalSpaceRegion {
    friend class RegionManager;

  private:
    explicit GlobalInternalSpaceRegion(RegionManager& manager)
        : GlobalSpaceRegion(manager, SpaceKind::GlobalInternal) {}

  public:
    ~GlobalInternalSpaceRegion() override = default;

  public:
    void dump(llvm::raw_ostream& os) const override {
        os << "GlobalInternalSpaceRegion";
    }

    [[nodiscard]] static bool classof(MemSpaceRegionRef R) {
        return R->get_kind() == SpaceKind::GlobalInternal;
    }

    [[nodiscard]] static bool classof(const GlobalSpaceRegion* R) {
        return R->get_kind() == SpaceKind::GlobalInternal;
    }

}; // class GlobalInternalSpaceRegion

class GlobalExternalSpaceRegion : public GlobalSpaceRegion {
    friend class RegionManager;

  private:
    explicit GlobalExternalSpaceRegion(RegionManager& manager)
        : GlobalSpaceRegion(manager, SpaceKind::GlobalExternal) {}

  public:
    ~GlobalExternalSpaceRegion() override = default;

  public:
    void dump(llvm::raw_ostream& os) const override {
        os << "GlobalExternalSpaceRegion";
    }

    [[nodiscard]] static bool classof(MemSpaceRegionRef R) {
        return R->get_kind() == SpaceKind::GlobalExternal;
    }

    [[nodiscard]] static bool classof(const GlobalSpaceRegion* R) {
        return R->get_kind() == SpaceKind::GlobalExternal;
    }

}; // class GlobalExternalSpaceRegion

class UnknownSpaceRegion : public MemSpaceRegion {
    friend class RegionManager;

  private:
    explicit UnknownSpaceRegion(RegionManager& manager)
        : MemSpaceRegion(manager, SpaceKind::Unknown) {}

  public:
    ~UnknownSpaceRegion() override = default;

  public:
    void dump(llvm::raw_ostream& os) const override {
        os << "UnknownSpaceRegion";
    }

    [[nodiscard]] static bool classof(MemSpaceRegionRef R) {
        return R->get_kind() == SpaceKind::Unknown;
    }

}; // class UnknownSpaceRegion

/// \brief Base class for all memory regions.
class MemRegion : public llvm::FoldingSetNode {
  protected:
    const RegionKind m_kind;
    MemSpaceRegionRef m_space;
    MemRegionRef m_parent;

  protected:
    MemRegion(RegionKind kind, MemSpaceRegionRef space, MemRegionRef parent)
        : m_kind(kind), m_space(space), m_parent(parent) {}

  public:
    virtual ~MemRegion() = default;

  public:
    [[nodiscard]] RegionKind get_kind() const { return m_kind; }
    [[nodiscard]] RegionManager& get_manager() const;
    [[nodiscard]] clang::ASTContext& get_ast_ctx() const;
    [[nodiscard]] MemSpaceRegionRef get_memory_space() const { return m_space; }
    [[nodiscard]] MemRegionRef get_parent() const { return m_parent; }

    [[nodiscard]] bool has_stack_storage() const;

    virtual void dump(llvm::raw_ostream& os) const = 0;

    [[nodiscard]] std::string to_string() const {
        std::string str;
        llvm::raw_string_ostream os(str);
        this->dump(os);
        return os.str();
    }

    virtual void Profile(llvm::FoldingSetNodeID& id) const { // NOLINT
        profile(id, m_kind, m_space, m_parent);
    }

    static void profile(llvm::FoldingSetNodeID& id,
                        RegionKind kind,
                        MemSpaceRegionRef space,
                        MemRegionRef parent) {
        id.AddInteger(static_cast< unsigned >(kind));
        id.AddPointer(space);
        id.AddPointer(parent);
    }
}; // class MemRegion

class TypedRegion : public MemRegion {
  protected:
    TypedRegion(RegionKind kind, MemSpaceRegionRef space, MemRegionRef parent)
        : MemRegion(kind, space, parent) {}

  public:
    ~TypedRegion() override = default;

  public:
    [[nodiscard]] virtual clang::QualType get_value_type() const = 0;
    [[nodiscard]] clang::QualType get_location_type() const;

    [[nodiscard]] static bool classof(MemRegionRef R) {
        return R->get_kind() == RegionKind::TypedRegion;
    }

    static void profile(llvm::FoldingSetNodeID& id,
                        RegionKind kind,
                        MemSpaceRegionRef space,
                        MemRegionRef parent) {
        MemRegion::profile(id, kind, space, parent);
    }
}; // class TypedRegion

class CXXThisRegion : public TypedRegion {
    friend class RegionManager;

  private:
    const clang::PointerType* m_this_ptr_type;

  private:
    CXXThisRegion(const clang::PointerType* this_ptr_type,
                  const StackArgSpaceRegion* arg_space,
                  MemRegionRef parent)
        : TypedRegion(RegionKind::ThisObjRegion, arg_space, parent),
          m_this_ptr_type(this_ptr_type) {
        knight_assert_msg(this_ptr_type->getPointeeType()
                                  ->getAsCXXRecordDecl() != nullptr,
                          "invalid this pointer type");
    }

  public:
    [[nodiscard]] clang::QualType get_value_type() const override {
        return {m_this_ptr_type, 0};
    }

    static void profile(llvm::FoldingSetNodeID& id,
                        const clang::PointerType* this_ptr_type,
                        const StackArgSpaceRegion* arg_space,
                        MemRegionRef parent) {
        TypedRegion::profile(id, RegionKind::ThisObjRegion, arg_space, parent);
        id.AddPointer(this_ptr_type);
    }

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        profile(id,
                m_this_ptr_type,
                llvm::cast< StackArgSpaceRegion >(m_space),
                m_parent);
    }

    [[nodiscard]] static bool classof(MemRegionRef R) {
        return R->get_kind() == RegionKind::ThisObjRegion;
    }

    [[nodiscard]] static bool classof(const TypedRegion* R) {
        return R->get_kind() == RegionKind::ThisObjRegion;
    }

    void dump(llvm::raw_ostream& os) const override { os << "this"; }

}; // class ThisRegion

class CXXBaseObjRegion : public TypedRegion {
    friend class RegionManager;

  private:
    llvm::PointerIntPair< const clang::CXXRecordDecl*, 1, bool >
        m_base_rd_and_is_virtual;

  private:
    CXXBaseObjRegion(const clang::CXXRecordDecl* base_rd,
                     bool is_virtual,
                     MemSpaceRegionRef space,
                     MemRegionRef parent)
        : TypedRegion(RegionKind::BaseObjRegion, space, parent),
          m_base_rd_and_is_virtual(base_rd, is_virtual) {
        knight_assert_msg(base_rd != nullptr, "invalid base record decl");
    }

  public:
    [[gnu::returns_nonnull, nodiscard]] const clang::CXXRecordDecl*
    get_base_record_decl() const {
        return m_base_rd_and_is_virtual.getPointer();
    }

    [[nodiscard]] bool is_virtual() const {
        return m_base_rd_and_is_virtual.getInt();
    }

    [[nodiscard]] clang::QualType get_value_type() const override {
        return {get_base_record_decl()->getTypeForDecl(), 0};
    }

    static void profile(llvm::FoldingSetNodeID& id,
                        const clang::CXXRecordDecl* base_rd,
                        bool is_virtual,
                        MemSpaceRegionRef space,
                        MemRegionRef parent) {
        TypedRegion::profile(id, RegionKind::BaseObjRegion, space, parent);
        id.AddPointer(base_rd);
        id.AddBoolean(is_virtual);
    }

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        profile(id, get_base_record_decl(), is_virtual(), m_space, m_parent);
    }

    void dump(llvm::raw_ostream& os) const override {
        os << "base object (";
        get_base_record_decl()->dump(os);
        os << (is_virtual() ? ", virtual" : ", non-virtual");
        os << ") of ";
        m_parent->dump(os);
    }

    [[nodiscard]] static bool classof(MemRegionRef R) {
        return R->get_kind() == RegionKind::BaseObjRegion;
    }

    [[nodiscard]] static bool classof(const TypedRegion* R) {
        return R->get_kind() == RegionKind::BaseObjRegion;
    }

}; // class BaseObjRegion

class CXXTempObjRegion : public TypedRegion {
    friend class RegionManager;

  private:
    const clang::Expr* m_src_expr;

  private:
    CXXTempObjRegion(const clang::Expr* src_expr,
                     const StackLocalSpaceRegion* space,
                     MemRegionRef parent)
        : TypedRegion(RegionKind::TempObjRegion, space, parent),
          m_src_expr(src_expr) {
        knight_assert_msg(src_expr != nullptr, "invalid source expression");
    }

  public:
    [[gnu::returns_nonnull, nodiscard]] const clang::Expr* get_source_expr()
        const {
        return m_src_expr;
    }

    [[nodiscard]] clang::QualType get_value_type() const override {
        return m_src_expr->getType();
    }

    static void profile(llvm::FoldingSetNodeID& id,
                        const clang::Expr* src_expr,
                        MemSpaceRegionRef space,
                        MemRegionRef parent) {
        TypedRegion::profile(id, RegionKind::TempObjRegion, space, parent);
        id.AddPointer(src_expr);
    }

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        profile(id, m_src_expr, m_space, m_parent);
    }

    void dump(llvm::raw_ostream& os) const override {
        os << "temporary object of type " << get_value_type().getAsString()
           << " initialized from ";
        m_src_expr->printPretty(os, nullptr, get_ast_ctx().getPrintingPolicy());
    }

    [[nodiscard]] static bool classof(MemRegionRef R) {
        return R->get_kind() == RegionKind::TempObjRegion;
    }

    [[nodiscard]] static bool classof(const TypedRegion* R) {
        return R->get_kind() == RegionKind::TempObjRegion;
    }

}; // class CXXTempObjRegion

class StringLitRegion : public TypedRegion {
    friend class RegionManager;

  private:
    const clang::StringLiteral* m_str_literal;

  private:
    StringLitRegion(const clang::StringLiteral* str_literal,
                    GlobalInternalSpaceRegion* space)
        : TypedRegion(RegionKind::StringLitRegion, space, nullptr),
          m_str_literal(str_literal) {
        knight_assert_msg(str_literal != nullptr, "invalid string literal");
    }

  public:
    ~StringLitRegion() override = default;
    [[gnu::returns_nonnull, nodiscard]] const clang::StringLiteral*
    get_string_literal() const {
        return m_str_literal;
    }

    [[nodiscard]] clang::QualType get_value_type() const override {
        return m_str_literal->getType();
    }

    static void profile(llvm::FoldingSetNodeID& id,
                        const clang::StringLiteral* str_literal,
                        MemSpaceRegionRef space) {
        TypedRegion::profile(id, RegionKind::StringLitRegion, space, nullptr);
        id.AddPointer(str_literal);
    }

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        profile(id, m_str_literal, m_space);
    }

    void dump(llvm::raw_ostream& os) const override {
        os << "string literal \"" << m_str_literal->getString() << "\"";
    }

    [[nodiscard]] static bool classof(MemRegionRef R) {
        return R->get_kind() == RegionKind::StringLitRegion;
    }

    [[nodiscard]] static bool classof(const TypedRegion* R) {
        return R->get_kind() == RegionKind::StringLitRegion;
    }
}; // class StringLitRegion

class ElementRegion : public TypedRegion {
    friend class RegionManager;

  private:
    clang::QualType m_element_type;
    // TODO(sym): add symbolic index value?
    SExprRef m_idx;

  private:
    ElementRegion(const clang::QualType& element_type,
                  MemRegionRef base_region,
                  SExprRef idx)
        : TypedRegion(RegionKind::ElemRegion,
                      base_region->get_memory_space(),
                      base_region),
          m_element_type(element_type),
          m_idx(idx) {
        knight_assert_msg(!element_type.isNull(), "invalid element type");
    }

  public:
    ~ElementRegion() override = default;

    [[nodiscard]] clang::QualType get_value_type() const override {
        return m_element_type;
    }

    [[nodiscard]] clang::QualType get_element_type() const {
        return m_element_type;
    }

    [[nodiscard]] SExprRef get_index() const { return m_idx; }

    static void profile(llvm::FoldingSetNodeID& id,
                        const clang::QualType& element_type,
                        MemRegionRef base_region,
                        SExprRef idx) {
        TypedRegion::profile(id,
                             RegionKind::ElemRegion,
                             base_region->get_memory_space(),
                             base_region);
        id.Add(element_type);
        id.AddPointer(idx);
    }

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        profile(id, m_element_type, m_parent, m_idx);
    }

    void dump(llvm::raw_ostream& os) const override {
        os << "element of type " << m_element_type.getAsString() << " of ";
        m_parent->dump(os);
    }

    [[nodiscard]] static bool classof(MemRegionRef R) {
        return R->get_kind() == RegionKind::ElemRegion;
    }

    [[nodiscard]] static bool classof(const TypedRegion* R) {
        return R->get_kind() == RegionKind::ElemRegion;
    }

}; // class ElementRegion

class DeclRegion : public TypedRegion {
  protected:
    DeclRegion(RegionKind kind, MemSpaceRegionRef space, MemRegionRef parent)
        : TypedRegion(kind, space, parent) {}

  public:
    ~DeclRegion() override = default;

    [[nodiscard]] virtual const clang::ValueDecl* get_decl() const = 0;

    [[nodiscard]] clang::QualType get_value_type() const override {
        return get_decl()->getType();
    }

    [[nodiscard]] static bool classof(MemRegionRef R) {
        return R->get_kind() == RegionKind::DeclRegion;
    }

    static void profile(llvm::FoldingSetNodeID& id,
                        const clang::ValueDecl* decl,
                        RegionKind kind,
                        MemSpaceRegionRef space,
                        MemRegionRef parent) {
        TypedRegion::profile(id, kind, space, parent);
        id.AddPointer(decl);
    }

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        profile(id, get_decl(), m_kind, m_space, m_parent);
    }

}; // class DeclRegion

class VarRegion : public DeclRegion {
    friend class RegionManager;

  private:
    const clang::VarDecl* m_var_decl;

  private:
    VarRegion(const clang::VarDecl* var_decl,
              MemSpaceRegionRef space,
              MemRegionRef parent)
        : DeclRegion(RegionKind::VarRegion, space, parent),
          m_var_decl(var_decl) {
        knight_assert_msg(var_decl != nullptr, "invalid variable declaration");
    }

  public:
    ~VarRegion() override = default;

    [[gnu::returns_nonnull, nodiscard]] const clang::VarDecl* get_var_decl()
        const {
        return m_var_decl;
    }

    [[gnu::returns_nonnull, nodiscard]] const clang::ValueDecl* get_decl()
        const override {
        return m_var_decl;
    }

    static void profile(llvm::FoldingSetNodeID& id,
                        const clang::VarDecl* var_decl,
                        MemSpaceRegionRef space,
                        MemRegionRef parent) {
        DeclRegion::profile(id, var_decl, RegionKind::VarRegion, space, parent);
    }

    void dump(llvm::raw_ostream& os) const override {
        if (const auto* id_info = m_var_decl->getIdentifier()) {
            os << id_info->getName();
        } else {
            os << "unnamed variable" << m_var_decl->getID();
        }
    }

    [[nodiscard]] static bool classof(MemRegionRef R) {
        return R->get_kind() == RegionKind::VarRegion;
    }

    [[nodiscard]] static bool classof(const DeclRegion* R) {
        return R->get_kind() == RegionKind::VarRegion;
    }

}; // class VarRegion

class ArgumentRegion : public DeclRegion {
    friend class RegionManager;

  private:
    const clang::ParmVarDecl* m_param_decl;
    const clang::Expr* m_arg_expr;
    unsigned m_arg_idx;

  private:
    explicit ArgumentRegion(const StackArgSpaceRegion* arg_space,
                            const clang::ParmVarDecl* param_decl = nullptr,
                            const clang::Expr* arg_expr = nullptr,
                            unsigned arg_idx = 0U)
        : DeclRegion(RegionKind::ArgRegion, arg_space, nullptr),
          m_param_decl(param_decl),
          m_arg_expr(arg_expr),
          m_arg_idx(arg_idx) {
        knight_assert_msg(arg_expr != nullptr || param_decl != nullptr,
                          "argument expression and parameter declaration shall "
                          "not be both null");
    }

  public:
    ~ArgumentRegion() override = default;
    [[nodiscard]] const clang::ParmVarDecl* get_param_decl() const {
        return m_param_decl; // is null when used for a variable arg
    }

    [[nodiscard]] const clang::ValueDecl* get_decl() const override {
        return m_param_decl;
    }

    [[nodiscard]] clang::QualType get_value_type() const override {
        if (m_param_decl != nullptr) {
            return m_param_decl->getType();
        }
        return get_arg_expr()->getType();
    }

    [[nodiscard]] const clang::Expr* get_arg_expr() const {
        return m_arg_expr; // is null when in top level stack
    }

    [[nodiscard]] unsigned get_arg_index() const { return m_arg_idx; }

    void set_arg_expr([[gnu::nonnull]] clang::Expr* arg_expr) {
        m_arg_expr = arg_expr;
    }

    void set_arg_index(unsigned arg_idx) { m_arg_idx = arg_idx; }

    static bool classof(MemRegionRef R) {
        return R->get_kind() == RegionKind::ArgRegion;
    }

    static bool classof(const DeclRegion* R) {
        return R->get_kind() == RegionKind::ArgRegion;
    }

    static void profile(llvm::FoldingSetNodeID& id,
                        const StackArgSpaceRegion* arg_space,
                        const clang::ParmVarDecl* param_decl,
                        const clang::Expr* arg_expr,
                        unsigned arg_idx) {
        DeclRegion::profile(id,
                            param_decl,
                            RegionKind::ArgRegion,
                            arg_space,
                            nullptr);
        id.AddPointer(arg_expr);
        id.AddInteger(arg_idx);
    }

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        profile(id,
                llvm::cast< StackArgSpaceRegion >(m_space),
                m_param_decl,
                m_arg_expr,
                m_arg_idx);
    }

    void dump(llvm::raw_ostream& os) const override {
        if (m_arg_expr != nullptr) {
            m_arg_expr->printPretty(os,
                                    nullptr,
                                    get_ast_ctx().getPrintingPolicy());

            os << " -> ";
        } else {
            os << "top-level stack argument: ";
        }

        if (m_param_decl != nullptr) {
            if (const auto* id_info = m_param_decl->getIdentifier()) {
                os << id_info->getName();
            } else {
                os << "unnamed parameter" << m_param_decl->getID();
            }
        } else {
            os << "variable argument";
        }
    }

}; // class ArgumentRegion

class FieldRegion : public DeclRegion {
    friend class RegionManager;

  private:
    const clang::FieldDecl* m_field_decl;

  private:
    FieldRegion(const clang::FieldDecl* field_decl,
                MemSpaceRegionRef space,
                MemRegionRef parent)
        : DeclRegion(RegionKind::FieldRegion, space, parent),
          m_field_decl(field_decl) {
        knight_assert_msg(field_decl != nullptr, "invalid field declaration");
    }

  public:
    ~FieldRegion() override = default;

    [[gnu::returns_nonnull, nodiscard]] const clang::FieldDecl* get_field_decl()
        const {
        return m_field_decl;
    }

    [[gnu::returns_nonnull, nodiscard]] const clang::ValueDecl* get_decl()
        const override {
        return m_field_decl;
    }

    void dump(llvm::raw_ostream& os) const override {
        if (const auto* id_info = m_field_decl->getIdentifier()) {
            os << id_info->getName();
        } else {
            os << "unnamed field" << m_field_decl->getID();
        }
    }

    static void profile(llvm::FoldingSetNodeID& id,
                        const clang::FieldDecl* field_decl,
                        MemSpaceRegionRef space,
                        MemRegionRef parent) {
        DeclRegion::profile(id,
                            field_decl,
                            RegionKind::FieldRegion,
                            space,
                            parent);
    }

    [[nodiscard]] static bool classof(MemRegionRef R) {
        return R->get_kind() == RegionKind::FieldRegion;
    }

    [[nodiscard]] static bool classof(const DeclRegion* R) {
        return R->get_kind() == RegionKind::FieldRegion;
    }

}; // class FieldRegion

/// \brief Symbolic region can be used in mapping symbolic values.
/// It is not a real region in the program, but a placeholder.
/// It is not required to be typed.
class SymbolicRegion : public MemRegion {
    friend class RegionManager;

  private:
    // TODO(sym): add symbolic value
  public:
    SymbolicRegion(MemSpaceRegionRef space, MemRegionRef parent)
        : MemRegion(RegionKind::SymRegion, space, parent) {}

  public:
    ~SymbolicRegion() override = default;

    void dump(llvm::raw_ostream& os) const override { os << "symbolic"; }

    [[nodiscard]] static bool classof(MemRegionRef R) {
        return R->get_kind() == RegionKind::SymRegion;
    }

    static void profile(llvm::FoldingSetNodeID& id,
                        MemSpaceRegionRef space,
                        MemRegionRef parent) {
        MemRegion::profile(id, RegionKind::SymRegion, space, parent);
    }

}; // class SymbolicRegion

class RegionManager {
  private:
    clang::ASTContext* m_ast_ctx;

    llvm::BumpPtrAllocator& m_allocator;
    llvm::FoldingSet< MemRegion > m_region_set;

    const CodeSpaceRegion* m_code_space_region{};
    const GlobalInternalSpaceRegion* m_global_internal_space_region{};
    const GlobalExternalSpaceRegion* m_global_external_space_region{};
    const HeapSpaceRegion* m_heap_space_region{};
    const UnknownSpaceRegion* m_unknown_space_region{};

    std::unordered_map< const StackFrame*, const StackLocalSpaceRegion* >
        m_stack_local_space_regions;

    std::unordered_map< const StackFrame*, const StackArgSpaceRegion* >
        m_stack_arg_space_regions;

  public:
    explicit RegionManager(llvm::BumpPtrAllocator& allocator)

        : m_allocator(allocator) {}

  public:
    void set_ast_ctx(clang::ASTContext& ast_ctx) { m_ast_ctx = &ast_ctx; }

    [[nodiscard]] clang::ASTContext& get_ast_ctx() const {
        knight_assert_msg(m_ast_ctx != nullptr, "ast context is not set");
        return *m_ast_ctx;
    }

    [[nodiscard]] llvm::BumpPtrAllocator& get_allocator() const {
        return m_allocator;
    }

    /// \brief Get a memory space region
    const StackLocalSpaceRegion* get_stack_local_space_region(
        const StackFrame* frame);
    const StackArgSpaceRegion* get_stack_arg_space_region(
        const StackFrame* frame);
    const CodeSpaceRegion* get_code_space();
    const GlobalInternalSpaceRegion* get_global_internal_space();
    const GlobalExternalSpaceRegion* get_global_external_space();
    const HeapSpaceRegion* get_heap_space();
    const UnknownSpaceRegion* get_unknown_space();

    /// \brief Get a memory region
    const SymbolicRegion* get_symbolic_region(MemSpaceRegionRef space,
                                              MemRegionRef parent);
    const StringLitRegion* get_string_lit_region(
        const clang::StringLiteral* str, GlobalInternalSpaceRegion* space);
    const CXXBaseObjRegion* get_cxx_base_object_region(
        const clang::CXXRecordDecl* base_rd,
        bool is_virtual,
        MemSpaceRegionRef space,
        MemRegionRef parent);
    const CXXTempObjRegion* get_cxx_temp_object_region(
        const clang::Expr* src_expr,
        const StackFrame* frame,
        MemRegionRef parent = nullptr);
    const ElementRegion* get_element_region(clang::QualType element_type,
                                            MemRegionRef base_region,
                                            SExprRef idx);
    const VarRegion* get_var_region(const clang::VarDecl* var_decl,
                                    MemSpaceRegionRef space,
                                    MemRegionRef parent);
    const FieldRegion* get_field_region(const clang::FieldDecl* field_decl,
                                        MemSpaceRegionRef space,
                                        MemRegionRef parent);

    const ArgumentRegion* get_top_level_stack_argument_region(
        const StackFrame* frame, const clang::ParmVarDecl* param_decl) {
        return get_argument_region(frame,
                                   param_decl,
                                   nullptr,
                                   param_decl->getFunctionScopeIndex());
    }

    const ArgumentRegion* get_argument_region(
        const StackFrame* frame,
        const clang::ParmVarDecl* param_decl = nullptr,
        const clang::Expr* arg_expr = nullptr,
        unsigned arg_idx = 0U);
    const CXXThisRegion* get_cxx_this_region(
        const clang::QualType& this_ptr_type,
        StackArgSpaceRegion* arg_space,
        MemRegionRef parent);

  public:
    const MemRegion* get_region(const clang::VarDecl* var_decl,
                                const StackFrame* frame);

  private:
    template < typename Space, typename... Args >
    const Space* get_persistent_space(Space*& region, Args&&... args) {
        if (region == nullptr) {
            region = new (m_allocator) // NOLINT
                Space(*this, std::forward< Args >(args)...);
        }

        return region;
    }

    template < typename Region, typename... Args >
    const Region* get_persistent_region(Args&&... args) {
        llvm::FoldingSetNodeID id;
        Region::profile(id, std::forward< Args >(args)...);

        void* insert_pos; // NOLINT
        auto* region = cast_or_null< Region >(
            m_region_set.FindNodeOrInsertPos(id, insert_pos));
        if (region == nullptr) {
            region = new (m_allocator) // NOLINT
                Region(std::forward< Args >(args)...);
            m_region_set.InsertNode(region, insert_pos);
        }
        return region;
    }

}; // class RegionManager

} // namespace knight::dfa
