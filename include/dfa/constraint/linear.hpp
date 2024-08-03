//===- linear.hpp -----------------------------------------------------===//
//
// Copyright (c) 2024 Junjie Shen
//
// see https://github.com/shenjunjiekoda/knight/blob/main/LICENSE for
// license information.
//
//===------------------------------------------------------------------===//
//
//  This header defines the linear system.
//
//===------------------------------------------------------------------===//

#pragma once

#include <llvm/Support/raw_ostream.h>

#include "support/dumpable.hpp"
#include "util/assert.hpp"

#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace knight::dfa {

template < typename Num, typename Var > class LinearExpr {
  public:
    using VarRef = Var::Ref;
    using VarSet = std::unordered_set< VarRef >;
    using Map = std::unordered_map< VarRef, Num >;

  private:
    Map m_terms;
    Num m_constant;

  public:
    LinearExpr() = default;
    explicit LinearExpr(Num n) : m_constant(std::move(n)) {}
    explicit LinearExpr(VarRef var) { this->m_terms.emplace(var, Num(1)); }

    /// \brief k * var
    LinearExpr(Num k, Var var) {
        if (k != 0) {
            this->m_terms.emplace(var, std::move(k));
        }
    }

    LinearExpr(const LinearExpr&) = default;
    LinearExpr(LinearExpr&&) = default;
    LinearExpr& operator=(const LinearExpr&) = default;
    LinearExpr& operator=(LinearExpr&&) = default;
    ~LinearExpr() = default;

  private:
    LinearExpr(Map terms, Num constant)
        : m_terms(std::move(terms)), m_constant(std::move(constant)) {}

  public:
    /// \brief Plus constant
    void plus(const Num& n) { this->m_constant += n; }

    /// \brief Plus variable
    void plus(VarRef var) { this->plus(1, var); }

    /// \brief Plus k * var
    void plus(const Num& factor, Var var) {
        auto it = this->m_terms.find(var);
        if (it != this->m_terms.end()) {
            Num r = it->second + factor;
            if (r == 0) {
                this->m_terms.erase(it);
            } else {
                it->second = r;
            }
        } else {
            if (factor != 0) {
                this->m_terms.emplace(var, factor);
            }
        }
    }

    void set_to_constant(Num cst) {
        Map().swap(this->m_terms);
        this->m_constant = cst;
    }

    void set_to_zero() { set_to_constant(0); }

    Map& get_terms() { return this->m_terms; }
    const Map& get_variable_terms() const { return this->m_terms; }
    std::size_t num_variable_terms() const { return this->m_terms.size(); }

    bool is_constant() const { return this->m_terms.empty(); }
    const Num& get_constant_term() const { return this->m_constant; }

    bool is_constant(Num cst) const {
        return is_constant() && get_constant_term() == cst;
    }

    Num get_factor_of(VarRef var) const {
        auto it = this->m_terms.find(var);
        if (it != this->m_terms.end()) {
            return it->second;
        } else {
            return Num(0);
        }
    }

    void operator+=(const Num& n) { this->m_constant += n; }
    void operator+=(VarRef var) { this->plus(var); }

    /// \brief Plus a linear expression
    void operator+=(const LinearExpr& expr) {
        for (const auto& [var, factor] : expr) {
            this->plus(factor, var);
        }
        this->m_constant += expr.constant();
    }

    /// \brief Substract a num
    void operator-=(const Num& n) { this->m_constant -= n; }

    /// \brief Substract a variable
    void operator-=(VarRef var) { this->plus(-1, var); }

    /// \brief Substract a linear expression
    void operator-=(const LinearExpr& expr) {
        for (const auto& term : expr) {
            this->plus(-term.second, term.first);
        }
        this->m_constant -= expr.constant();
    }

    /// \brief Unary minus
    LinearExpr operator-() const {
        LinearExpr r(*this);
        r *= -1;
        return r;
    }

    /// \brief Multiply by a constant
    void operator*=(const Num& n) {
        if (n == 0) {
            this->m_terms.clear();
            this->m_constant = 0;
        } else {
            for (auto& term : this->m_terms) {
                term.second *= n;
            }
            this->m_constant *= n;
        }
    }

    /// \brief Multiply by a constant
    void operator*=(int k) {
        if (k == 0) {
            set_to_zero();
        } else {
            for (auto& [var, factor] : this->m_terms) {
                factor *= k;
            }
            this->m_constant *= k;
        }
    }

    /// \brief return the single var x or std::nullopt if not a single var
    std::optional< VarRef > get_as_single_variable() const {
        if (this->m_constant == 0 && this->m_terms.size() == 1) {
            auto it = this->m_terms.begin();
            if (it->second == 1) {
                return it->first;
            }
        }
        return std::nullopt;
    }

    /// \brief Return all variables in the expression
    VarSet get_var_set() const {
        VarSet vars;
        for (const auto& [var, _] : this->m_terms) {
            vars.insert(var);
        }
        return vars;
    }

    void dump(llvm::raw_ostream& os) const {
        bool is_first = true;
        for (auto& [var, factor] : this->m_terms) {
            if (factor > 0 && !is_first) {
                os << "+";
            }
            if (factor == -1) {
                os << "-";
            } else if (factor != 1) {
                os << factor;
            }
            DumpableTrait< VarRef >::dump(os, var);
            is_first = false;
        }
        if (this->m_constant > 0 && !this->m_terms.empty()) {
            os << "+";
        }
        if (this->m_constant != 0 || this->m_terms.empty()) {
            os << this->m_constant;
        }
    }
}; // class LinearExpr

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator*(typename Var::Ref var, Num factor) {
    return LinearExpr< Num, Var >(std::move(factor), var);
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator*(Num factor, typename Var::Ref var) {
    return LinearExpr< Num, Var >(std::move(factor), var);
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator*(LinearExpr< Num, Var > linear_expr,
                                        const Num& k) {
    linear_expr *= k;
    return linear_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator*(const Num& k,
                                        LinearExpr< Num, Var > linear_expr) {
    linear_expr *= k;
    return linear_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator+(typename Var::Ref var, const Num& n) {
    LinearExpr< Num, Var > single_var_expr(var);
    single_var_expr += n;
    return single_var_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator+(const Num& n, typename Var::Ref var) {
    LinearExpr< Num, Var > single_var_expr(var);
    single_var_expr += n;
    return single_var_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator+(typename Var::Ref x,
                                        typename Var::Ref y) {
    LinearExpr< Num, Var > single_var_expr(x);
    single_var_expr += y;
    return single_var_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator+(LinearExpr< Num, Var > linear_expr,
                                        const Num& n) {
    linear_expr += n;
    return linear_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator+(const Num& n,
                                        LinearExpr< Num, Var > linear_expr) {
    linear_expr += n;
    return linear_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator+(LinearExpr< Num, Var > linear_expr,
                                        typename Var::Ref x) {
    linear_expr += x;
    return linear_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator+(typename Var::Ref linear_expr,
                                        LinearExpr< Num, Var > x) {
    x += linear_expr;
    return x;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator+(LinearExpr< Num, Var > x,
                                        const LinearExpr< Num, Var >& y) {
    x += y;
    return x;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator-(typename Var::Ref var, const Num& n) {
    LinearExpr< Num, Var > single_var_expr(var);
    single_var_expr -= n;
    return single_var_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator-(const Num& n, typename Var::Ref var) {
    LinearExpr< Num, Var > single_var_expr(-1, var);
    single_var_expr += n;
    return single_var_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator-(typename Var::Ref x,
                                        typename Var::Ref y) {
    LinearExpr< Num, Var > single_var_expr(x);
    single_var_expr -= y;
    return single_var_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator-(LinearExpr< Num, Var > linear_expr,
                                        const Num& n) {
    linear_expr -= n;
    return linear_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator-(LinearExpr< Num, Var > linear_expr,
                                        typename Var::Ref x) {
    linear_expr -= x;
    return linear_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator-(const Num& n,
                                        LinearExpr< Num, Var > linear_expr) {
    linear_expr *= -1;
    linear_expr += n;
    return linear_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator-(typename Var::Ref x,
                                        LinearExpr< Num, Var > linear_expr) {
    linear_expr *= -1;
    linear_expr += x;
    return linear_expr;
}

template < typename Num, typename Var >
inline LinearExpr< Num, Var > operator-(LinearExpr< Num, Var > x,
                                        const LinearExpr< Num, Var >& y) {
    x -= y;
    return x;
}

/// @}

/// \brief Linear constraint linearconstraintkind
enum class LinearConstraintKind {
    LCK_Equality,    // ==
    LCK_Disequation, // !=
    LCK_Inequality,  // <=
}; // enum class LinearConstraintLinearConstraintKind

template < typename Num, typename Var > class LinearConstraint {
  public:
    using enum LinearConstraintKind;
    using VarRef = Var::Ref;
    using VarSet = std::unordered_set< VarRef >;
    using LinearExpr = knight::dfa::LinearExpr< Var, Num >;

  private:
    LinearExpr m_linear_expr;
    LinearConstraintKind m_kind;

  public:
    LinearConstraint(LinearExpr expr, LinearConstraintKind linearconstraintkind)
        : m_linear_expr(std::move(expr)), m_kind(linearconstraintkind) {}

    LinearConstraint() = delete;

    LinearConstraint(const LinearConstraint&) = default;
    LinearConstraint(LinearConstraint&&) = default;
    LinearConstraint& operator=(const LinearConstraint&) = default;
    LinearConstraint& operator=(LinearConstraint&&) = default;
    ~LinearConstraint() = default;

    /// \brief Create the tautology 0 == 0
    static LinearConstraint tautology() {
        return LinearConstraint(LinearExpr(), LCK_Equality);
    }

    /// \brief Create the contradiction 0 != 0
    static LinearConstraint contradiction() {
        return LinearConstraint(LinearExpr(), LCK_Disequation);
    }

    /// \brief Return true if the constraint is a tautology
    bool is_tautology() const {
        if (!this->m_linear_expr.is_constant()) {
            return false;
        }

        switch (this->m_kind) {
        case LCK_Equality: {
            return this->m_linear_expr.constant() == 0;
        }
        case LCK_Disequation: {
            return this->m_linear_expr.constant() != 0;
        }
        case LCK_Inequality: {
            return this->m_linear_expr.constant() <= 0;
        }
        default:
            break;
        }
        knight_unreachable("Invalid constraint kind");
    }

    /// \brief Return true if the constraint is a contradiction
    bool is_contradiction() const {
        if (!this->m_linear_expr.is_constant()) {
            return false;
        }

        switch (this->m_kind) {
        case LCK_Equality: {
            // x == 0
            return this->m_linear_expr.constant() != 0;
        }
        case LCK_Disequation: {
            // x != 0
            return this->m_linear_expr.constant() == 0;
        }
        case LCK_Inequality: {
            // x <= 0
            return this->m_linear_expr.constant() > 0;
        }
        default:
            break;
        }
        knight_unreachable("Invalid constraint kind");
    }

    bool is_equality() const { return this->m_kind == LCK_Equality; }
    bool is_disequation() const { return this->m_kind == LCK_Disequation; }
    bool is_Inequality() const { return this->m_kind == LCK_Inequality; }

    const LinearExpr& get_linear_expression() const {
        return this->m_linear_expr;
    }

    LinearConstraintKind get_constraint_kind() const { return this->m_kind; }
    Num get_constant_term() const { return -this->m_linear_expr.constant(); }
    std::size_t num_variable_terms() const {
        return this->m_linear_expr.num_variable_terms();
    }

    Num get_factor_of(VarRef var) const {
        return this->m_linear_expr.get_factor_of(var);
    }
    VarSet get_var_set() const { return this->m_linear_expr.get_var_set(); }

    void dump(llvm::raw_ostream& os) const {
        if (this->is_contradiction()) {
            os << "false";
            return;
        }
        if (this->is_tautology()) {
            os << "true";
            return;
        }
        LinearExpr expr =
            this->m_linear_expr - this->m_linear_expr.get_constant_term();
        Num cst = -this->m_linear_expr.get_constant_term();

        expr.dump(os);
        switch (this->m_kind) {
        case LCK_Equality: {
            os << " == ";
            break;
        }
        case LCK_Disequation: {
            os << " != ";
            break;
        }
        case LCK_Inequality: {
            os << " <= ";
            break;
        }
        default: {
            knight_unreachable("Invalid constraint kind");
        }
        }
        os << cst;
    }
}; // class LinearConstraint

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator<=(
    LinearExpr< Num, Var > linear_expr, const Num& n) {
    return LinearConstraint< Num, Var >(std::move(linear_expr) - n,
                                        LinearConstraintKind::LCK_Inequality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator<=(LinearExpr< Num, Var > x,
                                               typename Var::Ref y) {
    return LinearConstraint< Num, Var >(std::move(x) - std::move(y),
                                        LinearConstraintKind::LCK_Inequality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator<=(typename Var::Ref x,
                                               const Num& n) {
    return LinearConstraint< Num,
                             Var >(std::move(x) - n,
                                   LinearConstraint< Num, Var >::Inequality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator<=(typename Var::Ref x,
                                               typename Var::Ref y) {
    return LinearConstraint< Num,
                             Var >(std::move(x) - std::move(y),
                                   LinearConstraint< Num, Var >::Inequality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator<=(typename Var::Ref x,
                                               LinearExpr< Num, Var > y) {
    return LinearConstraint< Num,
                             Var >(std::move(x) - std::move(y),
                                   LinearConstraint< Num, Var >::Inequality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator<=(
    LinearExpr< Num, Var > x, const LinearExpr< Num, Var >& y) {
    return LinearConstraint< Num,
                             Var >(std::move(x) - y,
                                   LinearConstraint< Num, Var >::Inequality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator>=(
    LinearExpr< Num, Var > linear_expr, const Num& n) {
    return LinearConstraint< Num,
                             Var >(n - std::move(linear_expr),
                                   LinearConstraint< Num, Var >::Inequality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator>=(
    LinearExpr< Num, Var > linear_expr, typename Var::Ref x) {
    return LinearConstraint< Num,
                             Var >(std::move(x) - std::move(linear_expr),
                                   LinearConstraint< Num, Var >::Inequality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator>=(typename Var::Ref x,
                                               const Num& n) {
    return LinearConstraint< Num,
                             Var >(n - std::move(x),
                                   LinearConstraint< Num, Var >::Inequality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator>=(typename Var::Ref x,
                                               typename Var::Ref y) {
    return LinearConstraint< Num,
                             Var >(std::move(y) - std::move(x),
                                   LinearConstraint< Num, Var >::Inequality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator>=(
    typename Var::Ref x, LinearExpr< Num, Var > linear_expr) {
    return LinearConstraint< Num,
                             Var >(std::move(linear_expr) - std::move(x),
                                   LinearConstraint< Num, Var >::Inequality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator>=(const LinearExpr< Num, Var >& x,
                                               LinearExpr< Num, Var > y) {
    return LinearConstraint< Num,
                             Var >(std::move(y) - x,
                                   LinearConstraint< Num, Var >::Inequality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator==(
    LinearExpr< Num, Var > linear_expr, const Num& n) {
    return LinearConstraint< Num, Var >(std::move(linear_expr) - n,
                                        LinearConstraint< Num, Var >::Equality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator==(
    LinearExpr< Num, Var > linear_expr, typename Var::Ref x) {
    return LinearConstraint< Num, Var >(std::move(linear_expr) - std::move(x),
                                        LinearConstraint< Num, Var >::Equality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator==(typename Var::Ref x,
                                               const Num& n) {
    return LinearConstraint< Num, Var >(std::move(x) - n,
                                        LinearConstraint< Num, Var >::Equality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator==(typename Var::Ref x,
                                               typename Var::Ref y) {
    return LinearConstraint< Num, Var >(std::move(x) - std::move(y),
                                        LinearConstraint< Num, Var >::Equality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator==(
    typename Var::Ref x, LinearExpr< Num, Var > linear_expr) {
    return LinearConstraint< Num, Var >(std::move(linear_expr) - std::move(x),
                                        LinearConstraint< Num, Var >::Equality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator==(
    LinearExpr< Num, Var > x, const LinearExpr< Num, Var >& y) {
    return LinearConstraint< Num, Var >(std::move(x) - y,
                                        LinearConstraint< Num, Var >::Equality);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator!=(
    LinearExpr< Num, Var > linear_expr, const Num& n) {
    return LinearConstraint< Num,
                             Var >(std::move(linear_expr) - n,
                                   LinearConstraint< Num, Var >::Disequation);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator!=(
    LinearExpr< Num, Var > linear_expr, typename Var::Ref x) {
    return LinearConstraint< Num,
                             Var >(std::move(linear_expr) - std::move(x),
                                   LinearConstraint< Num, Var >::Disequation);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator!=(typename Var::Ref x,
                                               const Num& n) {
    return LinearConstraint< Num,
                             Var >(std::move(x) - n,
                                   LinearConstraint< Num, Var >::Disequation);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator!=(typename Var::Ref x,
                                               typename Var::Ref y) {
    return LinearConstraint< Num,
                             Var >(std::move(x) - std::move(y),
                                   LinearConstraint< Num, Var >::Disequation);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator!=(
    typename Var::Ref x, LinearExpr< Num, Var > linear_expr) {
    return LinearConstraint< Num,
                             Var >(std::move(linear_expr) - std::move(x),
                                   LinearConstraint< Num, Var >::Disequation);
}

template < typename Num, typename Var >
inline LinearConstraint< Num, Var > operator!=(
    LinearExpr< Num, Var > x, const LinearExpr< Num, Var >& y) {
    return LinearConstraint< Num,
                             Var >(std::move(x) - y,
                                   LinearConstraint< Num, Var >::Disequation);
}

template < typename Num, typename Var > class LinearConstraintSystem {
  public:
    using LinearConstraint = LinearConstraint< Num, Var >;
    using VarRef = Var::Ref;
    using VarSet = typename LinearConstraint::VarSet;

  private:
    using LinearConstraints = std::vector< LinearConstraint >;

  private:
    LinearConstraints m_linear_csts;

  public:
    LinearConstraintSystem() = default;
    explicit LinearConstraintSystem(LinearConstraint cst)
        : m_linear_csts{std::move(cst)} {}
    LinearConstraintSystem(std::initializer_list< LinearConstraint > csts)
        : m_linear_csts(std::move(csts)) {}

    LinearConstraintSystem(const LinearConstraintSystem&) = default;
    LinearConstraintSystem(LinearConstraintSystem&&) = default;
    LinearConstraintSystem& operator=(const LinearConstraintSystem&) = default;
    LinearConstraintSystem& operator=(LinearConstraintSystem&&) = default;
    ~LinearConstraintSystem() = default;

    bool is_empty() const { return this->m_linear_csts.empty(); }

    std::size_t size() const { return this->m_linear_csts.size(); }

    void add_linear_constraint(LinearConstraint cst) {
        this->m_linear_csts.emplace_back(std::move(cst));
    }

    void merge_linear_constraint_system(const LinearConstraintSystem& csts) {
        this->m_linear_csts.reserve(this->m_linear_csts.size() + csts.size());
        this->m_linear_csts.insert(this->m_linear_csts.end(),
                                   csts.begin(),
                                   csts.end());
    }

    void merge_linear_constraint_system(LinearConstraintSystem&& csts) {
        this->m_linear_csts.reserve(this->m_linear_csts.size() + csts.size());
        this->m_linear_csts.insert(this->m_linear_csts.end(),
                                   std::make_move_iterator(csts.begin()),
                                   std::make_move_iterator(csts.end()));
    }

    const LinearConstraints& get_linear_constraints() const {
        return m_linear_csts;
    }
    LinearConstraints& get_linear_constraints_ref() { return m_linear_csts; }

    VarSet get_var_set() const {
        VarSet vars;
        for (const LinearConstraint& cst : this->m_linear_csts) {
            for (const auto& term : cst) {
                vars.insert(term.first);
            }
        }
        return vars;
    }

    void dump(llvm::raw_ostream& os) const {
        os << "{";
        for (auto it = this->m_linear_csts.begin(),
                  et = this->m_linear_csts.end();
             it != et;) {
            it->dump(os);
            ++it;
            if (it != et) {
                os << "; ";
            }
        }
        os << "}";
    }

}; // end class LinearConstraintSystem

} // namespace knight::dfa