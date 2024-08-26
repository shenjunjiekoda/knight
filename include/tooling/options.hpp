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

#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include "dfa/domain/domains.hpp"
#include "dfa/domain/interval_dom.hpp"

namespace knight {

enum class OptionSource { Default, CommandLine, ConfigFile };

const char* option_src_to_string(OptionSource source);

using CheckerOptVal = std::variant< bool, std::string, int >;
using Extensions = std::set< std::string >;

/// \brief Knight options.
struct KnightOptions {
    /// \brief Checkers filter.
    std::string checkers;

    /// \brief Analyses filter.
    std::string analyses;

    /// \brief ZNumerical domain.
    dfa::DomainKind zdom = dfa::DomainKind::ZIntervalDomain;

    /// \brief QNumerical domain.
    dfa::DomainKind qdom = dfa::DomainKind::QIntervalDomain;

    /// \brief Header file extensions.
    Extensions header_extensions = {"h", "hh", "hpp", "hxx"};

    /// \brief Implematation file extensions.
    Extensions impl_extensions = {"c", "cc", "cpp", "cxx"};

    /// \brief checker specific options
    std::map< std::string, CheckerOptVal > check_opts{};

    /// \brief the user using knight
    std::string user = "unknown";

    /// \brief use color in output
    bool use_color = false;

    /// \brief view control flow graph
    bool view_cfg = false;

    /// \brief dump control flow graph
    bool dump_cfg = false;
}; // struct KnightOptions

struct KnightOptionsProvider {
    virtual ~KnightOptionsProvider() = default;

    [[nodiscard]] virtual OptionSource get_checker_option_source(
        const std::string& option) const = 0;

    [[nodiscard]] virtual KnightOptions get_options_for(
        const std::string& file) const = 0;

    virtual void set_checker_option(const std::string& option,
                                    CheckerOptVal value) = 0;
}; // struct KnightOptionsProvider

struct KnightOptionsDefaultProvider : KnightOptionsProvider {
  public:
    KnightOptions options;

  public:
    KnightOptionsDefaultProvider();

    [[nodiscard]] OptionSource get_checker_option_source(
        const std::string& option) const override;

    [[nodiscard]] KnightOptions get_options_for(
        const std::string& file) const override;

    void set_checker_option(const std::string& option,
                            CheckerOptVal value) override;

  protected:
    void set_default_options();

}; // struct KnightOptionsDefaultProvider

struct KnightOptionsCommandLineProvider : KnightOptionsDefaultProvider {
  protected:
    std::set< std::string > m_cmd_override_opts;

  public:
    KnightOptionsCommandLineProvider() = default;

    [[nodiscard]] OptionSource get_checker_option_source(
        const std::string& option) const override;

    void set_checker_option(const std::string& option,
                            CheckerOptVal value) override;

}; // class KnightOptionsCommandLineProvider

// TODO: add config file support
class KnightOptionsConfigFileProvider
    : public KnightOptionsCommandLineProvider {
  public:
    explicit KnightOptionsConfigFileProvider(std::string config_file);
};

} // namespace knight