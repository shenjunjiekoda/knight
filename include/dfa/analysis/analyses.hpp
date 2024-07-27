//===- analyses.hpp ---------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the analysis metadatas.
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/ADT/StringRef.h>

#ifdef ANALYSIS_DEF
#    undef ANALYSIS_DEF
#endif

namespace knight::dfa {

enum class AnalysisKind {
    None,
#define ANALYSIS_DEF(KIND, NAME, ID, DESC) KIND,
#include "analyses.def"
}; // enum class AnalysisKind

inline llvm::StringRef get_analysis_name(AnalysisKind kind) {
    switch (kind) {
#undef ANALYSIS_DEF
#define ANALYSIS_DEF(KIND, NAME, ID, DESC)                                     \
    case AnalysisKind::KIND:                                                   \
        return NAME;
#include "analyses.def"
    default:
        return "Unknown";
    }
}

inline llvm::StringRef get_analysis_desc(AnalysisKind kind) {
    switch (kind) {
#undef ANALYSIS_DEF
#define ANALYSIS_DEF(KIND, NAME, ID, DESC)                                     \
    case AnalysisKind::KIND:                                                   \
        return DESC;
#include "analyses.def"
    default:
        return "Unknown";
    }
}

inline uint8_t get_analysis_id(AnalysisKind kind) {
    switch (kind) {
#undef ANALYSIS_DEF
#define ANALYSIS_DEF(KIND, NAME, ID, DESC)                                     \
    case AnalysisKind::KIND:                                                   \
        return ID;
#include "analyses.def"
    default:
        return 0;
    }
}

inline AnalysisKind get_analysis_kind(uint8_t id) {
    switch (id) {
#undef ANALYSIS_DEF
#define ANALYSIS_DEF(KIND, NAME, ID, DESC)                                     \
    case ID:                                                                   \
        return AnalysisKind::KIND;
#include "analyses.def"
    default:
        return AnalysisKind::None;
    }
}

inline llvm::StringRef get_analysis_name_by_id(uint8_t id) {
    return get_analysis_name(get_analysis_kind(id));
}

} // namespace knight::dfa
