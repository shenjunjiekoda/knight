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

#include <llvm/Support/Casting.h>

namespace knight::dfa {

RegionManager& MemRegion::get_manager() const {
    return m_space->get_manager();
}

clang::ASTContext& MemRegion::get_ast_ctx() const {
    return get_manager().get_ast_ctx();
}

bool MemRegion::has_stack_storage() const {
    return llvm::isa_and_nonnull< StackSpaceRegion >(m_space);
}

clang::QualType TypedRegion::get_location_type() const {
    return get_ast_ctx().getPointerType(get_value_type());
}

} // namespace knight::dfa