//===- program_state.hpp -------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the program state class.
//
//===------------------------------------------------------------------===//

#pragma once

#include <unordered_map>
#include <unordered_set>

#include "dfa/domain/dom_base.hpp"
#include "dfa/domain/domains.hpp"
#include "llvm/Support/raw_ostream.h"

namespace knight::dfa {

class ProgramState {
  public:
    using DomMap = std::unordered_map< DomID, DomSharedRef >;
    using DomRefs = std::unordered_set< DomRef >;

  private:
    DomMap m_dom_map;

  public:
    ProgramState(const DomIDs& ids);

    /// \brief Copy assignment is not allowed.
    void operator=(const ProgramState& R) = delete;

  public:
    /// \brief Check if the given domain kind exists in the program state.
    ///
    /// \return True if the domain kind exists, false otherwise.
    bool exists(DomainKind kind) { return get(kind).has_value(); }

    /// \brief Get the domain with the given kind.
    ///
    /// \return The domain with the given kind, or std::nullopt if it doesn't
    /// exist.
    std::optional< DomRef > get(DomainKind kind) {
        auto it = m_dom_map.find(get_domain_id(kind));
        if (it == m_dom_map.end()) {
            return std::nullopt;
        }
        return it->second.get();
    }

    /// \brief Remove the given domain from the program state.
    void remove(DomainKind kind) { m_dom_map.erase(get_domain_id(kind)); }

    /// \brief Set the given domain to the given abstract val.
    void set(DomainKind kind, DomSharedRef dom) {
        m_dom_map[get_domain_id(kind)] = std::move(dom);
    }

  public:
    void normalize();

    bool is_bottom() const;
    bool is_top() const;
    void set_to_bottom();
    void set_to_top();

    void join_with(const ProgramState& other);
    void join_with_at_loop_head(const ProgramState& other);
    // void join_consecutive_iter_with(const ProgramState& other);

    // void widen_with(const ProgramState& other);
    // void meet_with(const ProgramState& other);
    // void narrow_with(const ProgramState& other);

    // bool leq(const ProgramState& other) const;
    // bool equals(const ProgramState& other) const;

    // bool operator==(const ProgramState& other) const { return equals(other); }
    // bool operator!=(const ProgramState& other) const { return !equals(other); }

    // void dump(llvm::raw_ostream& os) const;

}; // class ProgramState

} // namespace knight::dfa