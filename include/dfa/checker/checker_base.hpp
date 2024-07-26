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

#include "dfa/checker_context.hpp"
#include "dfa/checker_manager.hpp"
#include "support/clang_ast.hpp"

namespace knight::dfa {

/// \brief Base class for all data flow checkers.
class CheckerBase {
  public:
    virtual ~CheckerBase() = default;

    virtual CheckerNameRef get_name() const = 0;
    virtual CheckerID get_id() const = 0;
    virtual bool is_language_supported(
        const clang::LangOptions& lang_opts) const {
        return true;
    }
    virtual void add_dependencies(CheckerManager& mgr) const {}

}; // class CheckerBase

template < typename CHECKER1, typename... CHECKERS >
class Checker : public CHECKER1, public CHECKERS..., public CheckerBase {
  public:
    template < typename CHECKER >
    static void register_callback(CHECKER* checker, CheckerManager& mgr) {
        CHECKER1::register_callback(checker, mgr);
        Checker< CHECKERS... >::register_callback(checker, mgr);
    }
};

template < typename CHECKER1 >
class Checker< CHECKER1 > : public CHECKER1, public CheckerBase {
  public:
    template < typename CHECKER >
    static void register_callback(CHECKER* checker, CheckerManager& mgr) {
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
            internal::CheckBeginFunctionCallBack(checker,
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
            internal::CheckEndFunctionCallBack(checker,
                                               run_end_function< CHECKER >));
    }

}; // class EndFunction

template < clang_stmt STMT > class PreStmt {
    template < typename CHECKER >
    static void run_pre_stmt(void* checker,
                             internal::StmtRef S,
                             CheckerContext& C) {
        ((const CHECKER*)checker)->pre_check_stmt(S, C);
    }

    static bool is_interesting_stmt(internal::StmtRef S) {
        return isa< STMT >(S);
    }

  public:
    template < typename CHECKER >
    void register_callback(CHECKER* checker, CheckerManager& mgr) {
        mgr.register_for_stmt(internal::CheckStmtCallBack(checker,
                                                          run_pre_stmt<
                                                              CHECKER >),
                              is_interesting_stmt,
                              internal::CheckStmtKind::Pre);
    }

}; // class PreStmt

template < clang_stmt STMT > class PostStmt {
    template < typename CHECKER >
    static void run_post_stmt(void* checker,
                              internal::StmtRef S,
                              CheckerContext& C) {
        ((const CHECKER*)checker)->post_check_stmt(S, C);
    }

    static bool is_interesting_stmt(internal::StmtRef S) {
        return isa< STMT >(S);
    }

  public:
    template < typename CHECKER >
    void register_callback(CHECKER* checker, CheckerManager& mgr) {
        mgr.register_for_stmt(internal::CheckStmtCallBack(checker,
                                                          run_post_stmt<
                                                              CHECKER >),
                              is_interesting_stmt,
                              internal::CheckStmtKind::Post);
    }
}; // class PostStmt

} // namespace check

} // namespace knight::dfa
