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
namespace knight::dfa {

class RegionManager;

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

    [[nodiscard]] static bool classof(const MemSpaceRegion* R) {
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

    [[nodiscard]] static bool classof(const MemSpaceRegion* R) {
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

    [[nodiscard]] static bool classof(const MemSpaceRegion* R) {
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

    [[nodiscard]] static bool classof(const MemSpaceRegion* R) {
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

    [[nodiscard]] static bool classof(const MemSpaceRegion* R) {
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
    [[nodiscard]] static bool classof(const MemSpaceRegion* R) {
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

    [[nodiscard]] static bool classof(const MemSpaceRegion* R) {
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

    [[nodiscard]] static bool classof(const MemSpaceRegion* R) {
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

    [[nodiscard]] static bool classof(const MemSpaceRegion* R) {
        return R->get_kind() == SpaceKind::Unknown;
    }

}; // class UnknownSpaceRegion

/// \brief Base class for all memory regions.
class MemRegion : public llvm::FoldingSetNode {
  protected:
    const RegionKind m_kind;
    const MemSpaceRegion* m_space;
    const MemRegion* m_parent;

  protected:
    MemRegion(RegionKind kind,
              const MemSpaceRegion* space,
              const MemRegion* parent)
        : m_kind(kind), m_space(space), m_parent(parent) {}

  public:
    virtual ~MemRegion() = default;

  public:
    [[nodiscard]] RegionKind get_kind() const { return m_kind; }
    [[nodiscard]] RegionManager& get_manager() const;
    [[nodiscard]] clang::ASTContext& get_ast_ctx() const;
    [[nodiscard]] const MemSpaceRegion* get_memory_space() const {
        return m_space;
    }
    [[nodiscard]] const MemRegion* get_parent() const { return m_parent; }

    [[nodiscard]] bool has_stack_storage() const;

    virtual void dump(llvm::raw_ostream& os) const = 0;

    [[nodiscard]] std::string to_string() const {
        std::string str;
        llvm::raw_string_ostream os(str);
        this->dump(os);
        return os.str();
    }

    virtual void Profile(llvm::FoldingSetNodeID& id) const { // NOLINT
        id.AddInteger(static_cast< unsigned >(m_kind));
        id.AddPointer(m_space);
        id.AddPointer(m_parent);
    }
}; // class MemRegion

class TypedRegion : public MemRegion {
  protected:
    TypedRegion(RegionKind kind,
                const MemSpaceRegion* space,
                const MemRegion* parent)
        : MemRegion(kind, space, parent) {}

  public:
    ~TypedRegion() override = default;

  public:
    [[nodiscard]] virtual clang::QualType get_value_type() const = 0;
    [[nodiscard]] clang::QualType get_location_type() const;

    [[nodiscard]] static bool classof(const MemRegion* R) {
        return R->get_kind() == RegionKind::TypedRegion;
    }
}; // class TypedRegion

class ThisRegion : public TypedRegion {
    friend class RegionManager;

  private:
    const clang::PointerType* m_this_ptr_type;

  private:
    ThisRegion(const clang::PointerType* this_ptr_type,
               StackArgSpaceRegion* arg_space,
               const MemRegion* parent)
        : TypedRegion(RegionKind::ThisObjRegion, arg_space, parent),
          m_this_ptr_type(this_ptr_type) {
        knight_assert_msg(this_ptr_type->getPointeeType()
                                  ->getAsCXXRecordDecl() != nullptr,
                          "invalid this pointer type");
    }

    [[nodiscard]] clang::QualType get_value_type() const override {
        return {m_this_ptr_type, 0};
    }

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        TypedRegion::Profile(id);
        id.AddPointer(m_this_ptr_type);
    }

    [[nodiscard]] static bool classof(const MemRegion* R) {
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
                     MemSpaceRegion* space,
                     const MemRegion* parent)
        : TypedRegion(RegionKind::BaseObjRegion, space, parent),
          m_base_rd_and_is_virtual(base_rd, is_virtual) {
        knight_assert_msg(base_rd != nullptr, "invalid base record decl");
    }

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

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        TypedRegion::Profile(id);
        id.AddPointer(m_base_rd_and_is_virtual.getPointer());
        id.AddBoolean(m_base_rd_and_is_virtual.getInt());
    }

    void dump(llvm::raw_ostream& os) const override {
        os << "base object (";
        get_base_record_decl()->dump(os);
        os << (is_virtual() ? ", virtual" : ", non-virtual");
        os << ") of ";
        m_parent->dump(os);
    }

    [[nodiscard]] static bool classof(const MemRegion* R) {
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
                     MemSpaceRegion* space,
                     const MemRegion* parent)
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

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        TypedRegion::Profile(id);
        id.AddPointer(m_src_expr);
    }

    void dump(llvm::raw_ostream& os) const override {
        os << "temporary object of type " << get_value_type().getAsString()
           << " initialized from ";
        m_src_expr->printPretty(os, nullptr, get_ast_ctx().getPrintingPolicy());
    }

    [[nodiscard]] static bool classof(const MemRegion* R) {
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

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        TypedRegion::Profile(id);
        id.AddPointer(m_str_literal);
    }

    void dump(llvm::raw_ostream& os) const override {
        os << "string literal \"" << m_str_literal->getString() << "\"";
    }

    [[nodiscard]] static bool classof(const MemRegion* R) {
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
  private:
    ElementRegion(const clang::QualType& element_type,
                  MemRegion* base_region,
                  const MemRegion* parent)
        : TypedRegion(RegionKind::ElemRegion,
                      base_region->get_memory_space(),
                      parent),
          m_element_type(element_type) {
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

    void dump(llvm::raw_ostream& os) const override {
        os << "element of type " << m_element_type.getAsString() << " of ";
        m_parent->dump(os);
    }

    [[nodiscard]] static bool classof(const MemRegion* R) {
        return R->get_kind() == RegionKind::ElemRegion;
    }

    [[nodiscard]] static bool classof(const TypedRegion* R) {
        return R->get_kind() == RegionKind::ElemRegion;
    }

}; // class ElementRegion

class DeclRegion : public TypedRegion {
  protected:
    DeclRegion(RegionKind kind,
               const MemSpaceRegion* space,
               const MemRegion* parent)
        : TypedRegion(kind, space, parent) {}

  public:
    ~DeclRegion() override = default;

    [[nodiscard]] virtual const clang::ValueDecl* get_decl() const = 0;

    [[nodiscard]] clang::QualType get_value_type() const override {
        return get_decl()->getType();
    }

    [[nodiscard]] static bool classof(const MemRegion* R) {
        return R->get_kind() == RegionKind::DeclRegion;
    }

    void Profile(llvm::FoldingSetNodeID& id) const override { // NOLINT
        TypedRegion::Profile(id);
        id.AddPointer(get_decl());
    }

}; // class DeclRegion

class VarRegion : public DeclRegion {
    friend class RegionManager;

  private:
    const clang::VarDecl* m_var_decl;

  private:
    VarRegion(const clang::VarDecl* var_decl,
              MemSpaceRegion* space,
              const MemRegion* parent)
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

    void dump(llvm::raw_ostream& os) const override {
        if (const auto* id_info = m_var_decl->getIdentifier()) {
            os << id_info->getName();
        } else {
            os << "unnamed variable" << m_var_decl->getID();
        }
    }

    [[nodiscard]] static bool classof(const MemRegion* R) {
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
    ArgumentRegion(StackArgSpaceRegion* arg_space,
                   const MemRegion* parent,
                   const clang::ParmVarDecl* param_decl = nullptr,
                   clang::Expr* arg_expr = nullptr,
                   unsigned arg_idx = 0U)
        : DeclRegion(RegionKind::ArgRegion, arg_space, parent),
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

    static bool classof(const MemRegion* R) {
        return R->get_kind() == RegionKind::ArgRegion;
    }

    static bool classof(const DeclRegion* R) {
        return R->get_kind() == RegionKind::ArgRegion;
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
                MemRegion* base_region,
                const MemRegion* parent)
        : DeclRegion(RegionKind::FieldRegion,
                     base_region->get_memory_space(),
                     parent),
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

    [[nodiscard]] static bool classof(const MemRegion* R) {
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
    SymbolicRegion(MemSpaceRegion* space, const MemRegion* parent)
        : MemRegion(RegionKind::SymRegion, space, parent) {}

  public:
    ~SymbolicRegion() override = default;

    void dump(llvm::raw_ostream& os) const override { os << "symbolic"; }

    [[nodiscard]] static bool classof(const MemRegion* R) {
        return R->get_kind() == RegionKind::SymRegion;
    }

}; // class SymbolicRegion

class RegionManager {
  private:
    clang::ASTContext& m_ast_ctx;
    llvm::FoldingSet< MemRegion > m_region_set;
    llvm::BumpPtrAllocator& m_allocator;

  public:
    RegionManager(clang::ASTContext& ast_ctx, llvm::BumpPtrAllocator& allocator)
        : m_ast_ctx(ast_ctx), m_allocator(allocator) {}

  public:
    [[nodiscard]] clang::ASTContext& get_ast_ctx() const { return m_ast_ctx; }
    [[nodiscard]] llvm::BumpPtrAllocator& get_allocator() const {
        return m_allocator;
    }

}; // class RegionManager

} // namespace knight::dfa
