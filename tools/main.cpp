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

#include <clang/Tooling/CommonOptionsParser.h>

#include "tooling/cl_opts.hpp"

using namespace llvm;
using namespace clang;
using namespace clang::tooling;
using namespace knight;
using namespace knight::cl_opts;

constexpr uint8_t OPT_PARSE_FAILURE = 1;

int main(int argc, const char** argv) {
    llvm::InitLLVM X(argc, argv);

    llvm::Expected< CommonOptionsParser > opts_parser =
        CommonOptionsParser::create(argc,
                                    argv,
                                    knight_category,
                                    cl::ZeroOrMore);
    if (!opts_parser) {
        WithColor::error() << toString(opts_parser.takeError());
        return OPT_PARSE_FAILURE;
    }

    llvm::outs() << "Hello, this is knight!\n";
    return 0;
}