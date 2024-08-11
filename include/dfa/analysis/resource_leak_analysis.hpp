#pragma once

#include "dfa/analysis/analysis_base.hpp"
#include "dfa/analysis_context.hpp"
#include "dfa/region/region.hpp"
#include "tooling/context.hpp"
#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>
#include <llvm/Support/raw_ostream.h>

namespace knight::dfa {

class ResourceLeakAnalysis : public Analysis< ResourceLeakAnalysis,
                                              analyze::BeginFunction,
                                              analyze::EndFunction > {
  public:
    explicit ResourceLeakAnalysis(KnightContext& ctx) : Analysis(ctx) {}

    [[nodiscard]] static AnalysisKind get_kind() {
        return AnalysisKind::ResourceLeakAnalysis;
    }

    void analyze_begin_function([[maybe_unused]] AnalysisContext& ctx) const {
        llvm::outs() << "ResourceLeakAnalysis::analyze_begin_function()\n";
    }

    void analyze_end_function([[maybe_unused]] AnalysisContext& ctx) const {
        llvm::outs() << "ResourceLeakAnalysis::analyze_end_function()\n";
        // TODO: the logic to check for resource leaks at the end of the function.
    }
};

} // namespace knight::dfa
