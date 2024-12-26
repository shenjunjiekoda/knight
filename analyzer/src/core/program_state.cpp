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

#include "analyzer/core/program_state.hpp"
#include "analyzer/core/analysis/analysis_base.hpp"
#include "analyzer/core/checker/checker_base.hpp"
#include "analyzer/core/constraint/constraint.hpp"
#include "analyzer/core/constraint/linear.hpp"
#include "analyzer/core/domain/dom_base.hpp"
#include "analyzer/core/domain/domains.hpp"
#include "analyzer/core/domain/numerical/numerical_base.hpp"
#include "analyzer/core/location_context.hpp"
#include "analyzer/core/region/region.hpp"
#include "analyzer/core/stack_frame.hpp"
#include "analyzer/core/symbol.hpp"
#include "analyzer/core/symbol_manager.hpp"
#include "common/util/assert.hpp"
#include "common/util/log.hpp"

#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/WithColor.h>
#include <llvm/Support/raw_ostream.h>

#include <limits>
#include <memory>
#include <optional>

#define DEBUG_TYPE "program-state"

namespace knight::analyzer {

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
                           RegionDefMap region_defs,
                           StmtSExprMap stmt_sexpr,
                           ConstraintSystem cst_system)
    : m_state_mgr(state_mgr),
      m_region_mgr(region_mgr),
      m_ref_cnt(0),
      m_zdom_kind(state_mgr->get_zdom_kind()),
      m_dom_val(std::move(dom_val)),
      m_region_defs(std::move(region_defs)),
      m_stmt_sexpr(std::move(stmt_sexpr)),
      m_constraint_system(std::move(cst_system)) {}

ProgramState::ProgramState(ProgramState&& other) noexcept
    : m_state_mgr(other.m_state_mgr),
      m_region_mgr(other.m_region_mgr),
      m_ref_cnt(0),
      m_zdom_kind(other.m_state_mgr->get_zdom_kind()),
      m_dom_val(std::move(other.m_dom_val)),
      m_region_defs(std::move(other.m_region_defs)),
      m_stmt_sexpr(std::move(other.m_stmt_sexpr)),
      m_constraint_system(std::move(other.m_constraint_system)) {}

ProgramStateManager& ProgramState::get_state_manager() const {
    return *m_state_mgr;
}

std::optional< RegionRef > ProgramState::get_region(
    ProcCFG::DeclRef decl, const StackFrame* frame) const {
    if (llvm::isa< clang::VarDecl >(decl)) {
        return get_region_manager().get_region(llvm::cast< clang::VarDecl >(
                                                   decl),
                                               frame);
    }
    knight_log(llvm::WithColor::error()
               << "unhandled decl type: " << decl->getDeclKindName() << "\n");
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

    if (auto region_def = get_region_def(region_opt.value(), frame)) {
        return ZVariable(*region_def);
    }
    return std::nullopt;
}

std::optional< RegionRef > ProgramState::get_region(
    ProcCFG::StmtRef stmt, const StackFrame* frame) const {
    if (const auto* imp_cast =
            llvm::dyn_cast< clang::ImplicitCastExpr >(stmt)) {
        if (imp_cast->getCastKind() == clang::CK_LValueToRValue) {
            stmt = imp_cast->getSubExpr();
        }
    }

    if (const auto* decl_ref_expr =
            llvm::dyn_cast< clang::DeclRefExpr >(stmt)) {
        const auto* decl = decl_ref_expr->getDecl();
        if (decl == nullptr) {
            return std::nullopt;
        }

        return get_region(decl, frame);
    }
    return std::nullopt;
}

ProgramStateRef ProgramState::set_region_def(RegionRef region,
                                             const StackFrame* frame,
                                             const RegionDef* def) const {
    auto region_defs = m_region_defs;
    region_defs[{region, frame}] = def;
    return get_state_manager()
        .get_persistent_state_with_copy_and_region_defs_map(*this, region_defs);
}

ProgramStateRef ProgramState::set_stmt_sexpr(ProcCFG::StmtRef stmt,
                                             const StackFrame* frame,
                                             SExprRef sexpr) const {
    auto stmt_sexpr = m_stmt_sexpr;
    stmt_sexpr[{stmt, frame}] = sexpr;
    return get_state_manager()
        .get_persistent_state_with_copy_and_stmt_sexpr_map(*this, stmt_sexpr);
}

ProgramStateRef ProgramState::set_constraint_system(
    const ConstraintSystem& cst_system) const {
    return get_state_manager()
        .get_persistent_state_with_copy_and_constraint_system(*this,
                                                              cst_system);
}

std::optional< const RegionDef* > ProgramState::get_region_def(
    RegionRef region, const StackFrame* frame) const {
    auto it = m_region_defs.find({region, frame});
    if (it != m_region_defs.end()) {
        return it->second;
    }
    if (region->get_memory_space()->is_stack_arg()) {
        return get_state_manager()
            .m_symbol_mgr.get_region_def(region, frame->get_entry_location());
    }
    return std::nullopt;
}

std::optional< SExprRef > ProgramState::get_stmt_sexpr(
    ProcCFG::StmtRef stmt, const StackFrame* frame) const {
    if (auto region = get_region(stmt, frame)) {
        if (auto region_def = get_region_def(*region, frame)) {
            return region_def;
        }
    }

    auto it = m_stmt_sexpr.find({stmt, frame});
    if (it != m_stmt_sexpr.end()) {
        return it->second;
    }

    return std::nullopt;
}

SExprRef ProgramState::get_stmt_sexpr_or_conjured(
    ProcCFG::StmtRef stmt,
    const clang::QualType& type,
    const LocationContext* loc_ctx) const {
    const auto* frame = loc_ctx->get_stack_frame();

    auto region_opt = get_region(stmt, frame);
    if (region_opt) {
        if (auto region_def = get_region_def(*region_opt, frame)) {
            return *region_def;
        }
    }

    auto it = m_stmt_sexpr.find({stmt, frame});
    if (it != m_stmt_sexpr.end()) {
        return it->second;
    }

    if (region_opt) {
        return m_state_mgr->m_symbol_mgr.get_region_def(*region_opt, loc_ctx);
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
#define UNION_MAP(OP)                                                          \
    DomValMap new_map;                                                         \
    for (const auto& [other_id, other_val] : other->m_dom_val) {               \
        auto it = m_dom_val.find(other_id);                                    \
        if (it == m_dom_val.end()) {                                           \
            new_map[other_id] = other_val->clone_shared();                     \
        } else {                                                               \
            auto* new_val = it->second->clone();                               \
            new_val->OP(*other_val);                                           \
            new_map[other_id] = SharedVal(new_val);                            \
        }                                                                      \
    }                                                                          \
                                                                               \
    StmtSExprMap stmt_sexpr;                                                   \
    llvm::for_each(m_stmt_sexpr, [&](const auto& pair) {                       \
        const auto& [stmt, sexpr] = pair;                                      \
        auto it = other->m_stmt_sexpr.find(stmt);                              \
        if (it == other->m_stmt_sexpr.end() || it->second == sexpr) {          \
            stmt_sexpr[stmt] = sexpr;                                          \
        }                                                                      \
    });                                                                        \
                                                                               \
    llvm::for_each(other->m_stmt_sexpr, [&](const auto& pair) {                \
        const auto& [stmt, sexpr] = pair;                                      \
        if (!stmt_sexpr.contains(stmt)) {                                      \
            stmt_sexpr[stmt] = sexpr;                                          \
        }                                                                      \
    });                                                                        \
                                                                               \
    RegionDefMap region_defs = m_region_defs;                                  \
    std::map< const RegionDef*,                                                \
              std::pair< const RegionDef*, const RegionDef* > >                \
        new_zregion_def;                                                       \
    for (const auto [region_frame_pair, def] : other->m_region_defs) {         \
        auto it = region_defs.find(region_frame_pair);                         \
        if (it == region_defs.end() || it->second == def) {                    \
            region_defs[region_frame_pair] = def;                              \
        } else {                                                               \
            const auto& [region, _] = region_frame_pair;                       \
                                                                               \
            knight_log_nl(llvm::outs() << "loc ctx#" << loc_ctx << "\n";       \
                          loc_ctx->dump(llvm::outs()););                       \
            knight_log_nl(llvm::outs() << "region#" << region << "\n";         \
                          region->dump(llvm::outs()););                        \
                                                                               \
            const auto* new_def =                                              \
                get_state_manager().m_symbol_mgr.get_region_def(region,        \
                                                                loc_ctx);      \
                                                                               \
            if (region->get_value_type()->isIntegralOrEnumerationType()) {     \
                new_zregion_def[new_def] = {it->second, def};                  \
                knight_log(                                                    \
                    llvm::outs() << "prepare new def: " << new_def << " with " \
                                 << it->second << " and " << def << "\n";      \
                    llvm::FoldingSetNodeID id;                                 \
                    new_def->Profile(id);                                      \
                    llvm::outs()                                               \
                    << "new region def id: " << id.ComputeHash() << "\n";      \
                    id.clear();                                                \
                    it->second->Profile(id);                                   \
                    llvm::outs()                                               \
                    << "this region def id: " << id.ComputeHash() << "\n";     \
                    id.clear();                                                \
                    def->Profile(id);                                          \
                    llvm::outs()                                               \
                    << "other region def id: " << id.ComputeHash() << "\n";);  \
            } else {                                                           \
                knight_log(llvm::outs() << "unsupported region type: "         \
                                        << region->get_value_type() << "\n");  \
                knight_unreachable("unsupported region type");                 \
            }                                                                  \
            region_defs[region_frame_pair] = new_def;                          \
                                                                               \
            knight_log(llvm::outs()                                            \
                       << #OP " region `" << *region                           \
                       << "` assigned new def: " << new_def << "\n");          \
        }                                                                      \
    }                                                                          \
                                                                               \
    if (!new_zregion_def.empty()) {                                            \
        auto* zdom =                                                           \
            dynamic_cast< ZNumericalDomBase* >(new_map[get_zdom_id()].get());  \
        auto* zdom_cloned = dynamic_cast< ZNumericalDomBase* >(                \
            new_map[get_zdom_id()]->clone());                                  \
        for (const auto& [new_def, pair] : new_zregion_def) {                  \
            ZVariable new_def_var(new_def);                                    \
            knight_log(llvm::outs()                                            \
                       << #OP " for new def: " << new_def_var << "\n");        \
            zdom->assign_var(new_def_var, ZVariable(pair.first));              \
            zdom_cloned->assign_var(new_def_var, ZVariable(pair.second));      \
        }                                                                      \
        knight_log(llvm::outs() << "assigned new def zdom: ";                  \
                   zdom->dump(llvm::outs());                                   \
                   llvm::outs() << "\n");                                      \
        knight_log(llvm::outs() << "assigned new def zdom_cloned: ";           \
                   zdom_cloned->dump(llvm::outs());                            \
                   llvm::outs() << "\n");                                      \
                                                                               \
        zdom->OP(*zdom_cloned);                                                \
        delete zdom_cloned;                                                    \
        knight_log(llvm::outs() << "joined zdom: ";                            \
                   new_map[get_zdom_id()]->dump(llvm::outs());                 \
                   llvm::outs() << "\n");                                      \
    }                                                                          \
                                                                               \
    ConstraintSystem cst_system = m_constraint_system;                         \
    cst_system.retain(other->m_constraint_system);                             \
                                                                               \
    auto res = get_state_manager()                                             \
                   .get_persistent_state_with_copy_and_stateful_member_map(    \
                       *this,                                                  \
                       std ::move(new_map),                                    \
                       std ::move(region_defs),                                \
                       std ::move(stmt_sexpr),                                 \
                       std ::move(cst_system));                                \
    knight_log(llvm::outs() << #OP " state: " << *res << "\n");                \
    return res;

ProgramStateRef ProgramState::join(const ProgramStateRef& other,
                                   const LocationContext* loc_ctx) const {
    // UNION_MAP(join_with);
    DomValMap new_map;
    for (const auto& [other_id, other_val] : other->m_dom_val) {
        auto it = m_dom_val.find(other_id);
        if (it == m_dom_val.end()) {
            new_map[other_id] = other_val->clone_shared();
        } else {
            auto* new_val = it->second->clone();
            new_val->join_with(*other_val);
            new_map[other_id] = SharedVal(new_val);
        }
    }

    StmtSExprMap stmt_sexpr;
    llvm::for_each(m_stmt_sexpr, [&](const auto& pair) {
        const auto& [stmt, sexpr] = pair;
        auto it = other->m_stmt_sexpr.find(stmt);
        if (it == other->m_stmt_sexpr.end() || it->second == sexpr) {
            stmt_sexpr[stmt] = sexpr;
        }
    });

    llvm::for_each(other->m_stmt_sexpr, [&](const auto& pair) {
        const auto& [stmt, sexpr] = pair;
        if (!stmt_sexpr.contains(stmt)) {
            stmt_sexpr[stmt] = sexpr;
        }
    });

    RegionDefMap region_defs = m_region_defs;
    std::map< const RegionDef*,
              std::pair< const RegionDef*, const RegionDef* > >
        new_zregion_def;
    for (const auto [region_frame_pair, def] : other->m_region_defs) {
        auto it = region_defs.find(region_frame_pair);
        if (it == region_defs.end() || it->second == def) {
            region_defs[region_frame_pair] = def;
        } else {
            const auto& [region, _] = region_frame_pair;

            knight_log_nl(llvm::outs() << "loc ctx#" << loc_ctx << "\n";
                          loc_ctx->dump(llvm::outs()););
            knight_log_nl(llvm::outs() << "region#" << region << "\n";
                          region->dump(llvm::outs()););

            const auto* new_def =
                get_state_manager().m_symbol_mgr.get_region_def(region,
                                                                loc_ctx);

            if (region->get_value_type()->isIntegralOrEnumerationType()) {
                new_zregion_def[new_def] = {it->second, def};
                knight_log(
                    llvm::outs() << "prepare new def: " << new_def << " with "
                                 << it->second << " and " << def << "\n";
                    llvm::FoldingSetNodeID id;
                    new_def->Profile(id);
                    llvm::outs()
                    << "new region def id: " << id.ComputeHash() << "\n";
                    id.clear();
                    it->second->Profile(id);
                    llvm::outs()
                    << "this region def id: " << id.ComputeHash() << "\n";
                    id.clear();
                    def->Profile(id);
                    llvm::outs()
                    << "other region def id: " << id.ComputeHash() << "\n";);
            } else {
                knight_log(llvm::outs() << "unsupported region type: "
                                        << region->get_value_type() << "\n");
                knight_unreachable("unsupported region type");
            }
            region_defs[region_frame_pair] = new_def;

            knight_log(llvm::outs()
                       << "merged region `" << *region
                       << "` assigned new def: " << new_def << "\n");
        }
    }

    // Join region definitions
    if (!new_zregion_def.empty()) {
        auto* zdom =
            dynamic_cast< ZNumericalDomBase* >(new_map[get_zdom_id()].get());
        auto* zdom_cloned =
            dynamic_cast< ZNumericalDomBase* >(new_map[get_zdom_id()]->clone());
        for (const auto& [new_def, pair] : new_zregion_def) {
            ZVariable new_def_var(new_def);
            knight_log(llvm::outs()
                       << "join for new def: " << new_def_var << "\n");
            zdom->assign_var(new_def_var, ZVariable(pair.first));
            zdom_cloned->assign_var(new_def_var, ZVariable(pair.second));
        }
        knight_log(llvm::outs() << "assigned new def zdom: ";
                   zdom->dump(llvm::outs());
                   llvm::outs() << "\n");
        knight_log(llvm::outs() << "assigned new def zdom_cloned: ";
                   zdom_cloned->dump(llvm::outs());
                   llvm::outs() << "\n");

        zdom->join_with(*zdom_cloned);
        delete zdom_cloned;
        knight_log(llvm::outs() << "joined zdom: ";
                   new_map[get_zdom_id()]->dump(llvm::outs());
                   llvm::outs() << "\n");
    }

    ConstraintSystem cst_system = m_constraint_system;
    // TODO(constraint-join): handle constraint join more precisely.
    cst_system.retain(other->m_constraint_system);

    auto res = get_state_manager()
                   .get_persistent_state_with_copy_and_stateful_member_map(
                       *this,
                       std ::move(new_map),
                       std ::move(region_defs),
                       std ::move(stmt_sexpr),
                       std ::move(cst_system));
    knight_log(llvm::outs() << "joined state: " << *res << "\n");
    return res;
}

ProgramStateRef ProgramState::join_at_loop_head(
    const ProgramStateRef& other, const LocationContext* loc_ctx) const {
    UNION_MAP(join_with_at_loop_head);
}

ProgramStateRef ProgramState::join_consecutive_iter(
    const ProgramStateRef& other, const LocationContext* loc_ctx) const {
    UNION_MAP(join_consecutive_iter_with);
}

ProgramStateRef ProgramState::widen(const ProgramStateRef& other,
                                    const LocationContext* loc_ctx) const {
    UNION_MAP(widen_with);
}

ProgramStateRef ProgramState::widen_with_threshold(
    const ProgramStateRef& other,
    const LocationContext* loc_ctx,
    const ZNum& threshold) const {
    DomValMap new_map;
    for (const auto& [other_id, other_val] : other->m_dom_val) {
        auto it = m_dom_val.find(other_id);
        if (it == m_dom_val.end()) {
            new_map[other_id] = other_val->clone_shared();
        } else {
            auto* new_val = it->second->clone();
            if (auto* new_zval = llvm::dyn_cast< ZNumericalDomBase >(new_val)) {
                new_zval
                    ->widen_with_threshold(*(llvm::cast< ZNumericalDomBase >(
                                               other_val.get())),
                                           threshold);
            } else {
                new_val->widen_with(*other_val);
            }
            new_map[other_id] = SharedVal(new_val);
        }
    }

    StmtSExprMap stmt_sexpr;
    llvm::for_each(m_stmt_sexpr, [&](const auto& pair) {
        const auto& [stmt, sexpr] = pair;
        auto it = other->m_stmt_sexpr.find(stmt);
        if (it == other->m_stmt_sexpr.end() || it->second == sexpr) {
            stmt_sexpr[stmt] = sexpr;
        }
    });

    llvm::for_each(other->m_stmt_sexpr, [&](const auto& pair) {
        const auto& [stmt, sexpr] = pair;
        if (!stmt_sexpr.contains(stmt)) {
            stmt_sexpr[stmt] = sexpr;
        }
    });

    RegionDefMap region_defs = m_region_defs;
    std::map< const RegionDef*,
              std::pair< const RegionDef*, const RegionDef* > >
        new_zregion_def;
    for (const auto [region_frame_pair, def] : other->m_region_defs) {
        auto it = region_defs.find(region_frame_pair);
        if (it == region_defs.end() || it->second == def) {
            region_defs[region_frame_pair] = def;
        } else {
            const auto& [region, _] = region_frame_pair;

            knight_log_nl(llvm::outs() << "loc ctx#" << loc_ctx << "\n";
                          loc_ctx->dump(llvm::outs()););
            knight_log_nl(llvm::outs() << "region#" << region << "\n";
                          region->dump(llvm::outs()););

            const auto* new_def =
                get_state_manager().m_symbol_mgr.get_region_def(region,
                                                                loc_ctx);

            if (region->get_value_type()->isIntegralOrEnumerationType()) {
                new_zregion_def[new_def] = {it->second, def};
                knight_log(
                    llvm::outs() << "prepare new def: " << new_def << " with "
                                 << it->second << " and " << def << "\n";
                    llvm::FoldingSetNodeID id;
                    new_def->Profile(id);
                    llvm::outs()
                    << "new region def id: " << id.ComputeHash() << "\n";
                    id.clear();
                    it->second->Profile(id);
                    llvm::outs()
                    << "this region def id: " << id.ComputeHash() << "\n";
                    id.clear();
                    def->Profile(id);
                    llvm::outs()
                    << "other region def id: " << id.ComputeHash() << "\n";);
            } else {
                knight_log(llvm::outs() << "unsupported region type: "
                                        << region->get_value_type() << "\n");
                knight_unreachable("unsupported region type");
            }
            region_defs[region_frame_pair] = new_def;

            knight_log(llvm::outs()
                       << "widen_with_threshold region `" << *region
                       << "` assigned new def: " << new_def << "\n");
        }
    }

    // Join region definitions
    if (!new_zregion_def.empty()) {
        auto* zdom =
            dynamic_cast< ZNumericalDomBase* >(new_map[get_zdom_id()].get());
        auto* zdom_cloned =
            dynamic_cast< ZNumericalDomBase* >(new_map[get_zdom_id()]->clone());
        for (const auto& [new_def, pair] : new_zregion_def) {
            ZVariable new_def_var(new_def);
            knight_log(llvm::outs()
                       << "join for new def: " << new_def_var << "\n");
            zdom->assign_var(new_def_var, ZVariable(pair.first));
            zdom_cloned->assign_var(new_def_var, ZVariable(pair.second));
        }
        knight_log(llvm::outs() << "assigned new def zdom: ";
                   zdom->dump(llvm::outs());
                   llvm::outs() << "\n");
        knight_log(llvm::outs() << "assigned new def zdom_cloned: ";
                   zdom_cloned->dump(llvm::outs());
                   llvm::outs() << "\n");

        zdom->widen_with_threshold(*zdom_cloned, threshold);
        delete zdom_cloned;
        knight_log(llvm::outs() << "widen_with_threshold zdom: ";
                   new_map[get_zdom_id()]->dump(llvm::outs());
                   llvm::outs() << "\n");
    }

    ConstraintSystem cst_system = m_constraint_system;
    // TODO(constraint-join): handle constraint join more precisely.
    cst_system.retain(other->m_constraint_system);

    auto res = get_state_manager()
                   .get_persistent_state_with_copy_and_stateful_member_map(
                       *this,
                       std ::move(new_map),
                       std ::move(region_defs),
                       std ::move(stmt_sexpr),
                       std ::move(cst_system));
    knight_log(llvm::outs() << "widen_with_threshold state: " << *res << "\n");
    return res;
}

ProgramStateRef ProgramState::meet(const ProgramStateRef& other) const {
    DomValMap map;
    for (const auto& [other_id, other_val] : other->m_dom_val) {
        auto it = m_dom_val.find(other_id);
        if (it != m_dom_val.end()) {
            auto* new_val = it->second->clone();
            new_val->meet_with(*other_val);
            map[other_id] = SharedVal(new_val);
        }
    }
    return get_state_manager()
        .get_persistent_state_with_copy_and_dom_val_map(*this, std ::move(map));
}

ProgramStateRef ProgramState::narrow(const ProgramStateRef& other) const {
    DomValMap map;
    for (const auto& [other_id, other_val] : other->m_dom_val) {
        auto it = m_dom_val.find(other_id);
        if (it != m_dom_val.end()) {
            auto* new_val = it->second->clone();
            new_val->narrow_with(*other_val);
            map[other_id] = SharedVal(new_val);
        }
    }
    return get_state_manager()
        .get_persistent_state_with_copy_and_dom_val_map(*this, std ::move(map));
}

ProgramStateRef ProgramState::narrow_with_threshold(
    const ProgramStateRef& other, const ZNum& threshold) const {
    DomValMap map;
    for (const auto& [other_id, other_val] : other->m_dom_val) {
        auto it = m_dom_val.find(other_id);
        if (it != m_dom_val.end()) {
            auto* new_val = it->second->clone();
            if (auto* new_zval = llvm::dyn_cast< ZNumericalDomBase >(new_val)) {
                knight_log_nl(llvm::outs() << "before narrow with threshold :"
                                           << threshold << "\n"
                                           << *new_val << "\n";);
                new_zval
                    ->narrow_with_threshold(*(llvm::cast< ZNumericalDomBase >(
                                                other_val.get())),
                                            threshold);

                knight_log_nl(llvm::outs() << "after narrow with threshold :"
                                           << threshold << "\n"
                                           << *new_val << "\n";);
            } else {
                new_val->narrow_with(*other_val);
            }

            map[other_id] = SharedVal(new_val);
        }
    }
    return get_state_manager()
        .get_persistent_state_with_copy_and_dom_val_map(*this, std ::move(map));
}

bool ProgramState::leq(const ProgramState& other) const {
    knight_log_nl(llvm::outs() << "leq this state: " << *this << "\n"
                               << "leq other state: " << other << "\n");

    return llvm::none_of(m_dom_val, [&](const auto& pair) {
        const auto& [id, val] = pair;
        auto it = other.m_dom_val.find(id);
        if (it == other.m_dom_val.end()) {
            knight_log(llvm::outs() << "other has no domain: "
                                    << get_domain_name_by_id(id) << "\n");
            return !val->is_bottom();
        }
        return !val->leq(*(it->second));
    });
}

bool ProgramState::equals(const ProgramState& other) const {
    if (this == &other) {
        return true;
    }
    if (m_region_defs != other.m_region_defs) {
        return false;
    }

    bool need_to_check_other = other.m_dom_val.size() != this->m_dom_val.size();

    llvm::BitVector this_key_set(std::numeric_limits< DomID >::max());
    for (const auto& [id, val] : this->m_dom_val) {
        this_key_set.set(id);
        auto it = other.m_dom_val.find(id);
        if (it == other.m_dom_val.end()) {
            if (val->is_bottom()) {
                need_to_check_other = true;
                continue;
            }
            return false;
        }

        if (!val->equals(*(it->second))) {
            return false;
        }
    }

    if (need_to_check_other) {
        for (const auto& [id, val] : other.m_dom_val) {
            if (!this_key_set[id] && !val->is_bottom()) {
                return false;
            }
        }
    }
    return true;
}

void ProgramState::dump(llvm::raw_ostream& os) const {
    os << "State:{\n";

    os << "Regions: {";
    for (const auto& [region_frame_pair, def] : m_region_defs) {
        const auto [region, frame] = region_frame_pair;
        os << "\n";
        region->dump(os);
        os << " @" << frame << ": ";
        def->dump(os);
    }
    if (!m_region_defs.empty()) {
        os << "\n";
    }
    os << "},\n";

    os << "Stmts: {";
    for (const auto& [stmt_frame_pair, sexpr] : m_stmt_sexpr) {
        const auto [stmt, frame] = stmt_frame_pair;
        os << "\n(" << stmt->getStmtClassName() << "#"
           << stmt->getID(get_state_manager().m_region_mgr.get_ast_ctx())
           << ") ";
        stmt->printPretty(os,
                          nullptr,
                          get_region_manager()
                              .get_ast_ctx()
                              .getPrintingPolicy());

        os << " @" << frame << ": ";
        sexpr->dump(os);
    }
    if (!m_stmt_sexpr.empty()) {
        os << "\n";
    }
    os << "},\n";

    m_constraint_system.dump(os);

    os << "Domains: {";
    for (const auto& [id, aval] : m_dom_val) {
        os << "\n[" << get_domain_name_by_id(id) << "]: ";
        aval->dump(os);
    }
    if (!m_dom_val.empty()) {
        os << "\n";
    }
    os << "}\n";

    os << "}\n";
}

ProgramStateRef ProgramStateManager::get_default_state() {
    DomValMap dom_val;
    for (auto analysis_id : m_analysis_mgr.get_required_analyses()) {
        for (auto dom_id :
             m_analysis_mgr.get_registered_domains_in(analysis_id)) {
            if (auto default_fn = get_domain_default_val_fn(dom_id)) {
                dom_val[dom_id] = std::move((*default_fn)());
            }
        }
    }
    ProgramState state(this,
                       &m_region_mgr,
                       std::move(dom_val),
                       RegionDefMap{},
                       StmtSExprMap{},
                       ConstraintSystem{});

    return get_persistent_state(state);
}

ProgramStateRef ProgramStateManager::get_bottom_state() {
    DomValMap dom_val;
    for (auto analysis_id : m_analysis_mgr.get_required_analyses()) {
        for (auto dom_id :
             m_analysis_mgr.get_registered_domains_in(analysis_id)) {
            if (auto bottom_fn = get_domain_bottom_val_fn(dom_id)) {
                dom_val[dom_id] = std::move((*bottom_fn)());
            }
        }
    }
    ProgramState state(this,
                       &m_region_mgr,
                       std::move(dom_val),
                       RegionDefMap{},
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
                           state.m_region_defs,
                           state.m_stmt_sexpr,
                           state.m_constraint_system);
    return get_persistent_state(new_state);
}

ProgramStateRef ProgramStateManager::
    get_persistent_state_with_copy_and_region_defs_map(
        const ProgramState& state, RegionDefMap region_defs) {
    ProgramState new_state(state.m_state_mgr,
                           state.m_region_mgr,
                           state.m_dom_val,
                           std::move(region_defs),
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
                           state.m_region_defs,
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
                           state.m_region_defs,
                           state.m_stmt_sexpr,
                           std::move(cst_system));

    return get_persistent_state(new_state);
}

[[nodiscard]] ProgramStateRef ProgramStateManager::
    get_persistent_state_with_copy_and_stateful_member_map(
        const ProgramState& state,
        DomValMap dom_val,
        RegionDefMap region_defs,
        StmtSExprMap stmt_sexpr,
        ConstraintSystem system) {
    ProgramState new_state(state.m_state_mgr,
                           state.m_region_mgr,
                           std::move(dom_val),
                           std::move(region_defs),
                           std::move(stmt_sexpr),
                           std::move(system));

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

} // namespace knight::analyzer