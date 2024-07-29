//===- program_state.hpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the program state class.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/analysis_manager.hpp"
#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"

#include <optional>
#include <unordered_map>
#include <unordered_set>

#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/ImmutableMap.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>

namespace knight::dfa {

class ProgramState;
class ProgramStateManager;

using ProgramStateRawPtr = const ProgramState*;
using ProgramStateRef = llvm::IntrusiveRefCntPtr< const ProgramState >;

void retain_state(ProgramStateRawPtr state);
void release_state(ProgramStateRawPtr state);

class ProgramState : public llvm::FoldingSetNode {
    friend class ProgramStateManager;

  public:
    using DomValMap = llvm::DenseMap< DomID, UniqueVal >;
    using ValRefSet = std::unordered_set< AbsValRef >;

  private:
    unsigned m_ref_cnt;
    DomValMap m_dom_val;
    ProgramStateManager* m_mgr;

  public:
    ProgramState(ProgramStateManager* mgr, DomValMap dom_val);

    /// \breif Copy constructor.
    ProgramState(const ProgramState& other);

    /// \brief Copy assignment is not allowed.
    void operator=(const ProgramState& other) = delete;

  private:
    friend void retain_state(ProgramStateRawPtr state);
    friend void release_state(ProgramStateRawPtr state);

  public:
    /// \brief Return the state manager.
    ProgramStateManager& get_manager() const;

    /// \brief Check if the given domain kind exists in the program state.
    ///
    /// \return True if the domain kind exists, false otherwise.
    template < typename Domain > bool exists() const;

    /// \brief Get the domain with the given kind.
    ///
    /// \return The domain with the given kind, or std::nullopt if it doesn't
    /// exist.
    template < typename Domain > std::optional< Domain* > get() const;

    /// \brief Remove the given domain from the program state.
    template < typename Domain > bool remove();

    /// \brief Set the given domain to the given abstract val.
    template < typename Domain > void set(UniqueVal val);

  public:
    ProgramStateRef clone() const;

    void normalize();

    bool is_bottom() const;
    bool is_top() const;
    void set_to_bottom();
    void set_to_top();

    ProgramStateRef join(ProgramStateRef other);
    ProgramStateRef join_at_loop_head(ProgramStateRef other);
    ProgramStateRef join_consecutive_iter(ProgramStateRef other);

    ProgramStateRef widen(ProgramStateRef other);
    ProgramStateRef meet(ProgramStateRef other);
    ProgramStateRef narrow(ProgramStateRef other);

    bool leq(const ProgramState& other) const;
    bool equals(const ProgramState& other) const;

    bool operator==(const ProgramState& other) const { return equals(other); }
    bool operator!=(const ProgramState& other) const { return !equals(other); }

    void dump(llvm::raw_ostream& os) const;

  public:
    /// \brief Profile the contents of a ProgramState object for use in a
    ///  FoldingSet.  Two ProgramState objects are considered equal if they
    ///  have the same domain value ptrs.
    static void Profile(llvm::FoldingSetNodeID& id, const ProgramState* s) {
        for (const auto& [_, val] : s->m_dom_val) {
            id.AddPointer(val.get());
        }
    }

    /// \brief Used to profile the contents of this object for inclusion
    ///  in a FoldingSet.
    void Profile(llvm::FoldingSetNodeID& id) const { Profile(id, this); }
}; // class ProgramState

class ProgramStateManager {
    friend class ProgramState;

    using DomValMap = ProgramState::DomValMap;
    using ValRefSet = ProgramState::ValRefSet;

  private:
    /// \brief enabled domain IDs.
    DomIDs m_ids;

    /// \brief Analysis manager.
    AnalysisManager& m_analysis_mgr;

    /// \brief  FoldingSet containing all the states created for analyzing
    ///  a particular function.  This is used to unique states.
    llvm::FoldingSet< ProgramState > m_state_set;

    /// \brief A BumpPtrAllocator to allocate states.
    llvm::BumpPtrAllocator& m_alloc;

    /// \brief A vector of ProgramStates that we can reuse.
    std::vector< ProgramState* > m_free_states;

  public:
    ProgramStateManager(AnalysisManager& analysis_mgr,
                        llvm::BumpPtrAllocator& alloc)
        : m_analysis_mgr(analysis_mgr), m_alloc(alloc) {}

  public:
    const DomIDs& dom_ids() const { return m_ids; }

    llvm::BumpPtrAllocator& get_allocator() { return m_alloc; }

  public:
    ProgramStateRef get_default_state(AnalysisIDSet analysis_ids);

    ProgramStateRef get_persistent_state(ProgramState& State);

    ProgramStateRef get_persistent_state_with_dom_val_map(ProgramState& state,
                                                          DomValMap dom_val);

  private:
    friend void retain_state(ProgramStateRawPtr state);
    friend void release_state(ProgramStateRawPtr state);
}; // class ProgramStateManager

} // namespace knight::dfa

namespace llvm {
template <> struct IntrusiveRefCntPtrInfo< const knight::dfa::ProgramState > {
    static void retain(knight::dfa::ProgramStateRawPtr state) {
        knight::dfa::retain_state(state);
    }
    static void release(knight::dfa::ProgramStateRawPtr state) {
        knight::dfa::release_state(state);
    }
};
} // namespace llvm