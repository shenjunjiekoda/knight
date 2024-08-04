//===- main.cpp -------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This is the main file of the knight tool.
//
//===------------------------------------------------------------------===//

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/WithColor.h>
#include <llvm/Support/raw_ostream.h>
#include <cstdint>

#include <clang/Tooling/CommonOptionsParser.h>
#include <string>

#include "tooling/cl_opts.hpp"
#include "tooling/context.hpp"
#include "tooling/diagnostic.hpp"
#include "tooling/knight.hpp"
#include "tooling/options.hpp"
#include "util/vfs.hpp"

using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace knight;
using namespace knight::cl_opts;

using ErrCode = uint8_t;
constexpr ErrCode NormalExit = 0;
constexpr ErrCode OptParseFailure = 1;
constexpr ErrCode BaseVfsCreateFailure = 2;
constexpr ErrCode VfsFileFailure = 3;
constexpr ErrCode NoInputFiles = 4;
constexpr ErrCode CompileErrorFound = 5;

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
    return std::move(opts_provider);
}

std::vector< std::string > get_enabled_checkers() {
    KnightContext context(std::move(get_opts_provider()));
    const KnightASTConsumerFactory factory(context);
    std::vector< std::string > checkers;
    for (const auto& [_, name] : factory.get_enabled_checks()) {
        checkers.push_back(name.str());
    }
    return checkers;
}

std::vector< std::string > get_directly_enabled_analyses() {
    KnightContext context(std::move(get_opts_provider()));
    const KnightASTConsumerFactory factory(context);
    std::vector< std::string > analyses;
    for (const auto& [_, name] : factory.get_directly_enabled_analyses()) {
        analyses.push_back(name.str());
    }
    return analyses;
}

void print_enabled_checkers(
    const std::vector< std::string >& enabled_checkers) {
    auto size = enabled_checkers.size();
    if (size == 0) {
        llvm::errs() << "No checkers are enabled.\n";
    } else {
        llvm::errs() << "\n* enabled" << size << " checker"
                     << (size > 1 ? "s" : "") << ":\n";
        auto i = 0;
        for (const auto& checker : enabled_checkers) {
            llvm::errs() << "(" << ++i << ") " << checker << "\n";
        }
    }
}

void print_enabled_analyses(
    const std::vector< std::string >& enabled_analyses) {
    auto size = enabled_analyses.size();
    if (size == 0) {
        llvm::errs() << "No analyses are enabled.\n";
    } else {
        llvm::errs() << "\n* enabled " << size << " "
                     << (size > 1 ? "analyses" : "analysis") << ":\n";
        auto i = 0;
        for (const auto& analysis : enabled_analyses) {
            llvm::errs() << "(" << ++i << ") " << analysis << "\n";
        }
    }
}

int main(int argc, const char** argv) {
    llvm::InitLLVM llvm_setup(argc, argv);

    auto opts_parser = CommonOptionsParser::create(argc,
                                                   argv,
                                                   knight_category,
                                                   cl::ZeroOrMore);

    ErrCode code = NormalExit;
    auto base_vfs = get_vfs(code);
    if (!base_vfs) {
        return code;
    }

    if (!opts_parser) {
        WithColor::error() << toString(opts_parser.takeError());
        return OptParseFailure;
    }

    auto opts_provider = get_opts_provider();
    auto input_path = std::string("dummy");
    const auto& src_path_lst = opts_parser->getSourcePathList();
    if (!src_path_lst.empty()) {
        input_path = fs::make_absolute(src_path_lst.front());
    }

    auto opts = opts_provider->get_options_for(input_path);

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();

    auto enabled_checkers = get_enabled_checkers();
    auto enabled_analyses = get_directly_enabled_analyses();
    if (list_checkers || list_analyses) {
        if (list_checkers) {
            print_enabled_checkers(enabled_checkers);
        }
        if (list_analyses) {
            print_enabled_analyses(enabled_analyses);
        }
        return NormalExit;
    }

    if (src_path_lst.empty()) {
        llvm::errs() << "No input files provided.\n";
        llvm::cl::PrintHelpMessage(false, true);
        return NoInputFiles;
    }

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
        llvm::errs() << "Error: compilation failed.\n";
        return CompileErrorFound;
    }

    return code;
}