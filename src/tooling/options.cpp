#include "tooling/options.hpp"
#include "util/assert.hpp"

#include <llvm/Support/Process.h>

namespace knight {

const char* optionSourceToString(OptionSource source) {
    switch (source) {
        case OptionSource::Default:
            return "Default";
        case OptionSource::CommandLine:
            return "CommandLine";
        case OptionSource::ConfigFile:
            return "ConfigFile";
    }
    knight_unreachable("invalid option source"); // NOLINT
}

KnightOptionsDefaultProvider::KnightOptionsDefaultProvider() {
    if (auto user_opt = llvm::sys::Process::GetEnv("USER")) {
        options.user = *user_opt;
    } else if (auto user_opt = llvm::sys::Process::GetEnv("USERNAME")) {
        options.user = *user_opt;
    }

    set_default_options();
}

OptionSource KnightOptionsDefaultProvider::get_checker_option_source(
    [[maybe_unused]] const std::string& option) const {
    return OptionSource::Default;
}

void KnightOptionsDefaultProvider::set_default_options() {
    // Set default checker options here.
}

void KnightOptionsDefaultProvider::set_checker_option(const std::string& option,
                                                      CheckerOptVal value) {
    options.check_opts[option] = value;
}

KnightOptions KnightOptionsDefaultProvider::get_options_for(
    [[maybe_unused]] const std::string& file) const {
    return options;
}

OptionSource KnightOptionsCommandLineProvider::get_checker_option_source(
    const std::string& option) const {
    if (m_cmd_override_opts.contains(option)) {
        return OptionSource::CommandLine;
    }
    return OptionSource::Default;
}

void KnightOptionsCommandLineProvider::set_checker_option(
    const std::string& option, CheckerOptVal value) {
    options.check_opts[option] = value;
    m_cmd_override_opts.insert(option);
}

KnightOptionsConfigFileProvider::KnightOptionsConfigFileProvider(
    [[maybe_unused]] std::string config_file) { // NOLINT
    // TODO(config): Implement this.
}

} // namespace knight