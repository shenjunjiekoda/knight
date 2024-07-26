//===- cl_opts.hpp ----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the command line options for the knight tool.
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/Support/CommandLine.h>
#include <llvm/Option/OptTable.h>

#include <clang/Driver/Options.h>
#include <clang/Tooling/CommonOptionsParser.h>

namespace knight::cl_opts {

using namespace llvm;
using namespace clang::driver;
using namespace clang::tooling;

inline cl::OptionCategory knight_category("knight options");

inline cl::extrahelp common_help_msg(CommonOptionsParser::HelpMessage);
inline cl::extrahelp knight_help_desc(R"(
Knight is a static analysis tool that checks C/C++ code for vulnerabilities and coding errors.
)");

inline cl::desc desc(StringRef desc) {
    return cl::desc{desc.ltrim()};
}

inline cl::opt< std::string > analyses("analyses",
                                       desc(R"(
use `*` for all analyses enabled, and `-` prefix to disable 
analyses. You can provide a comma-separated glob list of 
checker names.
)"),
                                       cl::init(""),
                                       cl::cat(knight_category));

inline cl::opt< std::string > checkers("checkers",
                                       desc(R"(
use `*` for all checkers enabled, and `-` prefix to disable 
checkers. You can provide a comma-separated glob list of 
checker names.
)"),
                                       cl::init(""),
                                       cl::cat(knight_category));

inline cl::opt< bool > list_checkers("list-checkers",
                                     desc(R"(
list enabled checkers and exit program. Use with
-checkers=* to list all available checkers.
)"),
                                     cl::init(false),
                                     cl::cat(knight_category));

inline cl::opt< bool > list_analyses("list-analyses",
                                     desc(R"(
list enabled analyses and exit program. Use with
-analyses=* to list all available analyses.
)"),
                                     cl::init(false),
                                     cl::cat(knight_category));

inline cl::opt< std::string > overlay_file("overlay_file",
                                           desc(R"(
Overlay the vfs described by overlay file on the real fs.
)"),
                                           cl::value_desc("filename"),
                                           cl::cat(knight_category));

inline cl::opt< bool > use_color("use-color",
                                 desc(R"(
Use colors in output.
)"),
                                 cl::init(true),
                                 cl::cat(knight_category));

} // namespace knight::cl_opts