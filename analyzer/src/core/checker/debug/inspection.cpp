//===- inspection.cpp -------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements a inspection checker for debugging purpose.
//
//===------------------------------------------------------------------===//

#include "analyzer/core/checker/debug/inspection.hpp"
#include "common/util/log.hpp"

#include <string>

#define DEBUG_TYPE "inspection-checker"

namespace knight::analyzer {

namespace {

constexpr unsigned ZValStrMaxLen = 64U;

} // anonymous namespace

void InspectionChecker::post_check_stmt(const clang::CallExpr* call_expr,
                                        CheckerContext& ctx) const {
    const auto* callee_fn = call_expr->getDirectCallee();
    if (callee_fn == nullptr || callee_fn->getIdentifier() == nullptr) {
        return;
    }
    auto name = callee_fn->getName();
    if (name == ZValDumper) {
        dump_zval(call_expr->getArg(0), ctx);
    } else if (name == ReachabilityDumper) {
        dump_reachability(call_expr, ctx);
    }
}

void InspectionChecker::dump_zval(const clang::Expr* expr,
                                  CheckerContext& ctx) const {
    knight_log_nl(llvm::outs() << "dump zval: "; expr->dumpColor();
                  llvm::outs() << "\n";);

    auto state = ctx.get_state();
    knight_log_nl(llvm::outs() << "state: "; state->dump(llvm::outs()));

    if (expr == nullptr || !expr->getType()->isIntegralOrEnumerationType()) {
        return;
    }
    auto zdom = state->get_zdom_ref();
    if (!zdom) {
        return;
    }
    if (zdom.value()->is_bottom()) {
        diagnose(expr->getExprLoc(), "⊥");
        return;
    }

    auto sexpr = state->get_stmt_sexpr(expr, ctx.get_current_stack_frame());
    if (!sexpr) {
        if (auto reg = state->get_region(expr, ctx.get_current_stack_frame())) {
            sexpr = state->get_region_def(*reg, ctx.get_current_stack_frame());
        }
        if (!sexpr) {
            diagnose(expr->getExprLoc(), "T");
            return;
        }
    }

    knight_log_nl(llvm::outs() << "sexpr: "; sexpr.value()->dump(llvm::outs());
                  llvm::outs() << "\n";);

    if (auto znum = sexpr.value()->get_as_znum()) {
        knight_log(llvm::outs() << "return znum: " << *znum << "\n";);
        return;
    }

    auto zvar = sexpr.value()->get_as_zvariable();
    if (!zvar) {
        return;
    }

    knight_log_nl(llvm::outs() << "zvar: " << *zvar << "\n";);

    auto zitv = zdom.value()->to_interval(*zvar);
    knight_log_nl(llvm::outs() << "zdom: "; zdom.value()->dump(llvm::outs());
                  llvm::outs() << "\n";
                  llvm::outs() << "zitv: ";
                  zitv.dump(llvm::outs());
                  llvm::outs() << "\n";);
    llvm::SmallString< ZValStrMaxLen > zval_str;
    llvm::raw_svector_ostream os(zval_str);
    zitv.dump(os);
    if (!zval_str.empty()) {
        diagnose(expr->getExprLoc(), zval_str);
    }
}

void InspectionChecker::dump_reachability(const clang::CallExpr* call_expr,
                                          CheckerContext& ctx) const {
    knight_log_nl(llvm::outs() << "dump reachability: ";
                  call_expr->getBeginLoc().dump(ctx.get_source_manager());
                  llvm::outs() << "\n";);

    auto state = ctx.get_state();
    knight_log_nl(llvm::outs() << "state: "; state->dump(llvm::outs()));

    diagnose(call_expr->getExprLoc(),
             state->is_bottom() ? "Unreachable" : "Reachable");
}

} // namespace knight::analyzer