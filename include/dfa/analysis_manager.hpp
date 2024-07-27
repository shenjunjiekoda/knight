//===- analysis_manager.hpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the analysis manager class.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/analysis_context.hpp"
#include "dfa/domain/dom_base.hpp"
#include "dfa/proc_cfg.hpp"
#include "tooling/context.hpp"

#include <memory>
#include <unordered_set>

namespace knight::dfa {

class AnalysisBase;

using AnalysisID = uint8_t;
using AnalysisIDSet = std::unordered_set< AnalysisID >;
using AnalysisNameRef = llvm::StringRef;

template < typename T > class AnalysisCallBack;

template < typename RET, typename... Args >
class AnalysisCallBack< RET(Args...) > {
  public:
    using CallBack = RET (*)(AnalysisID, Args...);

  private:
    CallBack m_callback;
    AnalysisID m_id;

  public:
    AnalysisCallBack(AnalysisID id, CallBack callback)
        : m_id(id), m_callback(callback) {}
    RET operator()(Args... args) const { return m_callback(m_id, args...); }
}; // class AnalysisCallBack

namespace internal {

using ExitNodeRef = ProcCFG::NodeRef;
using StmtRef = ProcCFG::StmtRef;

using AnalyzeBeginFunctionCallBack = AnalysisCallBack< void(AnalysisContext&) >;

using AnalyzeEndFunctionCallBack =
    AnalysisCallBack< void(ExitNodeRef, AnalysisContext&) >;

using AnalyzeStmtCallBack = AnalysisCallBack< void(StmtRef, AnalysisContext&) >;
using MatchStmtCallBack = bool (*)(StmtRef S);
enum class VisitStmtKind { Pre, Eval, Post };
struct StmtAnalysisInfo {
    AnalyzeStmtCallBack anz_cb;
    MatchStmtCallBack match_cb;
    VisitStmtKind kind;
}; // struct StmtAnalysisInfo

} // namespace internal

/// \brief The analysis manager which holds all the registered analyses.
///
class AnalysisManager {
    friend class CheckerManager;

  private:
    KnightContext& m_ctx;

    /// \brief analysis context
    std::unique_ptr< AnalysisContext > m_analysis_ctx;

    /// \brief analyses
    std::unordered_set< AnalysisID > m_analyses; // all analyses
    std::unordered_map< AnalysisID, std::unordered_set< AnalysisID > >
        m_analysis_dependencies; // all analysis dependencies

    std::unordered_map< AnalysisID, std::unique_ptr< AnalysisBase > >
        m_enabled_analyses; // enabled analysis shall be created.
    std::unordered_set< AnalysisID >
        m_required_analyses; // all analyses should be created,
    // shall be equivalent with enabled analyses key set.

    /// \brief registered domains
    std::unordered_map< DomID, AnalysisID > m_domains;
    std::unordered_map< AnalysisID, std::unordered_set< DomID > >
        m_analysis_domains;

    /// \brief visit begin function callbacks
    std::vector< internal::AnalyzeBeginFunctionCallBack >
        m_begin_function_analyses;
    /// \brief visit end function callbacks
    std::vector< internal::AnalyzeEndFunctionCallBack > m_end_function_analyses;
    /// \brief visit statement callbacks
    std::vector< internal::StmtAnalysisInfo > m_stmt_analyses;

  public:
    AnalysisManager(KnightContext& ctx)
        : m_ctx(ctx), m_analysis_ctx(std::make_unique< AnalysisContext >(ctx)) {
    }

  public:
    /// \brief specialized analysis management
    ///
    /// Dependencies shall be handled before the registration.
    /// @{
    void add_required_analysis(AnalysisID id);
    void add_analysis_dependency(AnalysisID id, AnalysisID required_id);
    std::unordered_set< AnalysisID > get_analysis_dependencies(
        AnalysisID id) const;

    void enable_analysis(std::unique_ptr< AnalysisBase > analysis);
    bool is_analysis_required(AnalysisID id) const;

    std::optional< AnalysisBase* > get_analysis(AnalysisID id);
    /// @}

    /// \brief domain management
    ///
    /// Analysis shall be registered first.
    /// Domain dependencies shall be handled before the registration.
    /// @{
    void register_domain(AnalysisID analysis_id, DomID dom_id);
    std::unordered_set< DomID > get_registered_domains_in(AnalysisID id) const;
    /// @}

    /// \brief callback registrations
    /// @{
    void register_for_begin_function(internal::AnalyzeBeginFunctionCallBack cb);
    void register_for_end_function(internal::AnalyzeEndFunctionCallBack cb);
    void register_for_stmt(internal::AnalyzeStmtCallBack cb,
                           internal::MatchStmtCallBack match_cb,
                           internal::VisitStmtKind kind);
    /// @}

    void compute_all_required_analyses_by_dependencies();
}; // class AnalysisManager

} // namespace knight::dfa
