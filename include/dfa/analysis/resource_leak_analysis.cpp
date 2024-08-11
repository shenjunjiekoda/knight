#include "dfa/analysis/resource_leak_analysis.hpp"
#include "dfa/analysis_context.hpp"
#include "dfa/program_state.hpp"
#include "llvm/Support/raw_ostream.h"

namespace knight::dfa {

void ResourceLeakAnalysis::analyze_end_function(AnalysisContext& ctx) const {
    llvm::outs() << "ResourceLeakAnalysis::analyze_end_function()\n";
    // TODO: for the actual resource leak detection logic.
}

} // namespace knight::dfa
