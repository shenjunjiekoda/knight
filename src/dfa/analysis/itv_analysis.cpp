
//===- intervcal_analysis.cpp -----------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements an interval analysis
//
//===------------------------------------------------------------------===//

#include "dfa/analysis/itv_analysis.hpp"
#include "dfa/constraint/linear.hpp"
#include "dfa/domain/interval_dom.hpp"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "ItvAnalysis"

namespace knight::dfa {

void ItvAnalysis::analyze_begin_function(
    [[maybe_unused]] AnalysisContext& ctx) const {
    LLVM_DEBUG(llvm::outs() << "ItvAnalysis::analyze_begin_function()\nstate:";
               ctx.get_state()->dump(llvm::outs());
               llvm::outs() << "\n";);
}

void ItvAnalysis::post_analyze_stmt(const clang::ReturnStmt* return_stmt,
                                    AnalysisContext& ctx) const {
    LLVM_DEBUG(llvm::outs() << "ItvAnalysis::pre_analyze ReturnStmt: \n";
               return_stmt->dumpColor();
               llvm::outs() << "\n";);
    const auto* ret_val = return_stmt->getRetValue();
    if (ret_val == nullptr ||
        !ret_val->getType()->isIntegralOrEnumerationType()) {
        return;
    }
    auto state = ctx.get_state();
    SExprRef ret_val_sexpr =
        state->get_stmt_sexpr_or_conjured(ret_val,
                                          ctx.get_current_stack_frame());
    if (auto znum = ret_val_sexpr->get_as_znum()) {
        LLVM_DEBUG(llvm::outs() << "return znum: " << *znum << "\n";);
        return;
    }
    auto zvar = ret_val_sexpr->get_as_zvariable();
    if (!zvar) {
        return;
    }

    LLVM_DEBUG(llvm::outs() << "return zvar: " << *zvar << "\n";
               if (auto zitv_dom_opt = state->get_ref< ZIntervalDom >()) {
                   const ZIntervalDom* zitv_dom = zitv_dom_opt.value();
                   llvm::outs() << "zinterval dom: ";
                   zitv_dom->dump(llvm::outs());
                   llvm::outs() << "\n";
                   llvm::outs() << "return zitv: ";
                   zitv_dom->get_value(*zvar).dump(llvm::outs());
                   llvm::outs() << "\n";
               });
}

} // namespace knight::dfa