//===- cg.hpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file defines the core cg data structures.
//
//===------------------------------------------------------------------===//

#pragma once

#include <string>

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/raw_ostream.h>

namespace knight::cg {

constexpr unsigned CSToStringMaxSize = 256U;

struct CallSite {
    unsigned line;
    unsigned col;
    std::string caller;
    std::string callee;

    CallSite() = default;
    CallSite(unsigned line,
             unsigned col,
             std::string caller,
             std::string callee)
        : line(line),
          col(col),
          caller(std::move(caller)),
          callee(std::move(callee)) {}

    bool operator==(const CallSite& other) const {
        return line == other.line && col == other.col &&
               caller == other.caller && callee == other.callee;
    }
    bool operator!=(const CallSite& other) const { return !(*this == other); }
    bool operator<(const CallSite& other) const {
        if (line != other.line) {
            return line < other.line;
        }
        if (col != other.col) {
            return col < other.col;
        }
        if (caller != other.caller) {
            return caller < other.caller;
        }
        return callee < other.callee;
    }

    void dump(llvm::raw_ostream& os) const {
        os << "CallSite: " << line << ":" << col << " " << caller << " -> "
           << callee;
    }

    [[nodiscard]] std::string to_string() const {
        llvm::SmallString< CSToStringMaxSize > str;
        llvm::raw_svector_ostream os(str);
        dump(os);
        return os.str().str();
    }
}; // struct CallSite

struct CallGraphNode {
    unsigned line;
    unsigned col;
    std::string name;
    std::string mangled_name;
    std::string file;

    CallGraphNode() = default;
    CallGraphNode(unsigned line,
                  unsigned col,
                  std::string name,
                  std::string mangled_name,
                  std::string file)
        : line(line),
          col(col),
          name(std::move(name)),
          mangled_name(std::move(mangled_name)),
          file(std::move(file)) {}

    bool operator==(const CallGraphNode& other) const {
        return line == other.line && col == other.col && name == other.name &&
               mangled_name == other.mangled_name && file == other.file;
    }
    bool operator!=(const CallGraphNode& other) const {
        return !(*this == other);
    }
    bool operator<(const CallGraphNode& other) const {
        if (line != other.line) {
            return line < other.line;
        }
        if (col != other.col) {
            return col < other.col;
        }
        if (name != other.name) {
            return name < other.name;
        }
        if (mangled_name != other.mangled_name) {
            return mangled_name < other.mangled_name;
        }
        return file < other.file;
    }

    void dump(llvm::raw_ostream& os) const {
        os << "CallGraphNode: " << line << ":" << col << " " << name << " "
           << mangled_name << " " << file;
    }

    [[nodiscard]] std::string to_string() const {
        llvm::SmallString< CSToStringMaxSize > str;
        llvm::raw_svector_ostream os(str);
        dump(os);
        return os.str().str();
    }
}; // struct CallGraphNode

} // namespace knight::cg

namespace std {

template <>
struct hash< knight::cg::CallSite > {
    std::size_t operator()(const knight::cg::CallSite& cs) const noexcept {
        std::size_t h1 = std::hash< unsigned >{}(cs.line);
        std::size_t h2 = std::hash< unsigned >{}(cs.col);
        std::size_t h3 = std::hash< std::string >{}(cs.caller);
        std::size_t h4 = std::hash< std::string >{}(cs.callee);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3); // NOLINT
    }
};

template <>
struct hash< knight::cg::CallGraphNode > {
    std::size_t operator()(
        const knight::cg::CallGraphNode& cgn) const noexcept {
        std::size_t h1 = std::hash< unsigned >{}(cgn.line);
        std::size_t h2 = std::hash< unsigned >{}(cgn.col);
        std::size_t h3 = std::hash< std::string >{}(cgn.name);
        std::size_t h4 = std::hash< std::string >{}(cgn.mangled_name);
        std::size_t h5 = std::hash< std::string >{}(cgn.file);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4); // NOLINT
    }
};

} // namespace std