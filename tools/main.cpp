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

#include <cstdint>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/WithColor.h>
#include <llvm/Support/TargetSelect.h>

#include <clang/Tooling/CommonOptionsParser.h>
#include <string>

#include "tooling/cl_opts.hpp"
#include "tooling/context.hpp"
#include "tooling/knight.hpp"
#include "tooling/options.hpp"
#include "util/vfs.hpp"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace knight;
using namespace knight::cl_opts;

using ErrCode = uint8_t;
constexpr ErrCode NORMAL_EXIT = 0;
constexpr ErrCode OPT_PARSE_FAILURE = 1;
constexpr ErrCode BASE_VFS_CREATE_FAILURE = 2;
constexpr ErrCode VFS_FILE_FAILURE = 3;
constexpr ErrCode NO_INPUT_FILES = 4;

llvm::IntrusiveRefCntPtr< llvm::vfs::OverlayFileSystem > get_vfs(
    ErrCode& code) {
    auto base_vfs = fs::create_base_vfs();
    if (!base_vfs) {
        code = BASE_VFS_CREATE_FAILURE;
        return nullptr;
    }

    if (!overlay_file.empty()) {
        if (auto vfs = fs::get_vfs_from_yaml(overlay_file, base_vfs)) {
            base_vfs->pushOverlay(std::move(vfs));
        } else {
            code = VFS_FILE_FAILURE;
            return nullptr;
        }
    }
    code = NORMAL_EXIT;
    return base_vfs;
}

std::unique_ptr< KnightOptionsProvider > get_opts_provider() {
    auto opts_provider = std::make_unique< KnightOptionsCommandLineProvider >();
    if (analyses.getNumOccurrences() > 0) {
        opts_provider->options.analyses = analyses;
    }
    if (checkers.getNumOccurrences() > 0) {
        opts_provider->options.checkers = checkers;
    }
    if (use_color.getNumOccurrences() > 0) {
        opts_provider->options.use_color = use_color;
    }
    return std::move(opts_provider);
}

std::vector< std::string > get_enabled_checkers() {
    KnightContext context(std::move(get_opts_provider()));
    KnightASTConsumerFactory factory(context);
    std::vector< std::string > checkers;
    for (const auto& [_, name] : factory.get_enabled_checks()) {
        checkers.push_back(name.str());
    }
    return checkers;
}

int main(int argc, const char** argv) {
    llvm::InitLLVM X(argc, argv);

    auto opts_parser = CommonOptionsParser::create(argc,
                                                   argv,
                                                   knight_category,
                                                   cl::ZeroOrMore);

    ErrCode code;
    auto base_vfs = get_vfs(code);
    if (!base_vfs) {
        return code;
    }

    if (!opts_parser) {
        WithColor::error() << toString(opts_parser.takeError());
        return OPT_PARSE_FAILURE;
    }

    auto opts_provider = get_opts_provider();
    auto input_path = std::string("dummy");
    const auto& src_path_lst = opts_parser->getSourcePathList();
    if (!src_path_lst.empty()) {
        input_path = fs::make_absolute(src_path_lst.front());
    }

    auto opts = opts_provider->get_options_for(input_path);

    llvm::outs() << "Hello " << opts.user << ", this is knight!\n";

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();

    auto checkers = get_enabled_checkers();
    if (list_checkers) {
        if (checkers.empty()) {
            llvm::errs() << "No checkers are enabled.\n";
        } else {
            llvm::errs() << "Enabled checkers:\n";
            auto i = 0;
            for (const auto& checker : checkers) {
                llvm::errs() << ++i << ". " << checker << "\n";
            }
        }
        return NORMAL_EXIT;
    }

    if (src_path_lst.empty()) {
        llvm::errs() << "No input files provided.\n";
        llvm::cl::PrintHelpMessage(false, true);
        return NO_INPUT_FILES;
    }

    return NORMAL_EXIT;
}