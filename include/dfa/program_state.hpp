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
#include "dfa/constraint/constraint.hpp"
#include "dfa/constraint/linear.hpp"
#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"
#include "dfa/location_context.hpp"
#include "dfa/proc_cfg.hpp"
#include "dfa/region/region.hpp"
#include "dfa/stack_frame.hpp"
#include "dfa/symbol.hpp"

#include <optional>
#include <unordered_set>

#include <llvm/ADT/ImmutableMap.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

class ProgramState;
class ProgramStateManager;
class SymbolManager;

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
using DomValMap = llvm::DenseMap< DomID, SharedVal >;
using RegionSExprMap = llvm::DenseMap< MemRegionRef, SExprRef >;
using StmtSExprMap = llvm::DenseMap< ProcCFG::StmtRef, SExprRef >;

namespace internal {

ProgramStateRef get_persistent_state_with_copy_and_dom_val_map(
    ProgramStateManager& manager, const ProgramState& state, DomValMap dom_val);

} // namespace internal

// TODO(ProgramState): fix ProgramState to be immutable!!
class ProgramState : public llvm::FoldingSetNode {
    friend class ProgramStateManager;

  public:
    using ValRefSet = std::unordered_set< AbsValRef >;

  private:
    /// \brief used in the refeference counting.
    unsigned m_ref_cnt;

    /// \brief domain value map. (domain id -> abstract value)
    DomValMap m_dom_val;

    /// \brief region sexpr map. (memory region -> sexpr)
    RegionSExprMap m_region_sexpr;

    /// \brief stmt sexpr map. (clang stmt -> sexpr)
    StmtSExprMap m_stmt_sexpr;

    /// \brief constraint system.
    ConstraintSystem m_constraint_system;

    /// \brief state manager.
    ProgramStateManager* m_state_mgr;

    /// \brief region manager.
    RegionManager* m_region_mgr;

  public:
    ProgramState(ProgramStateManager* state_mgr,
                 RegionManager* region_mgr,
                 DomValMap dom_val,
                 RegionSExprMap region_sexpr,
                 StmtSExprMap stmt_sexpr,
                 ConstraintSystem cst_system);

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
    [[nodiscard]] ProgramStateManager& get_state_manager() const;

    /// \brief Return the region manager.
    [[nodiscard]] RegionManager& get_region_manager() const {
        return *m_region_mgr;
    }

  public:
    [[nodiscard]] std::optional< MemRegionRef > get_region(
        ProcCFG::DeclRef decl, const StackFrame*) const;
    [[nodiscard]] std::optional< MemRegionRef > get_region(
        ProcCFG::StmtRef stmt, const StackFrame*) const;
    [[nodiscard]] std::optional< const TypedRegion* > get_typed_region(
        ProcCFG::DeclRef decl, const StackFrame* frame) const {
        if (auto region = get_region(decl, frame)) {
            if (const auto* typed_region =
                    dyn_cast< TypedRegion >(region.value())) {
                return typed_region;
            }
        }
        return std::nullopt;
    }
    [[nodiscard]] std::optional< const TypedRegion* > get_typed_region(
        ProcCFG::StmtRef stmt, const StackFrame* frame) const {
        if (auto region = get_region(stmt, frame)) {
            if (const auto* typed_region =
                    dyn_cast< TypedRegion >(region.value())) {
                return typed_region;
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] std::optional< ZVariable > try_get_zvariable(
        ProcCFG::DeclRef decl, const StackFrame* frame) const;
    [[nodiscard]] std::optional< QVariable > try_get_qvariable(
        ProcCFG::DeclRef decl, const StackFrame* frame) const;

    [[nodiscard]] ProgramStateRef set_region_sexpr(MemRegionRef region,
                                                   SExprRef sexpr) const;
    [[nodiscard]] ProgramStateRef set_stmt_sexpr(ProcCFG::StmtRef stmt,
                                                 SExprRef sexpr) const;
    [[nodiscard]] ProgramStateRef set_constraint_system(
        const ConstraintSystem& cst_system) const;

    [[nodiscard]] std::optional< SExprRef > get_region_sexpr(
        MemRegionRef region) const;
    [[nodiscard]] std::optional< SExprRef > get_stmt_sexpr(
        ProcCFG::StmtRef stmt) const;
    [[nodiscard]] std::optional< SExprRef > get_stmt_sexpr(
        ProcCFG::StmtRef stmt, const StackFrame* frame) const;
    [[nodiscard]] SExprRef get_stmt_sexpr_or_conjured(
        ProcCFG::StmtRef stmt,
        const clang::QualType& type,
        const LocationContext* loc_ctx) const;
    [[nodiscard]] SExprRef get_stmt_sexpr_or_conjured(
        const clang::Expr* expr, const LocationContext* loc_ctx) const {
        return get_stmt_sexpr_or_conjured(expr, expr->getType(), loc_ctx);
    }

  public:
    [[nodiscard]] ProgramStateRef add_zlinear_constraint(
        const ZLinearConstraint& constraint) const {
        auto cst_system = m_constraint_system;
        cst_system.add_zlinear_constraint(constraint);
        return set_constraint_system(cst_system);
    }

    [[nodiscard]] ProgramStateRef merge_zlinear_constraint_system(
        const ZLinearConstraintSystem& system) const {
        auto cst_system = m_constraint_system;
        cst_system.merge_zlinear_constraint_system(system);
        return set_constraint_system(cst_system);
    }

    [[nodiscard]] ProgramStateRef add_qlinear_constraint(
        const QLinearConstraint& constraint) const {
        auto cst_system = m_constraint_system;
        cst_system.add_qlinear_constraint(constraint);
        return set_constraint_system(cst_system);
    }

    [[nodiscard]] ProgramStateRef merge_qlinear_constraint_system(
        const QLinearConstraintSystem& system) const {
        auto cst_system = m_constraint_system;
        cst_system.merge_qlinear_constraint_system(system);
        return set_constraint_system(cst_system);
    }

    [[nodiscard]] ProgramStateRef add_non_linear_constraint(
        const SExprRef& constraint) const {
        auto cst_system = m_constraint_system;
        cst_system.add_non_linear_constraint(constraint);
        return set_constraint_system(cst_system);
    }

    [[nodiscard]] ProgramStateRef merge_non_linear_constraint_set(
        const ConstraintSystem::NonLinearConstraintSet& set) const {
        auto cst_system = m_constraint_system;
        cst_system.merge_non_linear_constraint_set(set);
        return set_constraint_system(cst_system);
    }

  public:
    /// \brief Check if the given domain kind exists in the program state.
    ///
    /// \return True if the domain kind exists, false otherwise.
    template < typename Domain >
    [[nodiscard]] bool exists() const {
        return get_ref(Domain::get_kind()).has_value();
    }

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

    /// \brief Get the cloned abstract value with the given domain.
    ///
    /// \return cloned abstract value which is abled to be modified
    /// and managed by the caller, or nullptr if the domain does not
    /// exist in the state.
    template < typename Domain >
    [[nodiscard]] std::shared_ptr< Domain > get_clone() const {
        auto it = m_dom_val.find(get_domain_id(Domain::get_kind()));
        if (it == m_dom_val.end()) {
            return std::static_pointer_cast< Domain >(Domain::default_val());
        }
        std::shared_ptr< AbsDomBase > base_ptr(it->second->clone());
        return std::static_pointer_cast< Domain >(base_ptr);
    }

    /// \brief Remove the given domain from the program state.
    template < typename Domain >
    [[nodiscard]] ProgramStateRef remove() const {
        auto id = get_domain_id(Domain::get_kind());
        auto dom_val = m_dom_val;
        dom_val.erase(id);
        return internal::
            get_persistent_state_with_copy_and_dom_val_map(get_state_manager(),
                                                           *this,
                                                           std::move(dom_val));
    }

    /// \brief Set the given domain to the given abstract val.
    template < typename Domain >
    [[nodiscard]] ProgramStateRef set(SharedVal val) const {
        auto dom_val = m_dom_val;
        dom_val[get_domain_id(Domain::get_kind())] = std::move(val);
        return internal::
            get_persistent_state_with_copy_and_dom_val_map(get_state_manager(),
                                                           *this,
                                                           std::move(dom_val));
    }

  public:
    [[nodiscard]] ProgramStateRef normalize() const;

    [[nodiscard]] bool is_bottom() const;
    [[nodiscard]] bool is_top() const;
    [[nodiscard]] ProgramStateRef set_to_bottom() const;
    [[nodiscard]] ProgramStateRef set_to_top() const;

    /// TODO(symbol): if a region `reg` have different values in `se1`
    /// and `se2`, join shall assign a symbolic value `sval` to `reg`.
    /// And `sval` shall join the constraint of `se1` and `se2`.
    ///
    /// We can add the constraint `sval = se1` and `sval = se2` to two
    /// states and join them, and `reg` will be assigned `sval` in the
    /// joined state.
    [[nodiscard]] ProgramStateRef join(const ProgramStateRef& other) const;
    [[nodiscard]] ProgramStateRef join_at_loop_head(
        const ProgramStateRef& other) const;
    [[nodiscard]] ProgramStateRef join_consecutive_iter(
        const ProgramStateRef& other) const;

    [[nodiscard]] ProgramStateRef widen(const ProgramStateRef& other) const;
    [[nodiscard]] ProgramStateRef meet(const ProgramStateRef& other) const;
    [[nodiscard]] ProgramStateRef narrow(const ProgramStateRef& other) const;

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
        for (const auto& [dom_id, val] : s->m_dom_val) {
            id.AddInteger(dom_id);
            id.AddPointer(val.get());
        }
        for (const auto& [region, sexpr] : s->m_region_sexpr) {
            id.AddPointer(region);
            id.AddPointer(sexpr);
        }
        for (const auto& [stmt, sexpr] : s->m_stmt_sexpr) {
            id.AddPointer(stmt);
            id.AddPointer(sexpr);
        }
        s->m_constraint_system.Profile(id);
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

    using ValRefSet = ProgramState::ValRefSet;

  private:
    /// \brief enabled domain IDs.
    DomIDs m_ids;

    /// \brief Analysis manager.
    AnalysisManager& m_analysis_mgr;

    /// \brief Region manager.
    RegionManager& m_region_mgr;

    /// \brief Symbol manager.
    SymbolManager& m_symbol_mgr;

    /// \brief  FoldingSet containing all the states created for analyzing
    ///  a particular function.  This is used to unique states.
    llvm::FoldingSet< ProgramState > m_state_set;

    /// \brief A BumpPtrAllocator to allocate states.
    llvm::BumpPtrAllocator& m_alloc;

    /// \brief A vector of ProgramStates that we can reuse.
    std::vector< ProgramState* > m_free_states;

  public:
    ProgramStateManager(AnalysisManager& analysis_mgr,
                        RegionManager& region_mgr,
                        SymbolManager& symbol_mgr,
                        llvm::BumpPtrAllocator& alloc)
        : m_analysis_mgr(analysis_mgr),
          m_region_mgr(region_mgr),
          m_symbol_mgr(symbol_mgr),
          m_alloc(alloc) {}

  public:
    [[nodiscard]] const DomIDs& dom_ids() const { return m_ids; }

    [[nodiscard]] llvm::BumpPtrAllocator& get_allocator() { return m_alloc; }

  public:
    [[nodiscard]] ProgramStateRef get_default_state();
    [[nodiscard]] ProgramStateRef get_bottom_state();

    [[nodiscard]] ProgramStateRef get_persistent_state(ProgramState& State);

    [[nodiscard]] ProgramStateRef get_persistent_state_with_ref_and_dom_val_map(
        ProgramState& state, DomValMap dom_val);
    [[nodiscard]] ProgramStateRef
    get_persistent_state_with_copy_and_dom_val_map(const ProgramState& state,
                                                   DomValMap dom_val);
    [[nodiscard]] ProgramStateRef
    get_persistent_state_with_copy_and_region_sexpr_map(
        const ProgramState& state, RegionSExprMap region_sexpr);
    [[nodiscard]] ProgramStateRef
    get_persistent_state_with_copy_and_stmt_sexpr_map(const ProgramState& state,
                                                      StmtSExprMap stmt_sexpr);
    [[nodiscard]] ProgramStateRef
    get_persistent_state_with_copy_and_constraint_system(
        const ProgramState& state, ConstraintSystem cst_system);
    [[nodiscard]] ProgramStateRef
    get_persistent_state_with_copy_and_stateful_member_map(
        const ProgramState& state,
        DomValMap dom_val,
        RegionSExprMap region_sexpr,
        StmtSExprMap stmt_sexpr,
        ConstraintSystem system);

  private:
    friend void retain_state(const ProgramState* state);
    friend void release_state(const ProgramState* state);
}; // class ProgramStateManager

} // namespace knight::dfa
