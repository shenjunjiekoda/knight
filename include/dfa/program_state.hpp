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
#include <unordered_set>

#include <llvm/ADT/ImmutableMap.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

class ProgramState;
class ProgramStateManager;

void retain_state(const ProgramState* state);
void release_state(const ProgramState* state);

} // namespace knight::dfa

namespace llvm {

template <>
struct IntrusiveRefCntPtrInfo< const knight::dfa::ProgramState > {
    static void retain(const knight::dfa::ProgramState* state) {
        knight::dfa::retain_state(state);
    }
    static void release(const knight::dfa::ProgramState* state) {
        knight::dfa::release_state(state);
    }
}; // struct IntrusiveRefCntPtrInfo

} // namespace llvm

namespace knight::dfa {

using ProgramStateRef = llvm::IntrusiveRefCntPtr< const ProgramState >;

// TODO(ProgramState): fix ProgramState to be immutable!!
class ProgramState : public llvm::FoldingSetNode {
    friend class ProgramStateManager;

  public:
    using DomValMap = llvm::DenseMap< DomID, SharedVal >;
    using ValRefSet = std::unordered_set< AbsValRef >;

  private:
    unsigned m_ref_cnt;
    DomValMap m_dom_val;
    ProgramStateManager* m_mgr;

  public:
    ProgramState(ProgramStateManager* mgr, DomValMap dom_val);

    /// \brief Only move constructor is allowed.
    ProgramState(ProgramState&& other) noexcept;
    ProgramState(const ProgramState& other) = delete;
    void operator=(const ProgramState& other) = delete;
    void operator=(ProgramState&& other) = delete;

    ~ProgramState() = default;

  private:
    friend void retain_state(const ProgramState* state);
    friend void release_state(const ProgramState* state);

  public:
    /// \brief Return the state manager.
    [[nodiscard]] ProgramStateManager& get_manager() const;

    /// \brief Check if the given domain kind exists in the program state.
    ///
    /// \return True if the domain kind exists, false otherwise.
    template < typename Domain >
    [[nodiscard]] bool exists() const;

    /// \brief Get the abstract cal with the given domain.
    template < typename Domain >
    [[nodiscard]] std::optional< const Domain* > get_ref() const {
        auto it = m_dom_val.find(get_domain_id(Domain::get_kind()));
        if (it == m_dom_val.end()) {
            return std::nullopt;
        }
        return std::make_optional(
            static_cast< const Domain* >(it->second.get()));
    }

    template < typename Domain >
    [[nodiscard]] std::shared_ptr< const Domain > get() const {
        auto it = m_dom_val.find(get_domain_id(Domain::get_kind()));
        if (it == m_dom_val.end()) {
            return std::static_pointer_cast< const Domain >(
                Domain::default_val());
        }
        return std::static_pointer_cast< const Domain >(
            it->second->clone_shared());
    }

    /// \brief Remove the given domain from the program state.
    template < typename Domain >
    [[nodiscard]] ProgramStateRef remove() const;

    /// \brief Set the given domain to the given abstract val.
    template < typename Domain >
    [[nodiscard]] ProgramStateRef set(SharedVal val) const;

  public:
    [[nodiscard]] ProgramStateRef normalize() const;

    [[nodiscard]] bool is_bottom() const;
    [[nodiscard]] bool is_top() const;
    [[nodiscard]] ProgramStateRef set_to_bottom() const;
    [[nodiscard]] ProgramStateRef set_to_top() const;

    [[nodiscard]] ProgramStateRef join(ProgramStateRef other) const;
    [[nodiscard]] ProgramStateRef join_at_loop_head(
        ProgramStateRef other) const;
    [[nodiscard]] ProgramStateRef join_consecutive_iter(
        ProgramStateRef other) const;

    [[nodiscard]] ProgramStateRef widen(ProgramStateRef other) const;
    [[nodiscard]] ProgramStateRef meet(ProgramStateRef other) const;
    [[nodiscard]] ProgramStateRef narrow(ProgramStateRef other) const;

    [[nodiscard]] bool leq(const ProgramState& other) const;
    [[nodiscard]] bool equals(const ProgramState& other) const;

    [[nodiscard]] bool operator==(const ProgramState& other) const {
        return equals(other);
    }
    [[nodiscard]] bool operator!=(const ProgramState& other) const {
        return !equals(other);
    }

    void dump(llvm::raw_ostream& os) const;

  public:
    /// \brief Profile the contents of a ProgramState object for use in a
    ///  FoldingSet.  Two ProgramState objects are considered equal if they
    ///  have the same domain value ptrs.
    static void Profile(llvm::FoldingSetNodeID& id, // NOLINT
                        const ProgramState* s) {
        for (const auto& [_, val] : s->m_dom_val) {
            id.AddPointer(val.get());
        }
    }

    /// \brief Used to profile the contents of this object for inclusion
    ///  in a FoldingSet.
    void Profile(llvm::FoldingSetNodeID& id) const // NOLINT
    {
        Profile(id, this);
    }
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
    [[nodiscard]] const DomIDs& dom_ids() const { return m_ids; }

    llvm::BumpPtrAllocator& get_allocator() { return m_alloc; }

  public:
    ProgramStateRef get_default_state();
    ProgramStateRef get_bottom_state();

    ProgramStateRef get_persistent_state(ProgramState& State);

    ProgramStateRef get_persistent_state_with_ref_and_dom_val_map(
        ProgramState& state, DomValMap dom_val);
    ProgramStateRef get_persistent_state_with_copy_and_dom_val_map(
        const ProgramState& state, DomValMap dom_val);

  private:
    friend void retain_state(const ProgramState* state);
    friend void release_state(const ProgramState* state);
}; // class ProgramStateManager

} // namespace knight::dfa
