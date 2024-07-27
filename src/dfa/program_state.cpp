//===- program_state.cpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This file implements the program state class.
//
//===------------------------------------------------------------------===//

#include "dfa/program_state.hpp"

#include <llvm/ADT/STLExtras.h>

#include <memory>

namespace knight::dfa {

ProgramState::ProgramState(const DomIDs& ids) {
    m_dom_map.reserve(ids.size());
    for (auto id : ids) {
        // TODO: add and collect domain registry method in analysis and create here.
    }
}

void ProgramState::normalize() {
    llvm::for_each(m_dom_map, [](auto& pair) { pair.second->normalize(); });
}

bool ProgramState::is_bottom() const {
    return llvm::any_of(m_dom_map, [](const auto& pair) {
        return pair.second->is_bottom();
    });
}

bool ProgramState::is_top() const {
    return llvm::all_of(m_dom_map,
                        [](const auto& pair) { return pair.second->is_top(); });
}

void ProgramState::set_to_bottom() {
    llvm::for_each(m_dom_map, [](auto& pair) { pair.second->set_to_bottom(); });
}

void ProgramState::set_to_top() {
    llvm::for_each(m_dom_map, [](auto& pair) { pair.second->set_to_top(); });
}

void ProgramState::join_with(const ProgramState& other) {
    for (auto& [other_id, other_dom] : other.m_dom_map) {
        auto it = m_dom_map.find(other_id);
        if (it == m_dom_map.end()) {
            this->m_dom_map.insert({other_id, other_dom});
        } else {
            this->m_dom_map[other_id]->join_with(*other_dom);
        }
    }
}

} // namespace knight::dfa