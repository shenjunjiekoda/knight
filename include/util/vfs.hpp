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

#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>

namespace knight::fs {

/// \brief Create a new VFS overlay from a YAML file.
llvm::IntrusiveRefCntPtr< llvm::vfs::FileSystem > get_vfs_from_yaml(
    const std::string& overlay_yaml_file,
    llvm::IntrusiveRefCntPtr< llvm::vfs::FileSystem > base_fs);

/// \brief Create a base VFS from RFS.
llvm::IntrusiveRefCntPtr< llvm::vfs::OverlayFileSystem > create_base_vfs();

/// \brief Make path an absolute path.
///
/// Makes path absolute using the current directory if it is not already. An
/// empty path will result in the current directory.
///
/// /absolute/path   => /absolute/path
/// relative/../path => <current-directory>/relative/../path
std::string make_absolute(llvm::StringRef file);

} // namespace knight::fs