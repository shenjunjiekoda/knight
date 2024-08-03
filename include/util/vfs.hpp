//===- vfs.hpp -------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines some vfs related utilities.
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/Support/VirtualFileSystem.h>

namespace knight::fs {

using FileSystemRef = llvm::IntrusiveRefCntPtr< llvm::vfs::FileSystem >;
using OverlayFileSystemRef =
    llvm::IntrusiveRefCntPtr< llvm::vfs::OverlayFileSystem >;

/// \brief Create a new VFS overlay from a YAML file.
FileSystemRef get_vfs_from_yaml(const std::string& overlay_yaml_file,
                                FileSystemRef base_fs);

/// \brief Create a base VFS from RFS.
OverlayFileSystemRef create_base_vfs();

/// \brief Make path an absolute path.
///
/// Makes path absolute using the current directory if it is not already. An
/// empty path will result in the current directory.
///
/// /absolute/path   => /absolute/path
/// relative/../path => <current-directory>/relative/../path
std::string make_absolute(llvm::StringRef file);

} // namespace knight::fs