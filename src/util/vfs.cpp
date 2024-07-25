#include "util/vfs.hpp"

namespace knight::fs {

llvm::IntrusiveRefCntPtr< llvm::vfs::FileSystem > get_vfs_from_yaml(
    const std::string& overlay_yaml_file,
    llvm::IntrusiveRefCntPtr< llvm::vfs::FileSystem > base_fs) {
    auto buffer = base_fs->getBufferForFile(overlay_yaml_file);
    if (!buffer) {
        llvm::errs() << "Can't load virtual filesystem overlay yaml file '"
                     << overlay_yaml_file
                     << "': " << buffer.getError().message() << ".\n";
        return nullptr;
    }

    llvm::IntrusiveRefCntPtr< llvm::vfs::FileSystem > fs =
        llvm::vfs::getVFSFromYAML(std::move(buffer.get()),
                                  nullptr,
                                  overlay_yaml_file);
    if (!fs) {
        llvm::errs() << "Yaml error: invalid virtual filesystem overlay file '"
                     << overlay_yaml_file << "'.\n";
        return nullptr;
    }
    return fs;
}

llvm::IntrusiveRefCntPtr< llvm::vfs::OverlayFileSystem > create_base_vfs() {
    return llvm::IntrusiveRefCntPtr< llvm::vfs::OverlayFileSystem >(
        new llvm::vfs::OverlayFileSystem(llvm::vfs::getRealFileSystem()));
}

std::string make_absolute(llvm::StringRef file) {
    if (file.empty())
        return {};
    llvm::SmallString< 256 > abs_path(file);
    if (std::error_code EC = llvm::sys::fs::make_absolute(abs_path)) {
        llvm::errs() << "make absolute path failed (" << file
                     << "): " << EC.message() << "\n";
    }
    return abs_path.str().str();
}

} // namespace knight::fs