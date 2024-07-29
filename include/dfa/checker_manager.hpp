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
#include "dfa/checker_context.hpp"
#include "dfa/domain/dom_base.hpp"
#include "dfa/proc_cfg.hpp"
#include "tooling/context.hpp"

#include <memory>
#include <unordered_set>

namespace knight::dfa {

class CheckerBase;

using CheckerID = uint8_t;
using CheckerIDSet = std::unordered_set< CheckerID >;
using CheckerNameRef = llvm::StringRef;

template < typename T > class CheckerCallBack;

template < typename RET, typename... Args >
class CheckerCallBack< RET(Args...) > {
  public:
    using CallBack = RET (*)(CheckerID, Args...);

  private:
    CallBack m_callback;
    CheckerID m_id;

  public:
    CheckerCallBack(CheckerID id, CallBack callback)
        : m_id(id), m_callback(callback) {}
    RET operator()(Args... args) const { return m_callback(m_id, args...); }
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
struct StmtCheckerInfo {
    CheckStmtCallBack anz_cb;
    MatchStmtCallBack match_cb;
    CheckStmtKind kind;
}; // struct StmtCheckerInfo

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

  public:
    /// \brief specialized checker management
    ///
    /// Each checker may depend on some analyses.
    /// Dependencies shall be handled before the registration.
    /// @{
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

}; // class CheckerManager

} // namespace knight::dfa
