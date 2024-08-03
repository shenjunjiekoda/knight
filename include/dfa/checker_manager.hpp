//===- checker_manager.hpp --------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the checker manager class.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/analysis_manager.hpp"
#include "dfa/checker/checkers.hpp"
#include "dfa/checker_context.hpp"
#include "dfa/proc_cfg.hpp"
#include "tooling/context.hpp"

#include <memory>
#include <unordered_set>

namespace knight::dfa {

class CheckerBase;

using UniqueCheckerRef = std::unique_ptr< CheckerBase >;
using CheckerRef = CheckerBase*;
using CheckerRefs = std::vector< CheckerRef >;

using CheckerIDSet = std::unordered_set< CheckerID >;
using CheckerNameRef = llvm::StringRef;

template < typename T > class CheckerCallBack;

template < typename RET, typename... Args >
class CheckerCallBack< RET(Args...) > {
  public:
    using CallBack = RET (*)(void*, Args...);

  private:
    CallBack m_callback;
    CheckerBase* m_checker;
    CheckerKind m_kind;

  public:
    CheckerCallBack(CheckerKind kind, CheckerBase* checker, CallBack callback)
        : m_kind(kind), m_checker(checker), m_callback(callback) {}

    CheckerID get_id() const { return get_checker_id(m_kind); }

    RET operator()(Args... args) const {
        return m_callback(m_checker, args...);
    }
}; // class CheckerCallBack

namespace internal {

using ExitNodeRef = ProcCFG::NodeRef;
using StmtRef = ProcCFG::StmtRef;

using CheckBeginFunctionCallBack = CheckerCallBack< void(CheckerContext&) >;

using CheckEndFunctionCallBack =
    CheckerCallBack< void(ExitNodeRef, CheckerContext&) >;

using CheckStmtCallBack = CheckerCallBack< void(StmtRef, CheckerContext&) >;
using MatchStmtCallBack = bool (*)(StmtRef S);
enum class CheckStmtKind { Pre, Post };
constexpr unsigned StmtCheckerInfoAlign = 64;
struct StmtCheckerInfo {
    CheckStmtCallBack anz_cb;
    MatchStmtCallBack match_cb;
    CheckStmtKind kind;
} __attribute__((aligned(StmtCheckerInfoAlign)))
__attribute__((packed)); // struct StmtCheckerInfo

} // namespace internal

/// \brief The checker manager which holds all the registered analyses.
///
class CheckerManager {
  private:
    /// \brief knight context
    KnightContext& m_ctx;

    /// \brief checker context
    std::unique_ptr< CheckerContext > m_checker_ctx;

    /// \brief analysis manager
    AnalysisManager& m_analysis_mgr;

    /// \brief checkers
    CheckerIDSet m_checkers; // all checkers
    std::unordered_map< CheckerID, std::unique_ptr< CheckerBase > >
        m_enabled_checkers;
    CheckerIDSet m_required_checkers;

    /// \brief visit begin function callbacks
    std::vector< internal::CheckBeginFunctionCallBack > m_begin_function_checks;
    /// \brief visit end function callbacks
    std::vector< internal::CheckEndFunctionCallBack > m_end_function_checks;
    /// \brief visit statement callbacks
    std::vector< internal::StmtCheckerInfo > m_stmt_checks;

  public:
    CheckerManager(KnightContext& ctx, AnalysisManager& analysis_mgr)
        : m_ctx(ctx), m_checker_ctx(std::make_unique< CheckerContext >(ctx)),
          m_analysis_mgr(analysis_mgr) {}

    CheckerContext& get_checker_context() const { return *m_checker_ctx; }

  public:
    /// \brief specialized checker management
    ///
    /// Each checker may depend on some analyses.
    /// Dependencies shall be handled before the registration.
    /// @{
    template < typename CHECKER, typename... AT >
    UniqueCheckerRef register_checker(KnightContext& ctx, AT&&... Args) {
        CheckerID id = get_checker_id(CHECKER::get_kind());
        if (m_checkers.contains(id)) {
            llvm::errs() << get_checker_name_by_id(id)
                         << " checker is already registered.\n";
        } else {
            m_checkers.insert(id);
        }

        auto checker =
            std::make_unique< CHECKER >(ctx, std::forward< AT >(Args)...);
        CHECKER::register_callback(checker.get(), *this);
        return std::move(checker);
    }

    void add_required_checker(CheckerID id);
    bool is_checker_required(CheckerID id) const;

    void enable_checker(std::unique_ptr< CheckerBase > checker);
    std::optional< CheckerBase* > get_checker(CheckerID id);

    template < typename Checker, typename Analysis >
    void add_checker_dependency() {
        CheckerID id = get_checker_id(Checker::get_kind());
        AnalysisID required_analysis_id = get_analysis_id(Analysis::get_kind());
        m_analysis_mgr.add_required_analysis(required_analysis_id);
    }
    /// @}

    /// \brief callback registrations
    /// @{
    void register_for_begin_function(internal::CheckBeginFunctionCallBack cb);
    void register_for_end_function(internal::CheckEndFunctionCallBack cb);
    void register_for_stmt(internal::CheckStmtCallBack cb,
                           internal::MatchStmtCallBack match_cb,
                           internal::CheckStmtKind kind);
    /// @}

    const std::vector< internal::CheckBeginFunctionCallBack >&
    begin_function_checks() const {
        return m_begin_function_checks;
    }

    const std::vector< internal::CheckEndFunctionCallBack >&
    end_function_checks() const {
        return m_end_function_checks;
    }

    const std::vector< internal::StmtCheckerInfo >& stmt_checks() const {
        return m_stmt_checks;
    }

    void run_checkers_for_stmt(internal::StmtRef stmt,
                               internal::CheckStmtKind check_kind);
    void run_checkers_for_pre_stmt(internal::StmtRef stmt);
    void run_checkers_for_post_stmt(internal::StmtRef stmt);
    void run_checkers_for_begin_function();
    void run_checkers_for_end_function(ProcCFG::NodeRef node);

}; // class CheckerManager

} // namespace knight::dfa
