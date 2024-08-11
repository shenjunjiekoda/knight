#pragma once

#include <string>
#include <vector>

namespace knight {

class ClangTidyRunner {
public:
    ClangTidyRunner();
    std::vector<std::string> run(const std::string& file);

private:
    // Clang-Tidy configuration
};

} // namespace knight
