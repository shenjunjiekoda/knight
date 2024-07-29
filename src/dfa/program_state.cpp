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
#include "dfa/domain/domains.hpp"
#include "dfa/analysis/analysis_base.hpp"
#include "dfa/checker/checker_base.hpp"
#include "util/assert.hpp"

#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/STLExtras.h>

#include <memory>

namespace knight::dfa {

void retain_state(ProgramStateRawPtr state) {
    ++const_cast< ProgramState* >(state)->m_ref_cnt;
}

void release_state(ProgramStateRawPtr state) {
    knight_assert(state->m_ref_cnt > 0);
    auto* s = const_cast< ProgramState* >(state);
    if (--s->m_ref_cnt == 0) {
        auto& mgr = s->get_manager();
        mgr.m_state_set.RemoveNode(s);
        s->~ProgramState();
        mgr.m_free_states.push_back(s);
    }
}

ProgramState::ProgramState(ProgramStateManager* mgr, DomValMap dom_val)
    : m_mgr(mgr), m_ref_cnt(0) {
    for (auto& [id, val] : dom_val) {
        m_dom_val[id] = std::move(val);
    }
}

ProgramState::ProgramState(const ProgramState& other)
    : m_mgr(other.m_mgr), m_ref_cnt(0) {
    for (auto& [id, val] : other.m_dom_val) {
        m_dom_val[id] = val->clone();
    }
}

ProgramStateManager& ProgramState::get_manager() const {
    return *m_mgr;
}

template < typename Domain > bool ProgramState::exists() const {
    return get(Domain::get_kind()).has_value();
}

template < typename Domain >
std::optional< Domain* > ProgramState::get() const {
    auto it = m_dom_val.find(get_domain_id(Domain::get_kind()));
    if (it == m_dom_val.end()) {
        return std::nullopt;
    }
    return std::make_optional(it->second.get());
}

template < typename Domain > bool ProgramState::remove() {
    auto id = get_domain_id(Domain::get_kind());
    return m_dom_val.erase(id) != 0U;
}

template < typename Domain > void ProgramState::set(UniqueVal val) {
    m_dom_val[get_domain_id(Domain::get_kind())] = std::move(val);
}

void ProgramState::normalize() {
    llvm::for_each(m_dom_val, [](auto& pair) { pair.second->normalize(); });
}

bool ProgramState::is_bottom() const {
    return llvm::any_of(m_dom_val, [](const auto& pair) {
        return pair.second->is_bottom();
    });
}

bool ProgramState::is_top() const {
    return llvm::all_of(m_dom_val,
                        [](const auto& pair) { return pair.second->is_top(); });
}

void ProgramState::set_to_bottom() {
    llvm::for_each(m_dom_val, [](auto& pair) { pair.second->set_to_bottom(); });
}

void ProgramState::set_to_top() {
    llvm::for_each(m_dom_val, [](auto& pair) { pair.second->set_to_top(); });
}

#define UNION_MAP(OP)                                                          \
    ProgramState new_state = *this;                                            \
    DomValMap map;                                                             \
    for (auto& [other_id, other_val] : other->m_dom_val) {                     \
        auto it = m_dom_val.find(other_id);                                    \
        if (it == m_dom_val.end()) {                                           \
            map[other_id] = other_val->clone();                                \
        } else {                                                               \
            auto new_val = it->second->clone();                                \
            new_val->OP(*other_val);                                           \
            map[other_id] = std::move(new_val);                                \
        }                                                                      \
    }                                                                          \
    return get_manager().get_persistent_state_with_dom_val_map(new_state,      \
                                                               std::move(      \
                                                                   map));

#define INTERSECT_MAP(OP)                                                      \
    ProgramState new_state = *this;                                            \
    DomValMap map;                                                             \
    for (auto& [other_id, other_val] : other->m_dom_val) {                     \
        auto it = m_dom_val.find(other_id);                                    \
        if (it != m_dom_val.end()) {                                           \
            auto new_val = it->second->clone();                                \
            new_val->OP(*other_val);                                           \
            map[other_id] = std::move(new_val);                                \
        }                                                                      \
    }                                                                          \
    return get_manager().get_persistent_state_with_dom_val_map(new_state,      \
                                                               std::move(      \
                                                                   map));

ProgramStateRef ProgramState::clone() const {
    ProgramState new_state = *this;
    DomValMap map;
    for (auto& [id, val] : new_state.m_dom_val) {
        map[id] = val->clone();
    }
    return get_manager().get_persistent_state_with_dom_val_map(new_state,
                                                               std::move(map));
}

ProgramStateRef ProgramState::join(ProgramStateRef other) {
    UNION_MAP(join_with);
}

ProgramStateRef ProgramState::join_at_loop_head(ProgramStateRef other) {
    UNION_MAP(join_with_at_loop_head);
}

ProgramStateRef ProgramState::join_consecutive_iter(ProgramStateRef other) {
    UNION_MAP(join_consecutive_iter_with);
}

ProgramStateRef ProgramState::widen(ProgramStateRef other) {
    UNION_MAP(widen_with);
}

ProgramStateRef ProgramState::meet(ProgramStateRef other) {
    INTERSECT_MAP(meet_with);
}

ProgramStateRef ProgramState::narrow(ProgramStateRef other) {
    INTERSECT_MAP(narrow_with);
}

bool ProgramState::leq(const ProgramState& other) const {
    llvm::BitVector this_key_set;
    bool need_to_check_other = other.m_dom_val != this->m_dom_val;
    for (auto& [id, val] : this->m_dom_val) {
        this_key_set.set(id);
        auto it = other.m_dom_val.find(id);
        if (it == other.m_dom_val.end()) {
            if (!val->is_bottom()) {
                return false;
            }
            need_to_check_other = true;
            continue;
        }
        if (!val->leq(*(it->second))) {
            return false;
        }
    }

    if (!need_to_check_other) {
        return true;
    }

    for (auto& [id, val] : other.m_dom_val) {
        if (!this_key_set[id]) {
            if (!val->is_top()) {
                return false;
            }
        }
    }

    return true;
}

bool ProgramState::equals(const ProgramState& other) const {
    return llvm::all_of(this->m_dom_val, [other](const auto& this_pair) {
        auto it = other.m_dom_val.find(this_pair.first);
        return it != other.m_dom_val.end() &&
               this_pair.second->equals(*(it->second));
    });
}

void ProgramState::dump(llvm::raw_ostream& os) const {
    os << "ProgramState:{\n";
    for (auto& [id, aval] : m_dom_val) {
        os << "[" << get_domain_name_by_id(id) << "]: ";
        aval->dump(os);
        os << "\n";
    }
    os << "}\n";
}

ProgramStateRef ProgramStateManager::get_default_state(
    AnalysisIDSet analysis_ids) {
    // TODO: add impl
    DomValMap dom_val;
    for (auto analysis_id : analysis_ids) {
        for (auto dom_id :
             m_analysis_mgr.get_registered_domains_in(analysis_id)) {
            // ...
        }
    }
    ProgramState State(this, std::move(dom_val));

    return get_persistent_state(State);
}

ProgramStateRef ProgramStateManager::get_persistent_state(ProgramState& State) {
    llvm::FoldingSetNodeID ID;
    State.Profile(ID);
    void* InsertPos;

    if (ProgramState* existed =
            m_state_set.FindNodeOrInsertPos(ID, InsertPos)) {
        return existed;
    }

    ProgramState* new_state = nullptr;
    if (!m_free_states.empty()) {
        new_state = m_free_states.back();
        m_free_states.pop_back();
    } else {
        new_state = m_alloc.Allocate< ProgramState >();
    }
    new (new_state) ProgramState(State);
    m_state_set.InsertNode(new_state, InsertPos);
    return new_state;
}

ProgramStateRef ProgramStateManager::get_persistent_state_with_dom_val_map(
    ProgramState& state, DomValMap dom_val) {
    state.m_dom_val = std::move(dom_val);
    return get_persistent_state(state);
}

} // namespace knight::dfa