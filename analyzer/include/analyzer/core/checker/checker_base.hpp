//===- checker_base.hpp -----------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the base for all property checkers.
//
//===------------------------------------------------------------------===//

#pragma once

#include "analyzer/core/checker/checkers.hpp"
#include "analyzer/core/checker_context.hpp"
#include "analyzer/core/checker_manager.hpp"
#include "analyzer/support/clang_ast.hpp"
namespace knight::analyzer {

/// \brief Base class for all data flow checkers.
///
/// Derived class can inherit like this:
/// \code
/// class MyCheckerImpl :
///    public Checker<MyCheckerImpl,
///        check::BeginFunction,
///        check::EndFunction,
///        (more callbacks accepted...) > {
///
///    /// \brief delegate to the Checker constructor
///    MyCheckerImpl(KnightContext& ctx, (more args...))
///         : Checker(ctx) {
///       // ...
///    }
///
///    /// \brief tell us the kind of checker.
///    static CheckerKind get_kind() {
///        return CheckerKind::MyChecker;
///    }
///
///    /// \brief Derived checker can add checker dependencies by
///    /// implementing the following function:
///    static void add_dependencies(CheckerManager& mgr) {
///        // add dependencies here
///    }
///
///    /// \brief BeginFunction callback.
///    void check_begin_function(CheckerContext& C) const {
///        // check begin function here.
///    }
///
///    /// \brief EndFunction callback.
///    void check_end_function(CheckerContext& C) const {
///        // check end function here.
///    }
///
///    // add more checker callbacks functions here.
///
///    /// \brief register your checker to manager here.
///    static UniqueCheckerRef register_checker(CheckerManager& mgr,
///                                             KnightContext& ctx) {
///        return mgr.register_checker< MyCheckerImpl >(ctx, (more ctor
///        args...));
///    }
///
/// }; // class MyCheckerImpl

class CheckerBase {
  public:
    CheckerKind kind;

  public:
    CheckerBase(KnightContext& ctx, CheckerKind k);
    virtual ~CheckerBase() = default;
    virtual bool is_language_supported(
        const clang::LangOptions& lang_opts) const {
        return true;
    }

  protected:
    clang::DiagnosticBuilder diagnose(clang::SourceLocation loc,
                                      llvm::StringRef info,
                                      clang::DiagnosticIDs::Level diag_level =
                                          clang::DiagnosticIDs::Warning) const;

    clang::DiagnosticBuilder diagnose(llvm::StringRef info,
                                      clang::DiagnosticIDs::Level diag_level =
                                          clang::DiagnosticIDs::Warning) const;

    clang::DiagnosticBuilder diagnose(clang::DiagnosticIDs::Level diag_level =
                                          clang::DiagnosticIDs::Warning) const;

    [[nodiscard]] KnightContext& get_knight_context() { return m_ctx; }

  private:
    KnightContext& m_ctx;
}; // class CheckerBase

template < typename Impl, typename CHECKER1, typename... CHECKERS >
class Checker : public CHECKER1, public CHECKERS..., public CheckerBase {
  public:
    Checker(KnightContext& ctx) : CheckerBase(ctx, Impl::get_kind()) {}

    static void register_callback(Impl* checker, CheckerManager& mgr) {
        CHECKER1::register_callback(checker, mgr);
        Checker< Impl, CHECKERS... >::register_callback(checker, mgr);
    }
};

template < typename Impl, typename CHECKER1 >
class Checker< Impl, CHECKER1 > : public CHECKER1, public CheckerBase {
  public:
    Checker(KnightContext& ctx) : CheckerBase(ctx, Impl::get_kind()) {}

    static void register_callback(Impl* checker, CheckerManager& mgr) {
        CHECKER1::register_callback(checker, mgr);
    }
};

//=------------------- Callback Registration -------------------=//

namespace check {

class BeginFunction {
    template < typename CHECKER >
    static void run_begin_function(void* checker, CheckerContext& C) {
        ((const CHECKER*)checker)->check_begin_function(C);
    }

  public:
    template < typename CHECKER >
    static void register_callback(CHECKER* checker, CheckerManager& mgr) {
        mgr.register_for_begin_function(
            internal::CheckBeginFunctionCallBack(CHECKER::get_kind(),
                                                 checker,
                                                 run_begin_function<
                                                     CHECKER >));
    }
}; // class BeginFunction

class EndFunction {
    template < typename CHECKER >
    static void run_end_function(void* checker, CheckerContext& C) {
        ((const CHECKER*)checker)->check_end_function(C);
    }

  public:
    template < typename CHECKER >
    static void register_callback(CHECKER* checker, CheckerManager& mgr) {
        mgr.register_for_end_function(
            internal::CheckEndFunctionCallBack(CHECKER::get_kind(),
                                               checker,
                                               run_end_function< CHECKER >));
    }

}; // class EndFunction

template < clang_stmt STMT >
class PreStmt {
    template < typename CHECKER >
    static void run_pre_stmt(void* checker,
                             internal::StmtRef S,
                             CheckerContext& C) {
        ((const CHECKER*)checker)->pre_check_stmt(dyn_cast< STMT >(S), C);
    }

    static bool is_interesting_stmt(internal::StmtRef S) {
        return isa< STMT >(S);
    }

  public:
    template < typename CHECKER >
    static void register_callback(CHECKER* checker, CheckerManager& mgr) {
        mgr.register_for_stmt(internal::CheckStmtCallBack(CHECKER::get_kind(),
                                                          checker,
                                                          run_pre_stmt<
                                                              CHECKER >),
                              is_interesting_stmt,
                              internal::CheckStmtKind::Pre);
    }

}; // class PreStmt

template < clang_stmt STMT >
class PostStmt {
    template < typename CHECKER >
    static void run_post_stmt(void* checker,
                              internal::StmtRef S,
                              CheckerContext& C) {
        ((const CHECKER*)checker)->post_check_stmt(dyn_cast< STMT >(S), C);
    }

    static bool is_interesting_stmt(internal::StmtRef S) {
        return isa< STMT >(S);
    }

  public:
    template < typename CHECKER >
    static void register_callback(CHECKER* checker, CheckerManager& mgr) {
        mgr.register_for_stmt(internal::CheckStmtCallBack(CHECKER::get_kind(),
                                                          checker,
                                                          run_post_stmt<
                                                              CHECKER >),
                              is_interesting_stmt,
                              internal::CheckStmtKind::Post);
    }
}; // class PostStmt

} // namespace check

} // namespace knight::analyzer
