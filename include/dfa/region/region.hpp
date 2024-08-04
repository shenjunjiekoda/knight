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

#include "dfa/region/regions.hpp"
#include "dfa/stack_frame.hpp"

#include <clang/AST/ASTContext.h>
#include <llvm/ADT/FoldingSet.h>

namespace knight::dfa {

class RegionManager;

class MemSpaceRegion {
  protected:
    enum class SpaceKind : unsigned {
        StackLocal,
        StackArg,
        Code,
        Heap,
        Global,
        Unknown
    } m_kind;

    RegionManager& m_manager;

  public:
    MemSpaceRegion(RegionManager& manager, SpaceKind kind)
        : m_manager(manager), m_kind(kind) {}

  public:
    RegionManager& get_manager() const { return m_manager; }
    void Profile(llvm::FoldingSetNodeID& id) const {
        id.AddInteger(static_cast< unsigned >(m_kind));
    }
}; // class MemSpaceRegion

class StackSpaceRegion : public MemSpaceRegion {
  private:
    const StackFrame* m_frame;

  protected:
    StackSpaceRegion(RegionManager& manager,
                     SpaceKind kind,
                     const StackFrame* frame)
        : MemSpaceRegion(manager, kind), m_frame(frame) {}
    void Profile(llvm::FoldingSetNodeID& id) const {
        MemSpaceRegion::Profile(id);
        id.AddPointer(m_frame);
    }
}; // class StackSpaceRegion

/// \brief Base class for all memory regions.
class MemRegion : public llvm::FoldingSetNode {
  public:
  private:
    const RegionKind m_kind;

  protected:
    explicit MemRegion(RegionKind kind) : m_kind(kind) {}
    virtual ~MemRegion() = default;

    RegionManager& get_manager() const;
}; // class MemRegion

class RegionManager {
  private:
    clang::ASTContext& m_ast_ctx;
    llvm::FoldingSet< MemRegion > m_region_set;
    llvm::BumpPtrAllocator& m_allocator;

  public:
    RegionManager(clang::ASTContext& ast_ctx, llvm::BumpPtrAllocator& allocator)
        : m_ast_ctx(ast_ctx), m_allocator(allocator) {}

  public:
    clang::ASTContext& get_ast_ctx() const { return m_ast_ctx; }
    llvm::BumpPtrAllocator& get_allocator() const { return m_allocator; }

}; // class RegionManager

} // namespace knight::dfa
