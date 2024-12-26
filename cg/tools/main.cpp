//===- main.cpp -------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This is the main file of the knight-cg tool.
//
//===------------------------------------------------------------------===//

#include "cg/tooling/cl_opts.hpp"
#include "cg/tooling/driver.hpp"
#include "common/util/log.hpp"
#include "common/util/vfs.hpp"

#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/WithColor.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdint>
#include <string>

#define DEBUG_TYPE "cg-main"

using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace knight;
using namespace knight::cg::cl_opts;

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

int main(int argc, const char** argv) {
    const llvm::InitLLVM llvm_setup(argc, argv);
    ErrCode code = NormalExit;
    auto base_vfs = get_vfs(code);
    if (!base_vfs) {
        return code;
    }

    auto opts_parser = CommonOptionsParser::create(argc,
                                                   argv,
                                                   knight_cg_category,
                                                   cl::ZeroOrMore);

    if (!opts_parser) {
        WithColor::error() << toString(opts_parser.takeError());
        return OptParseFailure;
    }

    auto input_path = std::string("dummy");
    auto src_path_lst = opts_parser->getSourcePathList();
    if (!src_path_lst.empty()) {
        input_path = fs::make_absolute(src_path_lst.front());
    }

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();

    if (src_path_lst.empty()) {
        llvm::WithColor::error() << "No input files provided.\n\n";
        llvm::cl::PrintHelpMessage(false, true);
        return NoInputFiles;
    }

    llvm::transform(src_path_lst, src_path_lst.begin(), fs::make_absolute);
    if (!ensure_input_files_exist(src_path_lst, base_vfs)) {
        return InputNotExists;
    }
    CGContext ctx(use_color,
                  show_process_sys_function,
                  skip_system_header,
                  skip_implicit_code,
                  base_vfs,
                  &opts_parser->getCompilations(),
                  src_path_lst,
                  knight_dir, // NOLINT
                  db_busy_timeout);
    KnightCGBuilder builder(ctx);
    builder.build();
    return code;
}