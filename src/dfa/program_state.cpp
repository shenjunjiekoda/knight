//===- program_state.cpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the program state class.
//
//===------------------------------------------------------------------===//

#include "dfa/program_state.hpp"
#include "dfa/analysis/analysis_base.hpp"
#include "dfa/checker/checker_base.hpp"
#include "dfa/constraint/constraint.hpp"
#include "dfa/constraint/linear.hpp"
#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"
#include "dfa/location_context.hpp"
#include "dfa/region/region.hpp"
#include "dfa/stack_frame.hpp"
#include "dfa/symbol.hpp"
#include "dfa/symbol_manager.hpp"
#include "util/assert.hpp"

#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/STLExtras.h>

#include <memory>
#include <optional>

namespace knight::dfa {

void retain_state(const ProgramState* state) {
    ++const_cast< ProgramState* >(state)->m_ref_cnt;
}

void release_state(const ProgramState* state) {
    knight_assert(state->m_ref_cnt > 0);
    auto* s = const_cast< ProgramState* >(state);
    if (--s->m_ref_cnt == 0) {
        auto& mgr = s->get_state_manager();
        mgr.m_state_set.RemoveNode(s);
        s->~ProgramState();
        mgr.m_free_states.push_back(s);
    }
}

ProgramState::ProgramState(ProgramStateManager* state_mgr,
                           RegionManager* region_mgr,
                           DomValMap dom_val,
                           RegionSExprMap region_sexpr,
                           StmtSExprMap stmt_sexpr,
                           ConstraintSystem cst_system)
    : m_state_mgr(state_mgr),
      m_region_mgr(region_mgr),
      m_ref_cnt(0),
      m_dom_val(std::move(dom_val)),
      m_region_sexpr(std::move(region_sexpr)),
      m_stmt_sexpr(std::move(stmt_sexpr)),
      m_constraint_system(std::move(cst_system)) {}

ProgramState::ProgramState(ProgramState&& other) noexcept
    : m_state_mgr(other.m_state_mgr),
      m_region_mgr(other.m_region_mgr),
      m_ref_cnt(0),
      m_dom_val(std::move(other.m_dom_val)),
      m_region_sexpr(std::move(other.m_region_sexpr)),
      m_stmt_sexpr(std::move(other.m_stmt_sexpr)),
      m_constraint_system(std::move(other.m_constraint_system)) {}

ProgramStateManager& ProgramState::get_state_manager() const {
    return *m_state_mgr;
}

std::optional< MemRegionRef > ProgramState::get_region(
    ProcCFG::DeclRef decl, const StackFrame* frame) const {
    if (llvm::isa< clang::VarDecl >(decl)) {
        return get_region_manager().get_region(llvm::cast< clang::VarDecl >(
                                                   decl),
                                               frame);
    }
    llvm::errs() << "unhandled decl type: " << decl->getDeclKindName() << "\n";
    return std::nullopt;
}

std::optional< ZVariable > ProgramState::try_get_zvariable(
    ProcCFG::DeclRef decl, const StackFrame* frame) const {
    const auto* value_decl = llvm::dyn_cast< clang::ValueDecl >(decl);
    if (value_decl == nullptr || !value_decl->getType()->isIntegerType()) {
        return std::nullopt;
    }

    auto region_opt = get_region(decl, frame);
    if (!region_opt.has_value()) {
        return std::nullopt;
    }

    auto region_sexpr_opt = get_region_sexpr(region_opt.value());
    if (!region_sexpr_opt.has_value()) {
        return std::nullopt;
    }

    SymbolRef sym = llvm::dyn_cast< Sym >(region_sexpr_opt.value());
    if (sym == nullptr) {
        return std::nullopt;
    }
    return ZVariable(sym);
}

std::optional< QVariable > ProgramState::try_get_qvariable(
    ProcCFG::DeclRef decl, const StackFrame* frame) const {
    const auto* value_decl = llvm::dyn_cast< clang::ValueDecl >(decl);
    if (value_decl == nullptr || !value_decl->getType()->isFloatingType()) {
        return std::nullopt;
    }

    auto region_opt = get_region(decl, frame);
    if (!region_opt.has_value()) {
        return std::nullopt;
    }

    auto region_sexpr_opt = get_region_sexpr(region_opt.value());
    if (!region_sexpr_opt.has_value()) {
        return std::nullopt;
    }

    SymbolRef sym = llvm::dyn_cast< Sym >(region_sexpr_opt.value());
    if (sym == nullptr) {
        return std::nullopt;
    }
    return QVariable(sym);
}

std::optional< SExprRef > ProgramState::try_get_sexpr(
    ProcCFG::StmtRef expr, const LocationContext* loc_ctx) const {
    if (auto res = get_stmt_sexpr(expr)) {
        return res;
    }

    auto region_opt = get_region(expr, loc_ctx->get_stack_frame());
    if (!region_opt.has_value()) {
        return std::nullopt;
    }

    auto region_sexpr_opt = get_region_sexpr(region_opt.value());
    if (!region_sexpr_opt.has_value()) {
        if (const auto* typed_region =
                llvm::dyn_cast< TypedRegion >(region_opt.value())) {
            // Use region sval as sexpr if we did not bind this region
            // before.
            return m_state_mgr->m_symbol_mgr.get_region_sym_val(typed_region,
                                                                loc_ctx);
        }
    }
    return region_sexpr_opt;
}

std::optional< MemRegionRef > ProgramState::get_region(
    ProcCFG::StmtRef expr, const StackFrame* frame) const {
    if (const auto* decl_ref_expr =
            llvm::dyn_cast< clang::DeclRefExpr >(expr)) {
        const auto* decl = decl_ref_expr->getDecl();
        if (decl == nullptr) {
            return std::nullopt;
        }

        return get_region(decl, frame);
    }
    return std::nullopt;
}

ProgramStateRef ProgramState::set_region_sexpr(MemRegionRef region,
                                               SExprRef sexpr) const {
    auto region_sexpr = m_region_sexpr;
    region_sexpr[region] = sexpr;
    return get_state_manager()
        .get_persistent_state_with_copy_and_region_sexpr_map(*this,
                                                             region_sexpr);
}

ProgramStateRef ProgramState::set_stmt_sexpr(ProcCFG::StmtRef stmt,
                                             SExprRef sexpr) const {
    auto stmt_sexpr = m_stmt_sexpr;
    stmt_sexpr[stmt] = sexpr;
    return get_state_manager()
        .get_persistent_state_with_copy_and_stmt_sexpr_map(*this, stmt_sexpr);
}

ProgramStateRef ProgramState::set_constraint_system(
    const ConstraintSystem& cst_system) const {
    return get_state_manager()
        .get_persistent_state_with_copy_and_constraint_system(*this,
                                                              cst_system);
}

std::optional< SExprRef > ProgramState::get_region_sexpr(
    MemRegionRef region) const {
    auto it = m_region_sexpr.find(region);
    if (it != m_region_sexpr.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional< SExprRef > ProgramState::get_stmt_sexpr(
    ProcCFG::StmtRef stmt) const {
    auto it = m_stmt_sexpr.find(stmt);
    if (it != m_stmt_sexpr.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional< SExprRef > ProgramState::get_stmt_sexpr(
    ProcCFG::StmtRef stmt, const StackFrame* frame) const {
    if (auto res = get_stmt_sexpr(stmt)) {
        return res;
    }
    if (auto region_opt = get_region(stmt, frame)) {
        return get_region_sexpr(region_opt.value());
    }
    return std::nullopt;
}

SExprRef ProgramState::get_stmt_sexpr_or_conjured(
    ProcCFG::StmtRef stmt,
    const clang::QualType& type,
    const StackFrame* frame) const {
    if (auto res = get_stmt_sexpr(stmt, frame)) {
        return res.value();
    }
    return m_state_mgr->m_symbol_mgr.get_symbol_conjured(stmt, type, frame);
}

ProgramStateRef ProgramState::normalize() const {
    DomValMap dom_val = m_dom_val;
    for (auto& [id, val] : dom_val) {
        const_cast< AbsDomBase* >(val.get())->normalize();
    }
    return get_state_manager()
        .get_persistent_state_with_copy_and_dom_val_map(*this,
                                                        std::move(dom_val));
}

bool ProgramState::is_bottom() const {
    return llvm::any_of(m_dom_val, [](const auto& pair) {
        return pair.second->is_bottom();
    });
}

bool ProgramState::is_top() const {
    return llvm::all_of(m_dom_val,
                        [](const auto& pair) { return pair.second->is_top(); });
}

ProgramStateRef ProgramState::set_to_bottom() const {
    return get_state_manager().get_bottom_state();
}

ProgramStateRef ProgramState::set_to_top() const {
    return get_state_manager().get_default_state();
}

// NOLINTNEXTLINE
#define UNION_MAP(OP)                                            \
    DomValMap new_map;                                           \
    for (const auto& [other_id, other_val] : other->m_dom_val) { \
        auto it = m_dom_val.find(other_id);                      \
        if (it == m_dom_val.end()) {                             \
            new_map[other_id] = other_val->clone_shared();       \
        } else {                                                 \
            auto new_val = it->second->clone();                  \
            new_val->OP(*other_val);                             \
            new_map[other_id] = SharedVal(new_val);              \
        }                                                        \
    }                                                            \
    return get_state_manager()                                   \
        .get_persistent_state_with_copy_and_dom_val_map(*this,   \
                                                        std ::move(new_map));
// NOLINTNEXTLINE
#define INTERSECT_MAP(OP)                                      \
    DomValMap map;                                             \
    for (auto& [other_id, other_val] : other->m_dom_val) {     \
        auto it = m_dom_val.find(other_id);                    \
        if (it != m_dom_val.end()) {                           \
            auto new_val = it->second->clone();                \
            new_val->OP(*other_val);                           \
            map[other_id] = SharedVal(new_val);                \
        }                                                      \
    }                                                          \
    return get_state_manager()                                 \
        .get_persistent_state_with_copy_and_dom_val_map(*this, \
                                                        std ::move(map));

ProgramStateRef ProgramState::join(const ProgramStateRef& other) const {
    // UNION_MAP(join_with);
    DomValMap new_map;
    for (const auto& [other_id, other_val] : other->m_dom_val) {
        auto it = m_dom_val.find(other_id);
        if (it == m_dom_val.end()) {
            new_map[other_id] = other_val->clone_shared();
        } else {
            auto new_val = it->second->clone();
            new_val->join_with(*other_val);
            new_map[other_id] = SharedVal(new_val);
        }
    }

    StmtSExprMap stmt_sexpr = m_stmt_sexpr;
    stmt_sexpr.insert(other->m_stmt_sexpr.begin(), other->m_stmt_sexpr.end());

    RegionSExprMap region_sexpr = m_region_sexpr;
    for (const auto [region, sexpr] : other->m_region_sexpr) {
        auto it = region_sexpr.find(region);
        if (it == region_sexpr.end() || it->second == sexpr) {
            region_sexpr[region] = sexpr;
        } else {
            // region_sexpr[region];
        }
    }

    return get_state_manager()
        .get_persistent_state_with_copy_and_dom_val_map(*this,
                                                        std ::move(new_map));
}

ProgramStateRef ProgramState::join_at_loop_head(
    const ProgramStateRef& other) const {
    UNION_MAP(join_with_at_loop_head);
}

ProgramStateRef ProgramState::join_consecutive_iter(
    const ProgramStateRef& other) const {
    UNION_MAP(join_consecutive_iter_with);
}

ProgramStateRef ProgramState::widen(const ProgramStateRef& other) const {
    UNION_MAP(widen_with);
}

ProgramStateRef ProgramState::meet(const ProgramStateRef& other) const {
    INTERSECT_MAP(meet_with);
}

ProgramStateRef ProgramState::narrow(const ProgramStateRef& other) const {
    INTERSECT_MAP(narrow_with);
}

bool ProgramState::leq(const ProgramState& other) const {
    llvm::BitVector this_key_set;
    bool need_to_check_other = other.m_dom_val.size() != this->m_dom_val.size();
    for (const auto& [id, val] : this->m_dom_val) {
        this_key_set.set(id);
        auto dom_val = m_dom_val;
        auto it = other.m_dom_val.find(id);
        if (it == other.m_dom_val.end()) {
            if (!val->is_bottom()) {
                return false;
            }
            need_to_check_other = true;
            continue;
        }
        if (!val->leq(*(it->second))) {
            return false;
        }
    }

    if (!need_to_check_other) {
        return true;
    }

    for (const auto& [id, val] : other.m_dom_val) {
        if (!this_key_set[id]) {
            if (!val->is_top()) {
                return false;
            }
        }
    }

    return true;
}

bool ProgramState::equals(const ProgramState& other) const {
    return llvm::all_of(this->m_dom_val, [&other](const auto& this_pair) {
        auto it = other.m_dom_val.find(this_pair.first);
        return it != other.m_dom_val.end() &&
               this_pair.second->equals(*(it->second));
    });
}

void ProgramState::dump(llvm::raw_ostream& os) const {
    os << "ProgramState:{\n";
    for (const auto& [id, aval] : m_dom_val) {
        os << "[" << get_domain_name_by_id(id) << "]: ";
        aval->dump(os);
        os << "\n";
    }
    os << "}\n";
}

ProgramStateRef ProgramStateManager::get_default_state() {
    DomValMap dom_val;
    for (auto analysis_id : m_analysis_mgr.get_required_analyses()) {
        for (auto dom_id :
             m_analysis_mgr.get_registered_domains_in(analysis_id)) {
            if (auto default_fn =
                    m_analysis_mgr.get_domain_default_val_fn(dom_id)) {
                dom_val[dom_id] = std::move((*default_fn)());
            }
        }
    }
    ProgramState state(this,
                       &m_region_mgr,
                       std::move(dom_val),
                       RegionSExprMap{},
                       StmtSExprMap{},
                       ConstraintSystem{});

    return get_persistent_state(state);
}

ProgramStateRef ProgramStateManager::get_bottom_state() {
    DomValMap dom_val;
    for (auto analysis_id : m_analysis_mgr.get_required_analyses()) {
        for (auto dom_id :
             m_analysis_mgr.get_registered_domains_in(analysis_id)) {
            if (auto bottom_fn =
                    m_analysis_mgr.get_domain_bottom_val_fn(dom_id)) {
                dom_val[dom_id] = std::move((*bottom_fn)());
            }
        }
    }
    ProgramState state(this,
                       &m_region_mgr,
                       std::move(dom_val),
                       RegionSExprMap{},
                       StmtSExprMap{},
                       ConstraintSystem{});

    return get_persistent_state(state);
}

ProgramStateRef ProgramStateManager::get_persistent_state(ProgramState& state) {
    llvm::FoldingSetNodeID id;
    state.Profile(id);
    void* insert_pos; // NOLINT

    if (ProgramState* existed =
            m_state_set.FindNodeOrInsertPos(id, insert_pos)) {
        return existed;
    }

    ProgramState* new_state = nullptr;
    if (!m_free_states.empty()) {
        new_state = m_free_states.back();
        m_free_states.pop_back();
    } else {
        new_state = m_alloc.Allocate< ProgramState >();
    }
    new (new_state) ProgramState(std::move(state));
    m_state_set.InsertNode(new_state, insert_pos);
    return new_state;
}

ProgramStateRef ProgramStateManager::
    get_persistent_state_with_ref_and_dom_val_map(ProgramState& state,
                                                  DomValMap dom_val) {
    state.m_dom_val = std::move(dom_val);
    return get_persistent_state(state);
}

ProgramStateRef ProgramStateManager::
    get_persistent_state_with_copy_and_dom_val_map(const ProgramState& state,
                                                   DomValMap dom_val) {
    ProgramState new_state(state.m_state_mgr,
                           state.m_region_mgr,
                           std::move(dom_val),
                           state.m_region_sexpr,
                           state.m_stmt_sexpr,
                           state.m_constraint_system);
    return get_persistent_state(new_state);
}

ProgramStateRef ProgramStateManager::
    get_persistent_state_with_copy_and_region_sexpr_map(
        const ProgramState& state, RegionSExprMap region_sexpr) {
    ProgramState new_state(state.m_state_mgr,
                           state.m_region_mgr,
                           state.m_dom_val,
                           std::move(region_sexpr),
                           state.m_stmt_sexpr,
                           state.m_constraint_system);

    return get_persistent_state(new_state);
}
ProgramStateRef ProgramStateManager::
    get_persistent_state_with_copy_and_stmt_sexpr_map(const ProgramState& state,
                                                      StmtSExprMap stmt_sexpr) {
    ProgramState new_state(state.m_state_mgr,
                           state.m_region_mgr,
                           state.m_dom_val,
                           state.m_region_sexpr,
                           std::move(stmt_sexpr),
                           state.m_constraint_system);

    return get_persistent_state(new_state);
}

ProgramStateRef ProgramStateManager::
    get_persistent_state_with_copy_and_constraint_system(
        const ProgramState& state, ConstraintSystem cst_system) {
    ProgramState new_state(state.m_state_mgr,
                           state.m_region_mgr,
                           state.m_dom_val,
                           state.m_region_sexpr,
                           state.m_stmt_sexpr,
                           std::move(cst_system));

    return get_persistent_state(new_state);
}

ProgramStateRef internal::get_persistent_state_with_copy_and_dom_val_map(
    ProgramStateManager& manager,
    const ProgramState& state,
    DomValMap dom_val) {
    return manager.get_persistent_state_with_copy_and_dom_val_map(state,
                                                                  std::move(
                                                                      dom_val));
}

} // namespace knight::dfa