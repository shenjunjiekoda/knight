//===- globs.hpp ------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements the utility functions for globs.
//
//===------------------------------------------------------------------===//

#include "util/globs.hpp"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallString.h>

namespace knight {

namespace {

llvm::Regex create_regex_for(llvm::StringRef Glob) {
    llvm::SmallString< 256 > pattern("^");
    for (char c : Glob) {
        if (c == '*') {
            pattern.append(".*");
        } else if (std::ispunct(c)) {
            pattern.append("\\");
            pattern.push_back(c);
        } else {
            pattern.push_back(c);
        }
    }

    pattern.push_back('$');
    return llvm::Regex(pattern.str());
}

} // anonymous namespace

Globs::Globs(llvm::StringRef globs) {
    while (!globs.empty()) {
        bool isNegative = globs.consume_front("-");
        auto current = globs.split(',').first.trim();
        if (!current.empty()) {
            Glob g{isNegative, create_regex_for(current)};
            m_globs.push_back(std::move(g));
        }
        globs = globs.split(',').second;
    }
}

bool Globs::matches(llvm::StringRef str) const {
    auto catch_it = m_cache.find(str);
    if (catch_it != m_cache.end()) {
        return catch_it->second;
    }

    bool res = false;
    for (const auto& g : llvm::reverse(m_globs)) {
        if (g.regex.match(str)) {
            res = !g.is_negative;
            break;
        }
    }

    return m_cache[str] = res;
}

} // namespace knight