//===- region.cpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the memory regions.
//
//===------------------------------------------------------------------===//

#include "dfa/region/region.hpp"

#include <clang/AST/DeclCXX.h>
#include <clang/AST/Type.h>
#include <llvm/Support/Casting.h>
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "dfa/constraint/linear.hpp"

namespace knight::dfa {

namespace {

bool is_valid_base_record(const clang::CXXRecordDecl* base_rd,
                          const TypedRegion* parent,
                          bool is_virtual) {
    base_rd = base_rd->getCanonicalDecl();
    const auto* derived_rd = parent->get_value_type()->getAsCXXRecordDecl();
    knight_assert_msg(derived_rd != nullptr, "invalid derived record");
    derived_rd = derived_rd->getCanonicalDecl();
    if (base_rd == nullptr || derived_rd == nullptr) {
        return false;
    }

    if (is_virtual) {
        return derived_rd->isVirtuallyDerivedFrom(base_rd);
    }

    return llvm::any_of(derived_rd->bases(),
                        [&](const clang::CXXBaseSpecifier& spec) {
                            return spec.getType()
                                       ->getAsCXXRecordDecl()
                                       ->getCanonicalDecl() == base_rd;
                        });
}

} // anonymous namespace

RegionManager& TypedRegion::get_manager() const {
    return m_space->get_manager();
}

clang::ASTContext& TypedRegion::get_ast_ctx() const {
    return get_manager().get_ast_ctx();
}

bool TypedRegion::has_stack_storage() const {
    return llvm::isa_and_nonnull< StackSpaceRegion >(m_space);
}

clang::QualType TypedRegion::get_location_type() const {
    return get_ast_ctx().getPointerType(get_value_type());
}

const StackLocalSpaceRegion* RegionManager::get_stack_local_space_region(
    const StackFrame* frame) {
    const StackLocalSpaceRegion*& region = m_stack_local_space_regions[frame];
    return region = get_persistent_space(region, frame);
}

const StackArgSpaceRegion* RegionManager::get_stack_arg_space_region(
    const StackFrame* frame) {
    const StackArgSpaceRegion*& region = m_stack_arg_space_regions[frame];
    return region = get_persistent_space(region, frame);
}

const CodeSpaceRegion* RegionManager::get_code_space() {
    return get_persistent_space(m_code_space_region);
}

const GlobalInternalSpaceRegion* RegionManager::get_global_internal_space() {
    return get_persistent_space(m_global_internal_space_region);
}

const GlobalExternalSpaceRegion* RegionManager::get_global_external_space() {
    return get_persistent_space(m_global_external_space_region);
}

const HeapSpaceRegion* RegionManager::get_heap_space() {
    return get_persistent_space(m_heap_space_region);
}

const UnknownSpaceRegion* RegionManager::get_unknown_space() {
    return get_persistent_space(m_unknown_space_region);
}

const SymbolicRegion* RegionManager::get_symbolic_region(
    SExprRef sym_val, MemSpaceRegionRef space, RegionRef parent) {
    if (space == nullptr) {
        space = get_unknown_space();
    }
    return get_persistent_region< SymbolicRegion >(sym_val, space, parent);
}

const StringLitRegion* RegionManager::get_string_lit_region(
    const clang::StringLiteral* str, GlobalInternalSpaceRegion* space) {
    return get_persistent_region< StringLitRegion >(str, space);
}

const CXXBaseObjRegion* RegionManager::get_cxx_base_object_region(
    const clang::CXXRecordDecl* base_rd,
    bool is_virtual,
    MemSpaceRegionRef space,
    RegionRef parent) {
    if (const auto* derived_region = llvm::dyn_cast< TypedRegion >(parent)) {
        knight_assert_msg(is_valid_base_record(base_rd,
                                               derived_region,
                                               is_virtual),
                          "invalid base record");
    }
    return get_persistent_region< CXXBaseObjRegion >(base_rd,
                                                     is_virtual,
                                                     space,
                                                     parent);
}

const CXXTempObjRegion* RegionManager::get_cxx_temp_object_region(
    const clang::Expr* src_expr, const StackFrame* frame, RegionRef parent) {
    return get_persistent_region<
        CXXTempObjRegion >(src_expr,
                           get_stack_local_space_region(frame),
                           parent);
}

const ElementRegion* RegionManager::get_element_region(
    clang::QualType element_type, RegionRef base_region, SExprRef idx) {
    element_type = element_type.getCanonicalType().getUnqualifiedType();
    return get_persistent_region< ElementRegion >(element_type,
                                                  base_region,
                                                  idx);
}

const VarRegion* RegionManager::get_var_region(const clang::VarDecl* var_decl,
                                               MemSpaceRegionRef space,
                                               RegionRef parent) {
    var_decl = var_decl->getCanonicalDecl();
    if (const auto* def = var_decl->getDefinition()) {
        var_decl = def;
    }
    return get_persistent_region< VarRegion >(var_decl, space, parent);
}

const FieldRegion* RegionManager::get_field_region(
    const clang::FieldDecl* field_decl,
    MemSpaceRegionRef space,
    RegionRef parent) {
    return get_persistent_region< FieldRegion >(field_decl, space, parent);
}

const ArgumentRegion* RegionManager::get_argument_region(
    const StackFrame* frame,
    const clang::ParmVarDecl* param_decl,
    const clang::Expr* arg_expr,
    unsigned arg_idx) {
    return get_persistent_region< ArgumentRegion >(get_stack_arg_space_region(
                                                       frame),
                                                   param_decl,
                                                   arg_expr,
                                                   arg_idx);
}

const CXXThisRegion* RegionManager::get_cxx_this_region(
    const clang::QualType& this_ptr_type,
    StackArgSpaceRegion* arg_space,
    RegionRef parent) {
    const auto* ptr_ty = this_ptr_type->getAs< clang::PointerType >();
    knight_assert(ptr_ty != nullptr);

    return get_persistent_region< CXXThisRegion >(ptr_ty, arg_space, parent);
}

RegionRef RegionManager::get_region(const clang::VarDecl* var_decl,
                                    const StackFrame* frame) {
    // TODO(var-region): impl
    knight_assert(var_decl != nullptr);
    var_decl = var_decl->getCanonicalDecl();
    if (var_decl == nullptr) {
        return nullptr;
    }

    if (const auto* param_decl = dyn_cast< clang::ParmVarDecl >(var_decl)) {
        if (frame->is_top_frame()) {
            return get_top_level_stack_argument_region(frame, param_decl);
        }
        const auto* current_decl = frame->get_decl();
        const auto* current_arg_expr =
            llvm::dyn_cast< clang::Expr >(frame->get_callsite_expr());
        knight_assert_msg(current_decl != nullptr, "invalid frame");
        knight_assert_msg(current_arg_expr != nullptr, "invalid frame");

        if (const auto* current_function_decl =
                llvm::dyn_cast< clang::FunctionDecl >(current_decl)) {
            return get_argument_region(frame,
                                       param_decl,
                                       current_arg_expr,
                                       current_function_decl->getNumParams() -
                                           1);
        }
        knight_unreachable("unhandled frame decl type");
    }
    MemSpaceRegionRef space = nullptr;
    if (var_decl->hasGlobalStorage()) {
        if (var_decl->hasExternalStorage()) {
            space = get_global_internal_space();
        } else {
            space = get_global_external_space();
        }
    } else if (var_decl->hasLocalStorage()) {
        space = get_stack_local_space_region(frame);
    } else {
        knight_unreachable("unhandled storage class");
    }

    return get_var_region(var_decl, /*space=*/space, /*parent=*/nullptr);
}

} // namespace knight::dfa