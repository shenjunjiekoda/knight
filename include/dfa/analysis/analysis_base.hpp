//===- analysis_base.hpp ----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the base for all data flow analyses.
//
//===------------------------------------------------------------------===//

#pragma once

#include "dfa/analysis_context.hpp"
#include "dfa/analysis_manager.hpp"
#include "dfa/analysis/analyses.hpp"
#include "support/clang_ast.hpp"

namespace knight::dfa {

/// \brief Base class for all data flow analyses.
///
/// derived checker can add analysis dependencies by implementing
/// the following function:
/// \code
///     static void add_dependencies(AnalysisManager& mgr) {}
/// \endcode
class AnalysisBase {
  public:
    AnalysisBase(KnightContext& ctx);
    virtual ~AnalysisBase() = default;

    virtual AnalysisKind get_kind() const = 0;
    virtual bool is_language_supported(
        const clang::LangOptions& lang_opts) const {
        return true;
    }

  protected:
    clang::DiagnosticBuilder diagnose(
        clang::SourceLocation loc,
        llvm::StringRef info,
        clang::DiagnosticIDs::Level diag_level = clang::DiagnosticIDs::Warning);

    clang::DiagnosticBuilder diagnose(
        llvm::StringRef info,
        clang::DiagnosticIDs::Level diag_level = clang::DiagnosticIDs::Warning);

    clang::DiagnosticBuilder diagnose(
        clang::DiagnosticIDs::Level diag_level = clang::DiagnosticIDs::Warning);

    KnightContext& get_knight_context() { return m_ctx; }

  private:
    KnightContext& m_ctx;
}; // class AnalysisBase

template < typename ANALYSIS1, typename... ANALYSES >
class Analysis : public ANALYSIS1, public ANALYSES..., public AnalysisBase {
  public:
    Analysis< ANALYSIS1, ANALYSES... >(KnightContext& ctx)
        : AnalysisBase(ctx) {}

    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* checker, AnalysisManager& mgr) {
        ANALYSIS1::register_callback(checker, mgr);
        Analysis< ANALYSES... >::register_callback(checker, mgr);
    }
};

template < typename ANALYSIS1 >
class Analysis< ANALYSIS1 > : public ANALYSIS1, public AnalysisBase {
  public:
    Analysis< ANALYSIS1 >(KnightContext& ctx) : AnalysisBase(ctx) {}

    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* checker, AnalysisManager& mgr) {
        ANALYSIS1::register_callback(checker, mgr);
    }
};

//=------------------- Callback Registration -------------------=//

namespace analyze {

class BeginFunction {
    template < typename ANALYSIS >
    static void run_begin_function(void* analysis, AnalysisContext& C) {
        ((const ANALYSIS*)analysis)->analyze_begin_function(C);
    }

  public:
    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_begin_function(
            internal::AnalyzeBeginFunctionCallBack(analysis,
                                                   run_begin_function<
                                                       ANALYSIS >));
    }
}; // class BeginFunction

class EndFunction {

    template < typename ANALYSIS >
    static void run_end_function(void* analysis, AnalysisContext& C) {
        ((const ANALYSIS*)analysis)->analyze_end_function(C);
    }

  public:
    template < typename ANALYSIS >
    static void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_end_function(
            internal::AnalyzeEndFunctionCallBack(analysis,
                                                 run_end_function< ANALYSIS >));
    }

}; // class EndFunction

template < clang_stmt STMT > class PreStmt {
    template < typename ANALYSIS >
    static void run_pre_stmt(void* analysis,
                             internal::StmtRef S,
                             AnalysisContext& C) {
        ((const ANALYSIS*)analysis)->pre_analyze_stmt(S, C);
    }

    static bool is_interesting_stmt(internal::StmtRef S) {
        return isa< STMT >(S);
    }

  public:
    template < typename ANALYSIS >
    void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_stmt(internal::AnalyzeStmtCallBack(analysis,
                                                            run_pre_stmt<
                                                                ANALYSIS >),
                              is_interesting_stmt,
                              internal::VisitStmtKind::Pre);
    }

}; // class PreStmt

template < clang_stmt STMT > class EvalStmt {
    template < typename ANALYSIS >
    static void run_eval_stmt(void* analysis,
                              internal::StmtRef S,
                              AnalysisContext& C) {
        ((const ANALYSIS*)analysis)->analyze_stmt(S, C);
    }

    static bool is_interesting_stmt(internal::StmtRef S) {
        return isa< STMT >(S);
    }

  public:
    template < typename ANALYSIS >
    void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_stmt(internal::AnalyzeStmtCallBack(analysis,
                                                            run_eval_stmt<
                                                                ANALYSIS >),
                              is_interesting_stmt,
                              internal::VisitStmtKind::Eval);
    }
}; // class EvalStmt

template < clang_stmt STMT > class PostStmt {
    template < typename ANALYSIS >
    static void run_post_stmt(void* analysis,
                              internal::StmtRef S,
                              AnalysisContext& C) {
        ((const ANALYSIS*)analysis)->post_analyze_stmt(S, C);
    }

    static bool is_interesting_stmt(internal::StmtRef S) {
        return isa< STMT >(S);
    }

  public:
    template < typename ANALYSIS >
    void register_callback(ANALYSIS* analysis, AnalysisManager& mgr) {
        mgr.register_for_stmt(internal::AnalyzeStmtCallBack(analysis,
                                                            run_post_stmt<
                                                                ANALYSIS >),
                              is_interesting_stmt,
                              internal::VisitStmtKind::Post);
    }
}; // class PostStmt

} // namespace analyze

} // namespace knight::dfa
