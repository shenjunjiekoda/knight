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

#include "dfa/checker/debug/inspection.hpp"
#include <string>
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "inspection-checker"

namespace knight::dfa {

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
    }
}

void InspectionChecker::dump_zval(const clang::Expr* expr,
                                  CheckerContext& ctx) const {
    LLVM_DEBUG(llvm::outs() << "dump zval: "; expr->dumpColor();
               llvm::outs() << "\n";);

    auto state = ctx.get_state();
    LLVM_DEBUG(llvm::outs() << "state: "; state->dump(llvm::outs()));

    if (expr == nullptr || !expr->getType()->isIntegralOrEnumerationType()) {
        return;
    }

    auto sexpr = state->get_stmt_sexpr(expr);
    if (!sexpr) {
        if (auto reg = state->get_region(expr, ctx.get_current_stack_frame())) {
            // ctx.get_symbol_manager().get_region_sym_val(const TypedRegion
            // *typed_region, const LocationContext *loc_ctx)
        }
    }

    LLVM_DEBUG(llvm::outs() << "sexpr: "; sexpr.value()->dump(llvm::outs());
               llvm::outs() << "\n";);

    if (auto znum = sexpr.value()->get_as_znum()) {
        LLVM_DEBUG(llvm::outs() << "return znum: " << *znum << "\n";);
        return;
    }

    auto zvar = sexpr.value()->get_as_zvariable();
    if (!zvar) {
        return;
    }

    LLVM_DEBUG(llvm::outs() << "zvar: " << *zvar << "\n";);
    if (auto zitv_dom_opt = state->get_ref< ZIntervalDom >()) {
        const ZIntervalDom* zitv_dom = zitv_dom_opt.value();
        LLVM_DEBUG(llvm::outs() << "zinterval dom: ";
                   zitv_dom->dump(llvm::outs());
                   llvm::outs() << "\n";
                   llvm::outs() << "zitv: ";
                   zitv_dom->get_value(*zvar).dump(llvm::outs());
                   llvm::outs() << "\n";);
        llvm::SmallString< ZValStrMaxLen > zval_str;
        llvm::raw_svector_ostream os(zval_str);
        zitv_dom->get_value(*zvar).dump(os);
        if (!zval_str.empty()) {
            diagnose(expr->getExprLoc(), zval_str);
        }
    }
}

} // namespace knight::dfa