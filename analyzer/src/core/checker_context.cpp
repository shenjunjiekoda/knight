//===- checker_context.cpp --------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the checker context class.
//
//===------------------------------------------------------------------===//

#include "analyzer/core/checker_context.hpp"
#include "analyzer/core/stack_frame.hpp"
#include "analyzer/tooling/context.hpp"

namespace knight::analyzer {

clang::ASTContext& CheckerContext::get_ast_context() const {
    return *m_ctx.get_ast_context();
}

clang::SourceManager& CheckerContext::get_source_manager() const {
    return m_ctx.get_source_manager();
}

const clang::Decl* CheckerContext::get_current_decl() const {
    return m_frame->get_decl();
}

const StackFrame* CheckerContext::get_current_stack_frame() const {
    return m_frame;
}

void CheckerContext::set_current_stack_frame(const StackFrame* frame) {
    m_frame = frame;
}

} // namespace knight::analyzer