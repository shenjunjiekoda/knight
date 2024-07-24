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

#include <unordered_set>

namespace knight::dfa {

class AnalysisBase;

using AnalysisID = uint8_t;
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
  public:
    AnalysisManager(AnalysisContext& ctx) : m_ctx(ctx) {}

  public:
    /// \brief specialized analysis management
    ///
    /// Dependencies shall be handled before the registration.
    /// @{
    void register_analysis(std::unique_ptr< AnalysisBase > analysis);
    std::optional< AnalysisBase* > get_analysis(AnalysisID id);
    void add_analysis_dependency(AnalysisID id, AnalysisID required_id);
    std::unordered_set< AnalysisID > get_analysis_dependencies(
        AnalysisID id) const;
    /// @}

    /// \brief domain management
    ///
    /// Analysis shall be registered first.
    /// Domain dependencies shall be handled before the registration.
    /// @{
    void register_domain(AnalysisID analysis_id, DomID dom_id);
    std::unordered_set< DomID > get_registered_domains_in(AnalysisID id) const;
    void add_dom_dependency(DomID id, DomID required_id);
    /// @}

    /// \brief callback registrations
    /// @{
    void register_for_begin_function(internal::AnalyzeBeginFunctionCallBack cb);
    void register_for_end_function(internal::AnalyzeEndFunctionCallBack cb);
    void register_for_stmt(internal::AnalyzeStmtCallBack cb,
                           internal::MatchStmtCallBack match_cb,
                           internal::VisitStmtKind kind);
    /// @}
  private:
    bool is_analysis_registered(AnalysisID id) const;

  private:
    /// \brief analysis context
    AnalysisContext& m_ctx;

    /// \brief registered analyses
    std::unordered_map< AnalysisID, std::unique_ptr< AnalysisBase > >
        m_analyses;
    std::unordered_set< AnalysisID > m_required_analyses;
    std::unordered_map< AnalysisID, std::unordered_set< AnalysisID > >
        m_analysis_dependencies;

    /// \brief registered domains
    std::unordered_map< DomID, AnalysisID > m_domains;
    std::unordered_map< AnalysisID, std::unordered_set< DomID > >
        m_analysis_domains;
    std::unordered_map< DomID, std::unordered_set< DomID > > m_dom_dependencies;

    /// \brief visit begin function callbacks
    std::vector< internal::AnalyzeBeginFunctionCallBack >
        m_begin_function_analyses;
    /// \brief visit end function callbacks
    std::vector< internal::AnalyzeEndFunctionCallBack > m_end_function_analyses;
    /// \brief visit statement callbacks
    std::vector< internal::StmtAnalysisInfo > m_stmt_analyses;

}; // class AnalysisManager

} // namespace knight::dfa
