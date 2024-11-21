//===- main.cpp -------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This is the main file of the knight-analyzer tool.
//
//===------------------------------------------------------------------===//

#include "analyzer/core/analyzer_options.hpp"
#include "analyzer/tooling/cl_opts.hpp"
#include "analyzer/tooling/context.hpp"
#include "analyzer/tooling/diagnostic.hpp"
#include "analyzer/tooling/driver.hpp"
#include "analyzer/tooling/options.hpp"
#include "common/util/log.hpp"
#include "common/util/vfs.hpp"

#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/WithColor.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdint>
#include <string>

#define DEBUG_TYPE "main"

using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace knight;
using namespace knight::cl_opts;

using ErrCode = uint8_t;
constexpr ErrCode NormalExit = 0U;
constexpr ErrCode OptParseFailure = 1U;
constexpr ErrCode BaseVfsCreateFailure = 2U;
constexpr ErrCode VfsFileFailure = 3U;
constexpr ErrCode NoInputFiles = 4U;
constexpr ErrCode InputNotExists = 5U;
constexpr ErrCode CompileErrorFound = 6U;

llvm::IntrusiveRefCntPtr< llvm::vfs::OverlayFileSystem > get_vfs(
    ErrCode& code) {
    auto base_vfs = fs::create_base_vfs();
    if (!base_vfs) {
        code = BaseVfsCreateFailure;
        return nullptr;
    }

    if (!overlay_file.empty()) {
        if (auto vfs = fs::get_vfs_from_yaml(overlay_file, base_vfs)) {
            base_vfs->pushOverlay(std::move(vfs));
        } else {
            code = VfsFileFailure;
            return nullptr;
        }
    }
    code = NormalExit;
    return base_vfs;
}

analyzer::AnalyzerOptions forward_to_knight_analyzer_options(
    int argc, const char** argv) {
    (void)cl::ParseCommandLineOptions(argc, argv, "knight analyzer options");

    return analyzer::AnalyzerOptions{widening_delay, max_unrolling_iterations};
}

/// \brief  Resolve -Xc options
analyzer::AnalyzerOptions get_analyzer_options() {
    std::vector< const char* > analyzer_argv;
    analyzer_argv.push_back("dummy");
    int analyzer_argc = 0;

    // Dummy program name
    for (const auto& arg : knight::cl_opts::XcArgs) {
        analyzer_argv.push_back(arg.c_str());
    }
    analyzer_argv.push_back(nullptr); // Null-terminate the array
    analyzer_argc = static_cast< int >(analyzer_argv.size()) - 1;

    knight_log_nl(
        llvm::WithColor::note()
            << "Forwarding the following arguments to the target mechanism:\n";
        for (int i = 1; i < analyzer_argc;
             ++i) { llvm::WithColor::note() << analyzer_argv[i] << "\n"; });

    return forward_to_knight_analyzer_options(analyzer_argc,
                                              analyzer_argv.data());
}

std::unique_ptr< KnightOptionsProvider > get_opts_provider() {
    auto opts_provider = std::make_unique< KnightOptionsCommandLineProvider >();
    if (analyses.getNumOccurrences() > 0) {
        opts_provider->options.analyses = analyses; // NOLINT
    }
    if (checkers.getNumOccurrences() > 0) {
        opts_provider->options.checkers = checkers; // NOLINT
    }
    if (use_color.getNumOccurrences() > 0) {
        opts_provider->options.use_color = use_color;
    } else {
        opts_provider->options.use_color =
            llvm::sys::Process::StandardOutHasColors();
    }
    if (view_cfg.getNumOccurrences() > 0) {
        opts_provider->options.view_cfg = view_cfg;
    }
    if (dump_cfg.getNumOccurrences() > 0) {
        opts_provider->options.dump_cfg = dump_cfg;
    }
    if (XcArgs.getNumOccurrences() > 0) {
        opts_provider->options.analyzer_opts = get_analyzer_options();
    }

    return std::move(opts_provider);
}

// std::vector< std::string > get_enabled_checkers() {
//     KnightContext context(std::move(get_opts_provider()));
//     const KnightASTConsumerFactory factory(context);
//     std::vector< std::string > checkers;
//     for (const auto& [_, name] : factory.get_enabled_checks()) {
//         checkers.push_back(name.str());
//     }
//     return checkers;
// }

// std::vector< std::string > get_directly_enabled_analyses() {
//     KnightContext context(std::move(get_opts_provider()));
//     const KnightASTConsumerFactory factory(context);
//     std::vector< std::string > analyses;
//     for (const auto& [_, name] : factory.get_directly_enabled_analyses()) {
//         analyses.push_back(name.str());
//     }
//     return analyses;
// }

bool ensure_input_files_exist(const std::vector< std::string >& files,
                              const fs::FileSystemRef& vfs) {
    return llvm::all_of(files, [&vfs](const auto& file) {
        bool res = vfs->exists(fs::make_absolute(file));
        if (!res) {
            llvm::WithColor::error()
                << "Errors: input `" << file << "` not found!\n";
        }
        return res;
    });
}

void print_enabled_checkers(
    const std::vector< std::string >& enabled_checkers) {
    auto size = enabled_checkers.size();
    if (size == 0) {
        llvm::WithColor::error() << "No checkers are enabled.\n";
    } else {
        llvm::WithColor::error() << "\n* enabled" << size << " checker"
                                 << (size > 1 ? "s" : "") << ":\n";
        auto i = 0;
        for (const auto& checker : enabled_checkers) {
            llvm::WithColor::error() << "(" << ++i << ") " << checker << "\n";
        }
    }
}

void print_enabled_analyses(
    const std::vector< std::string >& enabled_analyses) {
    auto size = enabled_analyses.size();
    if (size == 0) {
        llvm::WithColor::error() << "No analyses are enabled.\n";
    } else {
        llvm::WithColor::error()
            << "\n* enabled " << size << " "
            << (size > 1 ? "analyses" : "analysis") << ":\n";
        auto i = 0;
        for (const auto& analysis : enabled_analyses) {
            llvm::WithColor::error() << "(" << ++i << ") " << analysis << "\n";
        }
    }
}

int main(int argc, const char** argv) {
    const llvm::InitLLVM llvm_setup(argc, argv);
    ErrCode code = NormalExit;
    auto base_vfs = get_vfs(code);
    if (!base_vfs) {
        return code;
    }

    auto opts_parser = CommonOptionsParser::create(argc,
                                                   argv,
                                                   knight_category,
                                                   cl::ZeroOrMore);

    if (!opts_parser) {
        WithColor::error() << toString(opts_parser.takeError());
        return OptParseFailure;
    }

    auto opts_provider = get_opts_provider();
    auto input_path = std::string("dummy");
    auto src_path_lst = opts_parser->getSourcePathList();
    if (!src_path_lst.empty()) {
        input_path = fs::make_absolute(src_path_lst.front());
    }

    auto opts = opts_provider->get_options_for(input_path);

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();

    // auto enabled_checkers = get_enabled_checkers();
    //     auto enabled_analyses = get_directly_enabled_analyses();
    //     if (list_checkers || list_analyses) {
    //         if (list_checkers) {
    //             print_enabled_checkers(enabled_checkers);
    //         }
    //         if (list_analyses) {
    //             print_enabled_analyses(enabled_analyses);
    //         }
    //         return NormalExit;
    //     }

    if (src_path_lst.empty()) {
        llvm::WithColor::error() << "No input files provided.\n";
        llvm::cl::PrintHelpMessage(false, true);
        return NoInputFiles;
    }

    llvm::transform(src_path_lst, src_path_lst.begin(), fs::make_absolute);
    if (!ensure_input_files_exist(src_path_lst, base_vfs)) {
        return InputNotExists;
    }

    //     if (enabled_analyses.empty() && enabled_checkers.empty()) {
    //         llvm::WithColor::error() << "No analyses or checkers are
    //         enabled.\n"; return NormalExit;
    //     }

    KnightContext ctx(std::move(opts_provider));
    KnightDriver driver(ctx,
                        opts_parser->getCompilations(),
                        src_path_lst,
                        base_vfs);
    const auto& diags = driver.run();
    driver.handle_diagnostics(diags, try_fix);

    if (const bool compile_error_found =
            llvm::any_of(diags, [](const auto& diag) {
                return diag.DiagLevel == KnightDiagnostic::Error;
            })) {
        llvm::WithColor::error().changeColor(llvm::raw_ostream::Colors::RED,
                                             true);
        llvm::WithColor::error() << "Error: compilation failed.\n";
        llvm::WithColor::error().resetColor();
        return CompileErrorFound;
    }

    return code;
}