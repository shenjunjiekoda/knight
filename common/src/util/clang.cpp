//===- clang.cpp -------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements some llvm related utilities.
//
//===------------------------------------------------------------------===//

#include "common/util/clang.hpp"

#include <llvm/Support/WithColor.h>
#include <llvm/Support/raw_ostream.h>

#include <clang/AST/ASTContext.h>
#include <memory>
#include "clang/AST/Mangle.h"

#define DEBUG_TYPE "clang-util"

namespace knight::clang_util {

namespace {

std::string extract_llvm_version(std::string input) {
    std::regex re("\\d+");
    std::smatch match;

    if (std::regex_search(input, match, re)) {
        return match.str();
    }
    knight_log_nl(llvm::WithColor::note()
                  << "failed to extract version from input: " << input);
    return input;
}

} // anonymous namespace

std::string get_clang_include_dir() {
    return std::string("-I" LLVM_LIBRARY_DIRS "/clang/") +
           extract_llvm_version(LLVM_PACKAGE_VERSION) + "/include/";
}

constexpr unsigned MangleBufSize = 256U;

std::string get_mangled_name(const clang::NamedDecl* named_decl,
                             const clang::TargetInfo* target_info) {
    using namespace clang;
    auto& ast_ctx = named_decl->getASTContext();
    std::unique_ptr< clang::MangleContext > mangler;
    mangler.reset(ast_ctx.createMangleContext(target_info));

    if (!mangler->shouldMangleDeclName(named_decl)) {
        return std::string(named_decl->getIdentifier()->getName());
    }
    SmallString< MangleBufSize > buffer;
    llvm::raw_svector_ostream os(buffer);
    GlobalDecl global_decl;
    if (const CXXConstructorDecl* ctor =
            dyn_cast< CXXConstructorDecl >(named_decl)) {
        global_decl = GlobalDecl(ctor, Ctor_Base);
    } else if (const CXXDestructorDecl* dtor =
                   dyn_cast< CXXDestructorDecl >(named_decl)) {
        global_decl = GlobalDecl(dtor, Dtor_Base);
    } else if (named_decl->hasAttr< CUDAGlobalAttr >()) {
        global_decl = GlobalDecl(cast< FunctionDecl >(named_decl));
    } else {
        global_decl = GlobalDecl(named_decl);
    }
    mangler->mangleName(global_decl, os);

    if (!buffer.empty() && buffer.front() == '\01') {
        return std::string(buffer.substr(1));
    }
    return std::string(buffer.str());
}

} // namespace knight::clang_util