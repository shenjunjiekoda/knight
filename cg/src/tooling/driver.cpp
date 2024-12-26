//===- knight.cpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the core driver for the Knight cg tool.
//
//===------------------------------------------------------------------===//

#include "cg/tooling/driver.hpp"
#include "cg/core/builder.hpp"
#include "cg/db/db.hpp"
#include "common/util/clang.hpp"
#include "common/util/log.hpp"
#include "common/util/vfs.hpp"

#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Support/raw_ostream.h>

#define DEBUG_TYPE "cg-driver"

STATISTIC(NumSystemFunctions, "The # of times system functions are processed");
STATISTIC(NumImplicitFunctions,
          "The # of times implicit functions are processed");
STATISTIC(NumTotalFunctions, "The # of times total functions are processed");

namespace knight {

namespace {

class KnightAction : public clang::ASTFrontendAction {
  public:
    explicit KnightAction(KnightIRConsumer* ast_factory)
        : m_ast_factory(ast_factory) {}
    std::unique_ptr< clang::ASTConsumer > CreateASTConsumer(
        clang::CompilerInstance& ci, llvm::StringRef file) override {
        return m_ast_factory->create_ast_consumer(ci, file);
    }

  private:
    KnightIRConsumer* m_ast_factory;
}; // class KnightAction

class KnightActionFactory : public clang::tooling::FrontendActionFactory {
  public:
    explicit KnightActionFactory(CGContext& ctx) : m_ast_factory(ctx) {}
    std::unique_ptr< clang::FrontendAction > create() override {
        return std::make_unique< KnightAction >(&m_ast_factory);
    }

  private:
    KnightIRConsumer m_ast_factory;
}; // class KnightActionFactory

} // anonymous namespace

KnightIRConsumer::KnightIRConsumer(CGContext& ctx) : m_ctx(ctx) {}

std::unique_ptr< clang::ASTConsumer > KnightIRConsumer::create_ast_consumer(
    clang::CompilerInstance& ci, llvm::StringRef file) {
    auto& source_mgr = ci.getSourceManager();
    auto& file_mgr = source_mgr.getFileManager();
    m_ctx.file = file;
    m_ctx.ast_ctx = &ci.getASTContext();

    return std::make_unique< KnightASTConsumer >(m_ctx);
}

void KnightCGBuilder::build() {
    using namespace clang;
    using namespace clang::tooling;
    ClangTool clang_tool(*m_ctx.cdb,
                         m_ctx.input_files,
                         std::make_shared< PCHContainerOperations >(),
                         m_ctx.overlay_fs);
    clang::tooling::CommandLineArguments clang_include{
        clang_util::get_clang_include_dir()};
    knight_log_nl(llvm::outs()
                  << "clang include dir: " << clang_include[0] << "\n");
    clang_tool.appendArgumentsAdjuster(
        clang::tooling::
            getInsertArgumentAdjuster(clang_include,
                                      clang::tooling::ArgumentInsertPosition::
                                          END));

    KnightActionFactory action_factory(m_ctx);
    clang_tool.run(&action_factory);

    knight_log_nl(
        llvm::outs() << "dump db: " << "\n";
        cg::Database db(m_ctx.knight_dir, m_ctx.db_busy_timeout);
        for (const auto& cs
             : db.get_all_callsites()) {
            cs.dump(llvm::outs());
        } for (const auto& node
               : db.get_all_cg_nodes()) { node.dump(llvm::outs()); });
}

bool KnightASTConsumer::HandleTopLevelDecl(clang::DeclGroupRef decl_group) {
    for (auto* decl : decl_group) {
        auto* function = llvm::dyn_cast_or_null< clang::FunctionDecl >(decl);
        if (function == nullptr || !function->hasBody()) {
            continue;
        }
        auto is_system_function =
            m_ctx.ast_ctx->getSourceManager().isInSystemHeader(
                decl->getLocation());
        if (is_system_function && m_ctx.skip_system_header) {
            continue;
        }
        if (function->isImplicit()) {
            if (m_ctx.skip_implicit_code) {
                continue;
            }
            NumImplicitFunctions++;
        }
        if (!is_system_function || m_ctx.show_process_sys_function) {
            llvm::outs() << "[*] Processing function: ";
        }

        if (is_system_function) {
            NumSystemFunctions++;
        }

        NumTotalFunctions++;

        if (m_ctx.use_color) {
            if (is_system_function) {
                if (m_ctx.show_process_sys_function) {
                    llvm::outs().changeColor(llvm::raw_ostream::Colors::RED);
                }
            } else {
                llvm::outs().changeColor(llvm::raw_ostream::Colors::GREEN);
            }
        }
        if (!is_system_function || m_ctx.show_process_sys_function) {
            function->printName(llvm::outs());
            llvm::outs().resetColor();
            if (function->isImplicit()) {
                llvm::outs() << " (implicit)";
            }
            llvm::outs() << "\n";
        }
        m_builder.TraverseDecl(function);
    }

    return true;
}
} // namespace knight