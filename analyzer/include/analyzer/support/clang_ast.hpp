//===- clang_ast.hpp --------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the clang AST concept and utility.
//
//===------------------------------------------------------------------===//

#pragma once

#include <clang/AST/Stmt.h>
#include "clang/AST/ASTDumper.h"
#include "common/support/dumpable.hpp"
#include "llvm/Support/raw_ostream.h"

#include <concepts>

namespace knight {

template < typename STMT >
struct is_clang_stmt : std::is_base_of< clang::Stmt, STMT > {}; // NOLINT
template < typename DECL >
struct is_clang_decl : std::is_base_of< clang::Decl, DECL > {}; // NOLINT

template < typename STMT >
concept clang_stmt = is_clang_stmt< STMT >::value;

template < typename DECL >
concept clang_decl = is_clang_decl< DECL >::value;

template < clang_stmt T >
struct DumpableTrait< T* > {
    static void dump(llvm::raw_ostream& os, T* stmt) {
        clang::ASTDumper dumper(os, /*ShowColors=*/false);
        dumper.Visit(stmt);
    }
};

template < clang_stmt T >
struct is_dumpable< T > : std::true_type {};

} // namespace knight