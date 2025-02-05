//===- block_engine.cpp -----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the execution engine for the Basic Block.
//
//===------------------------------------------------------------------===//

#include "analyzer/core/engine/block_engine.hpp"
#include "analyzer/core/analysis/analysis_base.hpp"
#include "analyzer/core/proc_cfg.hpp"
#include "analyzer/core/program_state.hpp"
#include "common/util/assert.hpp"

#include <clang/Analysis/CFG.h>
#include <llvm/Support/Casting.h>

#define DEBUG_TYPE "block_engine"

namespace knight::analyzer {

void BlockExecutionEngine::exec() {
    ProgramStateRef state = m_state;

    state = exec_branch_condition(state);
    if (state->is_bottom()) {
        m_state = state;
        return;
    }
    for (const auto& elem : m_node->Elements) {
        m_current_elem_idx++;
        switch (elem.getKind()) {
            using enum clang::CFGElement::Kind;
            default:
                break;
            case Initializer: {
                const auto& init = elem.castAs< clang::CFGInitializer >();
                exec_cxx_ctor_initializer(init.getInitializer());
            } break;
            case ScopeBegin: {
                const auto& scope_begin = elem.castAs< clang::CFGScopeBegin >();
                exec_scope_begin(scope_begin.getTriggerStmt(),
                                 scope_begin.getVarDecl());
            } break;
            case ScopeEnd: {
                const auto& scope_end = elem.castAs< clang::CFGScopeEnd >();
                exec_scope_end(scope_end.getTriggerStmt(),
                               scope_end.getVarDecl());
            } break;
            case NewAllocator: {
                const auto& new_allocator =
                    elem.castAs< clang::CFGNewAllocator >();
                exec_new_allocator_call(new_allocator.getAllocatorExpr());
            } break;
            case LifetimeEnds: {
                const auto& lifetime_ends =
                    elem.castAs< clang::CFGLifetimeEnds >();
                exec_lifetime_ends(lifetime_ends.getTriggerStmt(),
                                   lifetime_ends.getVarDecl());
            } break;
            case LoopExit: {
                knight_unreachable( // NOLINT
                    "construct loop exit not implemented yet");
            } break;
            case Statement: {
                const auto& cfg_stmt = elem.castAs< clang::CFGStmt >();
                state = exec_cfg_stmt(cfg_stmt.getStmt(), state);
                if (state->is_bottom()) {
                    break;
                }
            } break;
            case Constructor: {
                knight_unreachable("constructor not implemented yet"); // NOLINT
            } break;
            case AutomaticObjectDtor:
            case DeleteDtor:
            case BaseDtor:
            case MemberDtor:
            case TemporaryDtor: {
                knight_unreachable("destructors not implemented yet"); // NOLINT
            } break;
            case CleanupFunction: {
                knight_unreachable( // NOLINT
                    "cleanup function not implemented yet");
            } break;
        }
    }
    m_state = state;
}

[[nodiscard]] const LocationContext* BlockExecutionEngine::
    get_location_context() const {
    return m_location_manager.create_location_context(m_frame,
                                                      m_current_elem_idx,
                                                      m_node);
}

// TODO(condition): only support successor size 2 for now

ProgramStateRef BlockExecutionEngine::exec_branch_condition(
    ProgramStateRef state) {
    if (NodeRef pred = ProcCFG::get_unique_pred(m_node);
        pred != nullptr && pred->succ_size() == 2U) {
        if (ExprRef cond = pred->getLastCondition()) {
            bool is_true_branch = *(pred->succ_begin()) == m_node;

            if (const auto* scalar_int = llvm::dyn_cast_or_null< ScalarInt >(
                    state->get_stmt_sexpr(cond, m_frame).value_or(nullptr))) {
                knight_log(llvm::outs() << "condition as scalar int: "
                                        << *scalar_int << "\n");
                if ((is_true_branch && scalar_int->get_value() == 0) ||
                    (!is_true_branch && scalar_int->get_value() != 1)) {
                    return state->get_state_manager().get_bottom_state();
                }
            }
            AnalysisContext analysis_ctx(m_analysis_manager.get_context(),
                                         m_analysis_manager
                                             .get_region_manager(),
                                         m_frame,
                                         m_sym_manager,
                                         get_location_context());
            analysis_ctx.set_state(state);
            m_analysis_manager
                .run_analyses_for_condition_filter(analysis_ctx,
                                                   cond,
                                                   is_true_branch);

            return analysis_ctx.get_state();
        }
    }
    return state;
}

/// \brief Transfer C++ base or member initializer from constructor's
/// initialization list.
void BlockExecutionEngine::exec_cxx_ctor_initializer(
    clang::CXXCtorInitializer* initializer) {
    // todo
}

/// \brief Transfer beginning of a scope implicitly generated
/// by the compiler on encountering a CompoundStmt
///
/// \param trigger_stmt The CompoundStmt that triggers the scope
/// begin transfer.
/// \param var_decl The variable declaration that triggers the scope
/// begin transfer.
void BlockExecutionEngine::exec_scope_begin(StmtRef trigger_stmt,
                                            VarDeclRef var_decl) {
    // todo
}

/// \brief Transfer ending of a scope implicitly generated by
/// the compiler after the last Stmt in a CompoundStmt's body
void BlockExecutionEngine::exec_scope_end(StmtRef trigger_stmt,
                                          VarDeclRef var_decl) {
    // todo
}

/// \brief Transfer C++ new allocator call
void BlockExecutionEngine::exec_new_allocator_call(
    const clang::CXXNewExpr* expr) {
    // todo
}

/// \brief Transfer the point where the lifetime of an automatic object ends
void BlockExecutionEngine::exec_lifetime_ends(StmtRef trigger_stmt,
                                              VarDeclRef var_decl) {
    // todo
}

/// \brief Transfer the stmt
ProgramStateRef BlockExecutionEngine::exec_cfg_stmt(
    StmtRef stmt, const ProgramStateRef& state) {
    AnalysisContext analysis_ctx(m_analysis_manager.get_context(),
                                 m_analysis_manager.get_region_manager(),
                                 m_frame,
                                 m_sym_manager,
                                 get_location_context());
    analysis_ctx.set_state(state);
    m_stmt_pre[stmt] = state;

    m_analysis_manager.run_analyses_for_pre_stmt(analysis_ctx, stmt);
    m_analysis_manager.run_analyses_for_eval_stmt(analysis_ctx, stmt);
    m_analysis_manager.run_analyses_for_post_stmt(analysis_ctx, stmt);

    auto post_state = analysis_ctx.get_state();
    m_stmt_post[stmt] = post_state;
    return std::move(post_state);
}

} // namespace knight::analyzer