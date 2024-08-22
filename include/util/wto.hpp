//===- wto.hpp --------------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines some utilities for weak topological ordering.
//
//===------------------------------------------------------------------===//

#pragma once

#include "support/dumpable.hpp"
#include "support/graph.hpp"
#include "util/assert.hpp"

#include <llvm/Support/raw_ostream.h>

#include <climits>
#include <list>

namespace knight {

template < graph G, typename GraphTrait >
class Wto;
template < graph G, typename GraphTrait >
class WtoVertex;
template < graph G, typename GraphTrait >
class WtoCycle;
template < graph G, typename GraphTrait >
class WtoComponentVisitor;

/// \brief Represents the nesting of a node
///
/// The nesting of a node is the list of cycles containing the node, from
/// the outermost to the innermost.
template < graph G, typename GraphTrait = GraphTrait< G > >
class WtoNesting {
  public:
    using NodeRef = typename GraphTrait::NodeRef;
    using Nodes = std::vector< NodeRef >;
    using NodesIterator = typename Nodes::const_iterator;

  private:
    Nodes m_nodes;

  public:
    WtoNesting() = default;
    WtoNesting(const WtoNesting&) = default;
    WtoNesting(WtoNesting&&) = default;
    WtoNesting& operator=(const WtoNesting&) = default;
    WtoNesting& operator=(WtoNesting&&) = default;
    ~WtoNesting() = default;

  public:
    /// \brief Add a cycle head in the nesting
    void add(NodeRef head) { this->m_nodes.push_back(head); }

    /// \brief iterator over the head of cycles
    /// @{
    NodesIterator begin() const { return this->m_nodes.cbegin(); }
    NodesIterator end() const { return this->m_nodes.cend(); }
    /// @}

    /// \brief Return the common prefix of the given nestings
    WtoNesting get_common_prefix(const WtoNesting& other) const {
        WtoNesting res;
        for (auto it = this->begin(), other_it = other.begin();
             it != this->end() && other_it != other.end();
             ++it, ++other_it) {
            if (*it == *other_it) {
                res.add(*it);
            } else {
                break;
            }
        }
        return res;
    }

    void dump(llvm::raw_ostream& os) const {
        os << "[";
        for (auto it = this->begin(), end = this->end(); it != end;) {
            DumpableTrait< NodeRef >::dump(os, *it);
            ++it;
            if (it != end) {
                os << ", ";
            }
        }
        os << "]";
    }

  public:
    bool operator<(const WtoNesting& other) const {
        return this->compare(other) == -1;
    }
    bool operator<=(const WtoNesting& other) const {
        return this->compare(other) <= 0;
    }
    bool operator==(const WtoNesting& other) const {
        return this->compare(other) == 0;
    }
    bool operator>=(const WtoNesting& other) const {
        return this->operator<=(other, *this);
    }
    bool operator>(const WtoNesting& other) const {
        return this->compare(other) == 1;
    }

  private:
    /// \brief Compare the given nestings
    ///
    /// \return -1 if `this` is nested within `other`
    /// \return 0 if they are equal
    /// \return 1 if `other` is nested within `this`
    /// \return other if they are not comparable
    int compare(const WtoNesting& other) const {
        if (this == &other) {
            return 0; // equals
        }

        auto it = this->begin();
        auto other_it = other.begin();
        while (it != this->end()) {
            if (other_it == other.end()) {
                return 1; // `this` is nested within `other`
            }
            if (*it == *other_it) {
                ++it;
                ++other_it;
            } else {
                return 2; // not comparable
            }
        }
        if (other_it == other.end()) {
            return 0; // equals
        }
        return -1; // `other` is nested within `this`
    }

}; // class WtoNesting

/// \brief Base class for components of a weak topological order
///
/// This is either a vertex or a cycle.
template < graph G, typename GraphTrait = GraphTrait< G > >
class WtoComponent {
  public:
    WtoComponent() = default;
    WtoComponent(const WtoComponent&) noexcept = default;
    WtoComponent(WtoComponent&&) noexcept = default;
    WtoComponent& operator=(const WtoComponent&) noexcept = default;
    WtoComponent& operator=(WtoComponent&&) noexcept = default;
    virtual ~WtoComponent() = default;

  public:
    /// \brief Accept the given visitor
    virtual void accept(WtoComponentVisitor< G, GraphTrait >&) const = 0;

}; // class WtoComponent

template < graph G, typename GraphTrait = GraphTrait< G > >
class WtoVertex final : public WtoComponent< G, GraphTrait > {
  public:
    using NodeRef = typename GraphTrait::NodeRef;

  private:
    NodeRef m_node;

  public:
    explicit WtoVertex(NodeRef node) : m_node(node) {}

  public:
    /// \brief Return the graph node
    NodeRef get_node() const { return this->m_node; }

    /// \brief Accept the given visitor
    void accept(WtoComponentVisitor< G, GraphTrait >& v) const override {
        v.visit(*this);
    }

    void dump(llvm::raw_ostream& os) const {
        DumpableTrait< NodeRef >::dump(os, this->m_node);
    }

}; // class WtoVertex

template < graph G, typename GraphTrait = GraphTrait< G > >
class WtoCycle final : public WtoComponent< G, GraphTrait > {
  public:
    using NodeRef = typename GraphTrait::NodeRef;
    using WtoComponentT = WtoComponent< G, GraphTrait >;
    using WtoComponentPtr = std::unique_ptr< WtoComponentT >;
    using WtoComponentList = std::list< WtoComponentPtr >;

  private:
    /// \brief Head of the cycle
    NodeRef m_head;

    /// \brief List of components
    WtoComponentList m_components;

  public:
    WtoCycle(NodeRef head, WtoComponentList components)
        : m_head(head), m_components(std::move(components)) {}

    /// \brief Return the head of the cycle
    NodeRef get_head() const { return this->m_head; }

    std::list< WtoComponentT* > components() const {
        std::list< WtoComponentT* > res;
        for (const auto& c : this->m_components) {
            res.push_back(c.get());
        }
        return res;
    }

    /// \brief Accept the given visitor
    void accept(WtoComponentVisitor< G, GraphTrait >& v) const override {
        v.visit(*this);
    }

    /// \brief Dump the cycle, for debugging purpose
    void dump(llvm::raw_ostream& o) const {
        o << "(";
        DumpableTrait< NodeRef >::dump(o, this->m_head);
        for (const auto& c : this->m_components) {
            o << " ";
            c->dump(o);
        }
        o << ")";
    }

}; // class WtoCycle

/// \brief Weak topological order visitor
template < graph G, typename GraphTrait = GraphTrait< G > >
class WtoComponentVisitor {
  public:
    using WtoVertexT = WtoVertex< G, GraphTrait >;
    using WtoCycleT = WtoCycle< G, GraphTrait >;

  public:
    WtoComponentVisitor() = default;
    WtoComponentVisitor(const WtoComponentVisitor&) noexcept = default;
    WtoComponentVisitor(WtoComponentVisitor&&) noexcept = default;
    WtoComponentVisitor& operator=(const WtoComponentVisitor&) noexcept =
        default;
    WtoComponentVisitor& operator=(WtoComponentVisitor&&) noexcept = default;
    virtual ~WtoComponentVisitor() = default;

    /// \brief Visit the given vertex
    virtual void visit(const WtoVertexT&) = 0;
    /// \brief Visit the given cycle
    virtual void visit(const WtoCycleT&) = 0;

}; // class WtoComponentVisitor

/// \brief Weak Topological Ordering
template < graph G, typename GraphTrait = GraphTrait< G > >
class Wto {
  public:
    using GraphRef = G::GraphRef;
    using NodeRef = typename GraphTrait::NodeRef;
    using WtoNestingT = WtoNesting< G, GraphTrait >;
    using WtoComponentT = WtoComponent< G, GraphTrait >;
    using WtoVertexT = WtoVertex< G, GraphTrait >;
    using WtoCycleT = WtoCycle< G, GraphTrait >;

  private:
    using WtoComponentPtr = std::unique_ptr< WtoComponentT >;
    using WtoComponentList = std::list< WtoComponentPtr >;
    using WtoComponentListConstIterator =
        typename WtoComponentList::const_iterator;
    using Dfn = int;
    using DfnTable = std::unordered_map< NodeRef, Dfn >;
    using Stack = std::vector< NodeRef >;
    using WtoNestingPtr = std::shared_ptr< WtoNestingT >;
    using NestingTable = std::unordered_map< NodeRef, WtoNestingPtr >;

  private:
    WtoComponentList m_components;
    NestingTable m_nesting_table;
    DfnTable m_dfn_table;
    Dfn m_num{0};
    Stack m_stack;

  public:
    /// \brief Compute the weak topological order of the given graph
    explicit Wto(GraphRef cfg) {
        this->visit(GraphTrait::entry(cfg), this->m_components);
        this->m_dfn_table.clear();
        this->m_stack.clear();
        this->build_nesting();
    }

    Wto(const Wto& other) = delete;
    Wto(Wto&& other) = default;
    Wto& operator=(const Wto& other) = delete;
    Wto& operator=(Wto&& other) = default;
    ~Wto() = default;

  private:
    /// \brief Visitor to build the nestings of each node
    class NestingBuilder final : public WtoComponentVisitor< G, GraphTrait > {
      private:
        WtoNestingPtr m_nesting;
        NestingTable& m_nesting_table;

      public:
        explicit NestingBuilder(NestingTable& nesting_table)
            : m_nesting(std::make_shared< WtoNestingT >()),
              m_nesting_table(nesting_table) {}

        void visit(const WtoCycleT& cycle) override {
            NodeRef head = cycle.get_head();
            WtoNestingPtr previous_nesting = this->m_nesting;
            this->m_nesting_table.insert(std::make_pair(head, this->m_nesting));
            this->m_nesting = std::make_shared< WtoNestingT >(*this->m_nesting);
            this->m_nesting->add(head);
            for (auto* component : cycle.components()) {
                component->accept(*this);
            }
            this->m_nesting = previous_nesting;
        }

        void visit(const WtoVertexT& vertex) override {
            this->m_nesting_table.insert(
                std::make_pair(vertex.get_node(), this->m_nesting));
        }

    }; // end class NestingBuilder

  private:
    /// \brief Return the depth-first number of the given node
    Dfn dfn(NodeRef n) const {
        auto it = this->m_dfn_table.find(n);
        if (it != this->m_dfn_table.end()) {
            return it->second;
        }
        return 0;
    }

    /// \brief Set the depth-first number of the given node
    void set_dfn(NodeRef n, const Dfn& dfn) {
        auto res = this->m_dfn_table.insert(std::make_pair(n, dfn));
        if (!res.second) {
            (res.first)->second = dfn;
        }
    }

    /// \brief Pop a node from the stack
    NodeRef pop() {
        knight_assert_msg(!this->m_stack.empty(), "empty stack");
        NodeRef top = this->m_stack.back();
        this->m_stack.pop_back();
        return top;
    }

    /// \brief Push a node on the stack
    void push(NodeRef n) { this->m_stack.push_back(n); }

    /// \brief Create the cycle component for the given vertex
    WtoComponentPtr component(NodeRef vertex) {
        WtoComponentList partition;
        for (auto it = GraphTrait::succ_begin(vertex),
                  et = GraphTrait::succ_end(vertex);
             it != et;
             ++it) {
            NodeRef succ = *it;
            if (this->dfn(succ) == 0) {
                this->visit(succ, partition);
            }
        }
        return std::make_unique< WtoCycleT >(vertex, std::move(partition));
    }

    /// \brief Visit the given node
    ///
    /// Algorithm to build a weak topological order of a graph
    Dfn visit(NodeRef vertex, WtoComponentList& partition) {
        Dfn head(0);
        Dfn min(0);

        this->push(vertex);
        this->m_num += 1;
        head = this->m_num;
        this->set_dfn(vertex, head);
        bool loop = false;
        for (auto it = GraphTrait::succ_begin(vertex),
                  et = GraphTrait::succ_end(vertex);
             it != et;
             ++it) {
            NodeRef succ = *it;
            Dfn succ_dfn = this->dfn(succ);
            if (succ_dfn == 0) {
                min = this->visit(succ, partition);
            } else {
                min = succ_dfn;
            }
            if (min <= head) {
                head = min;
                loop = true;
            }
        }
        if (head == this->dfn(vertex)) {
            this->set_dfn(vertex, INT_MAX);
            NodeRef element = this->pop();
            if (loop) {
                while (element != vertex) {
                    this->set_dfn(element, 0);
                    element = this->pop();
                }
                partition.push_front(this->component(vertex));
            } else {
                partition.push_front(std::make_unique< WtoVertexT >(vertex));
            }
        }
        return head;
    }

    /// \brief Build the nesting table
    void build_nesting() {
        NestingBuilder builder(this->m_nesting_table);
        for (auto& unique_component : m_components) {
            unique_component.get()->accept(builder);
        }
    }

  public:
    /// \brief Return the nesting of the given node
    const WtoNestingT& get_nesting(NodeRef n) const {
        auto it = this->m_nesting_table.find(n);
        assert(it != this->m_nesting_table.end() && "node not found");
        return *(it->second);
    }

    /// \brief Accept the given visitor
    void accept(WtoComponentVisitor< G, GraphTrait >& v) {
        for (const auto& c : this->m_components) {
            c->accept(v);
        }
    }

    WtoComponentListConstIterator begin() const {
        return this->m_components.cbegin();
    }

    WtoComponentListConstIterator end() const {
        return this->m_components.cend();
    }

    /// \brief Dump the order, for debugging purpose
    void dump(llvm::raw_ostream& o) const {
        for (auto it = this->begin(), et = this->end(); it != et;) {
            it->dump(o);
            ++it;
            if (it != et) {
                o << " ";
            }
        }
    }

}; // end class Wto

} // namespace knight