//===- builder.cpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the core builder for the Knight cg tool.
//
//===------------------------------------------------------------------===//

#include "cg/core/builder.hpp"
#include "cg/core/cg.hpp"
#include "cg/db/db.hpp"
#include "cg/tooling/driver.hpp"
#include "common/util/clang.hpp"
#include "common/util/log.hpp"
#include "common/util/vfs.hpp"
#include "llvm/Support/raw_ostream.h"

#include <clang/AST/Decl.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/Support/Casting.h>

#define DEBUG_TYPE "cg-builder"

namespace knight::cg {

namespace {

using namespace clang;

bool visit_call(const Decl* decl,
                const SourceLocation& loc,
                const std::string& cur_function_name,
                bool skip_system_header,
                Database& db) {
    auto& sm = decl->getASTContext().getSourceManager();
    const auto* named_decl = llvm::dyn_cast_or_null< clang::NamedDecl >(decl);
    if (named_decl == nullptr) {
        return true;
    }

    if (skip_system_header && sm.isInSystemHeader(loc)) {
        return true;
    }

    auto presumed_loc = sm.getPresumedLoc(named_decl->getLocation());
    if (presumed_loc.isInvalid()) {
        return true;
    }

    auto line = presumed_loc.getLine();
    auto col = presumed_loc.getColumn();
    auto file = knight::fs::make_absolute(presumed_loc.getFilename());

    auto callee_name = knight::clang_util::get_mangled_name(named_decl);
    CallSite cs(line, col, cur_function_name, callee_name);
    db.insert_callsite(cs);

    knight_log_nl(llvm::outs() << "find callsite: " << cs.to_string());

    return true;
}

} // anonymous namespace

bool CGBuilder::VisitCallExpr(const clang::CallExpr* call) {
    return visit_call(call->getCalleeDecl(),
                      call->getExprLoc(),
                      m_current_function_name,
                      m_ctx.skip_system_header,
                      m_db);
}

bool CGBuilder::VisitCXXConstructExpr(
    const clang::CXXConstructExpr* ctor_call) {
    return visit_call(ctor_call->getConstructor(),
                      ctor_call->getExprLoc(),
                      m_current_function_name,
                      m_ctx.skip_system_header,
                      m_db);
}

bool CGBuilder::VisitFunctionDecl(const clang::FunctionDecl* function) {
    auto& sm = function->getASTContext().getSourceManager();
    if (m_ctx.skip_system_header &&
        sm.isInSystemHeader(function->getLocation())) {
        return true;
    }

    if (!function->hasBody() || !function->isThisDeclarationADefinition()) {
        return true;
    }

    auto mangled_name = knight::clang_util::get_mangled_name(function);
    m_current_function_name = mangled_name;
    auto name = function->getQualifiedNameAsString() + " " +
                function->getType().getAsString();

    auto presumed_loc = sm.getPresumedLoc(function->getLocation());
    if (presumed_loc.isInvalid()) {
        return true;
    }

    auto line = presumed_loc.getLine();
    auto col = presumed_loc.getColumn();
    auto file = knight::fs::make_absolute(presumed_loc.getFilename());

    CallGraphNode node(line, col, name, mangled_name, file);
    m_db.insert_cg_node(node);

    knight_log_nl(llvm::outs() << "find cg node: " << node.to_string());

    return true;
}

CGBuilder::CGBuilder(knight::CGContext& ctx)
    : m_ctx(ctx),
      m_db(ctx.knight_dir, static_cast< int >(ctx.db_busy_timeout)) {}

bool CGBuilder::shouldVisitImplicitCode() const { // NOLINT
    return m_ctx.skip_implicit_code;
}

} // namespace knight::cg