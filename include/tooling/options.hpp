//===- options.hpp ----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the knight options used by the knight library and
//  command-line tools.
//
//===------------------------------------------------------------------===//

#pragma once

#include <optional>
#include <string>
#include <set>
#include <variant>
#include <map>

namespace knight {

enum class OptionSource { Default, CommandLine, ConfigFile };

const char* optionSourceToString(OptionSource source);

using CheckerOptVal = std::variant< bool, std::string, int >;

/// \brief Knight options.
struct KnightOptions {
    /// \brief Checks filter.
    std::optional< std::string > checks = "";

    /// \brief Header file extensions.
    std::set< std::string > header_extensions = {"h", "hh", "hpp", "hxx"};

    /// \brief Implematation file extensions.
    std::set< std::string > impl_extensions = {"c", "cc", "cpp", "cxx"};

    /// \brief checker specific options
    std::map< std::string, CheckerOptVal > check_opts{};

    /// \brief the user using knight
    std::string user = "unknown";

    /// \brief use color in output
    bool use_color = false;
}; // struct KnightOptions

struct KnightOptionsProvider {
    virtual ~KnightOptionsProvider() = default;

    virtual OptionSource get_checker_option_source(
        const std::string& option) const = 0;

    virtual KnightOptions get_options_for(const std::string& file) const = 0;

    virtual void set_checker_option(const std::string& option,
                                    CheckerOptVal value) = 0;
}; // struct KnightOptionsProvider

struct KnightOptionsDefaultProvider : KnightOptionsProvider {
    KnightOptions options;

    KnightOptionsDefaultProvider();

    OptionSource get_checker_option_source(
        const std::string& option) const override;

    KnightOptions get_options_for(const std::string& file) const override;

    void set_checker_option(const std::string& option,
                            CheckerOptVal value) override;

  protected:
    void set_default_options();

}; // struct KnightOptionsDefaultProvider

struct KnightOptionsCommandLineProvider : KnightOptionsDefaultProvider {
    KnightOptionsCommandLineProvider() = default;

    OptionSource get_checker_option_source(
        const std::string& option) const override;

    void set_checker_option(const std::string& option,
                            CheckerOptVal value) override;

  protected:
    std::set< std::string > m_cmd_override_opts;
}; // class KnightOptionsCommandLineProvider

// TODO: add config file support
class KnightOptionsConfigFileProvider
    : public KnightOptionsCommandLineProvider {
  public:
    KnightOptionsConfigFileProvider(std::string config_file);
};

} // namespace knight