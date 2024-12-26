//===- vfs.cpp -------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header implements some vfs related utilities.
//
//===------------------------------------------------------------------===//

#include "common/util/vfs.hpp"

#include <llvm/Support/WithColor.h>

namespace knight::fs {

namespace {

constexpr unsigned PathMaxLen = 256U;

} // anonymous namespace

FileSystemRef get_vfs_from_yaml(const std::string& overlay_yaml_file,
                                const FileSystemRef& base_fs) {
    auto buffer = base_fs->getBufferForFile(overlay_yaml_file);
    if (!buffer) {
        llvm::WithColor::error()
            << "Can't load virtual filesystem overlay yaml file '"
            << overlay_yaml_file << "': " << buffer.getError().message()
            << ".\n";
        return nullptr;
    }

    FileSystemRef fs = llvm::vfs::getVFSFromYAML(std::move(buffer.get()),
                                                 nullptr,
                                                 overlay_yaml_file);
    if (!fs) {
        llvm::WithColor::error()
            << "Yaml error: invalid virtual filesystem overlay file '"
            << overlay_yaml_file << "'.\n";
        return nullptr;
    }
    return fs;
}

OverlayFileSystemRef create_base_vfs() {
    return {new llvm::vfs::OverlayFileSystem(llvm::vfs::getRealFileSystem())};
}

std::string make_absolute(llvm::StringRef file) {
    if (file.empty()) {
        return {};
    }
    llvm::SmallString< PathMaxLen > abs_path(file);
    if (const std::error_code ec = llvm::sys::fs::make_absolute(abs_path)) {
        llvm::WithColor::error() << "make absolute path failed (" << file
                                 << "): " << ec.message() << "\n";
    }
    return abs_path.str().str();
}

bool exists(llvm::StringRef file, const FileSystemRef& fs) {
    return fs->exists(file);
}

} // namespace knight::fs