//===- cl_opts.hpp ----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the command line options for the knight cg tool.
//
//===------------------------------------------------------------------===//

#pragma once

#include <clang/Driver/Options.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Option/OptTable.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

namespace knight::cg::cl_opts {

using namespace llvm;
using namespace clang::driver;
using namespace clang::tooling;

// NOLINTBEGIN(readability-identifier-naming,cert-err58-cpp,cppcoreguidelines-avoid-non-const-global-variables,cppcoreguidelines-interfaces-global-init,fuchsia-statically-constructed-objects)

inline cl::OptionCategory knight_cg_category("knight cg options");

inline cl::extrahelp common_help_msg(CommonOptionsParser::HelpMessage);
inline cl::extrahelp knight_help_desc(R"(
Welcome to Knight!

Knight is a static analysis tool that checks C/C++ code for vulnerabilities and coding errors.
)");

inline cl::desc desc(StringRef desc) {
    return cl::desc{desc.ltrim()};
}

inline cl::opt< std::string > overlay_file("overlay_file",
                                           desc(R"(
Overlay the vfs described by overlay file on the real fs.
)"),
                                           cl::value_desc("filename"),
                                           cl::cat(knight_cg_category));

inline cl::opt< std::string > knight_dir("dir",
                                         desc(R"(
The directory to store the knight intermediate files.
)"),
                                         cl::value_desc("directory"),
                                         cl::cat(knight_cg_category));

inline cl::opt< unsigned > db_busy_timeout("db-busy-timeout",
                                           desc(R"(
The timeout for waiting for the database to become available.
)"),
                                           cl::value_desc("milliseconds"),
                                           cl::init(5000),
                                           cl::cat(knight_cg_category));

inline cl::opt< bool > use_color("use-color",
                                 desc(R"(
Use colors in output.
)"),
                                 cl::init(true),
                                 cl::cat(knight_cg_category));

inline cl::opt< bool > show_process_sys_function("show-process-sys-function",
                                                 desc(R"(
Show processing system functions.
)"),
                                                 cl::init(false),
                                                 cl::cat(knight_cg_category));

inline cl::opt< bool > skip_system_header("skip-system-header",
                                          desc(R"(
Skip system header files.
)"),
                                          cl::init(false),
                                          cl::cat(knight_cg_category));

inline cl::opt< bool > skip_implicit_code("skip-implicit-code",
                                          desc(R"(
Skip implicit code.
)"),
                                          cl::init(false),
                                          cl::cat(knight_cg_category));

// NOLINTEND(readability-identifier-naming,cert-err58-cpp,cppcoreguidelines-avoid-non-const-global-variables,cppcoreguidelines-interfaces-global-init,fuchsia-statically-constructed-objects)

} // namespace knight::cg::cl_opts