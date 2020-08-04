#ifndef LUAINTERPRETER_H
#define LUAINTERPRETER_H

#include "luaast.h"

#include <variant>
#include <string>
#include <memory>
#include <cmath>

using namespace std;

namespace lua {
namespace rt {

#define EVAL(varname, exp, env) \
val varname; \
source_change_t varname##_sc; \
if (auto eval_result = (exp)->accept(*this, (env), assign); holds_alternative<string>(eval_result)) {\
    return eval_result; \
} else { \
    varname = get_val(eval_result); \
    varname##_sc = get_sc(eval_result); \
}

#define EVALR(varname, exp, env) \
val varname; \
source_change_t varname##_sc; \
if (auto eval_result = (exp)->accept(*this, (env), {}); holds_alternative<string>(eval_result)) {\
    return eval_result; \
} else { \
    varname = get_val(eval_result); \
    varname##_sc = get_sc(eval_result); \
}

#define EVALL(varname, exp, env, newval) \
val varname; \
source_change_t varname##_sc; \
if (auto eval_result = (exp)->accept(*this, (env), newval); holds_alternative<string>(eval_result)) {\
    return eval_result; \
} else { \
    varname = get_val(eval_result); \
    varname##_sc = get_sc(eval_result); \
}

struct Environment : enable_shared_from_this<Environment> {
private:
    table t;
    shared_ptr<Environment> parent;
    table* global = nullptr;

public:
    Environment(const shared_ptr<Environment>& parent) : parent{parent} {
        if (parent) {
            global = parent->global;
        } else {
            global = &t;
        }
    }

    void clear() {
        t.clear();
    }

    void assign(const val& var, const val& newval, bool is_local);
    val getvar(const val& var);

    void populate_stdlib();
};

eval_result_t op_add(val a, val b, const LuaToken& tok = {LuaToken::Type::ADD, ""});
inline val operator+(const val& a, const val& b) {
    return unwrap(op_add(a, b));
}

eval_result_t op_sub(val a, val b, const LuaToken& tok = {LuaToken::Type::SUB, ""});
inline val operator-(const val& a, const val& b) {
    return unwrap(op_sub(a, b));
}

eval_result_t op_mul(val a, val b, const LuaToken& tok = {LuaToken::Type::MUL, ""});
inline val operator*(const val& a, const val& b) {
    return unwrap(op_mul(a, b));
}

eval_result_t op_div(val a, val b, const LuaToken& tok = {LuaToken::Type::DIV, ""});
inline val operator/(const val& a, const val& b) {
    return unwrap(op_div(a, b));
}

eval_result_t op_pow(val a, val b, const LuaToken& tok = {LuaToken::Type::POW, ""});
inline val operator^(const val& a, const val& b) {
    return unwrap(op_pow(a, b));
}

eval_result_t op_mod(val a, val b, const LuaToken& tok = {LuaToken::Type::MOD, ""});
eval_result_t op_concat(val a, val b);
eval_result_t op_eval(val a, val b, const LuaToken& tok = {LuaToken::Type::EVAL, ""});
eval_result_t op_postfix_eval(val a, const LuaToken& tok = {LuaToken::Type::EVAL, ""});

eval_result_t op_lt(val a, val b);
inline bool operator<(const val& a, const val& b) {
    return get<bool>(unwrap(op_lt(a, b)));
}

eval_result_t op_leq(val a, val b);
inline bool operator<=(const val& a, const val& b) {
    return get<bool>(unwrap(op_leq(a, b)));
}

eval_result_t op_gt(val a, val b);
inline bool operator>(const val& a, const val& b) {
    return get<bool>(unwrap(op_gt(a, b)));
}

eval_result_t op_geq(val a, val b);
inline bool operator>=(const val& a, const val& b) {
    return get<bool>(unwrap(op_geq(a, b)));
}

eval_result_t op_eq(val a, val b);
inline bool operator==(const val& a, const val& b) {
    return get<bool>(unwrap(op_eq(a, b)));
}

eval_result_t op_neq(val a, val b);
inline bool operator!=(const val& a, const val& b) {
    return get<bool>(unwrap(op_neq(a, b)));
}

eval_result_t op_and(val a, val b);
inline val operator&&(const val& a, const val& b) {
    return unwrap(op_and(a, b));
}

eval_result_t op_or(val a, val b);
inline val operator||(const val& a, const val& b) {
    return unwrap(op_or(a, b));
}

eval_result_t op_len(val v);

eval_result_t op_not(val v);
inline bool operator!(const val& a) {
    return get<bool>(unwrap(op_not(a)));
}

eval_result_t op_neg(val v, const LuaToken& tok = {LuaToken::Type::SUB, ""});
inline val operator-(const val& a) {
    return unwrap(op_neg(a));
}

eval_result_t op_sqrt(val v);
eval_result_t op_strip(val v);

val fst(const val& v);
vallist flatten(const vallist& list);

struct ASTEvaluator {
    eval_result_t visit(const _LuaAST&, const shared_ptr<Environment>&, const assign_t&) const {
        return string {"unimplemented"};
    }

    eval_result_t visit(const _LuaName& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaOp& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaUnop& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaExplist& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaFunctioncall& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaAssignment& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaValue& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaNameVar& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaIndexVar& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaMemberVar& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaReturnStmt& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaBreakStmt& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaForStmt& for_stmt, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaLoopStmt& loop_stmt, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaChunk& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaTableconstructor& stmt, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaFunction& exp, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(const _LuaIfStmt& stmt, const shared_ptr<Environment>& env, const assign_t& assign) const;
};

struct SourceChangeVisitor {
    virtual ~SourceChangeVisitor();

    virtual void visit(const struct SourceChangeOr& sc) = 0;
    virtual void visit(const struct SourceChangeAnd& sc) = 0;
    virtual void visit(const struct SourceAssignment& sc) = 0;
};

struct ApplySCVisitor : public SourceChangeVisitor {
    void visit(const SourceChangeOr& sc) override;
    void visit(const SourceChangeAnd& sc) override;
    void visit(const SourceAssignment& sc) override;

    vector<LuaToken> apply_changes(const vector<LuaToken>& tokens);

    std::vector<SourceAssignment> changes;
};

struct SourceChange {
    virtual ~SourceChange();
    virtual string to_string() const = 0;

    vector<LuaToken> apply(const vector<LuaToken>& tokens) {
        ApplySCVisitor vis;
        accept(vis);

        return vis.apply_changes(tokens);
    }

    virtual void accept(SourceChangeVisitor& v) const = 0;
    std::string hint = "?";
};

struct SourceChangeOr : SourceChange {
    vector<shared_ptr<SourceChange>> alternatives;

    virtual string to_string() const override;

    virtual void accept(SourceChangeVisitor& v) const override {
        v.visit(*this);
    }
};

struct SourceChangeAnd : SourceChange {
    vector<shared_ptr<SourceChange>> changes;

    virtual string to_string() const override;

    virtual void accept(SourceChangeVisitor& v) const override {
        v.visit(*this);
    }
};

struct SourceAssignment : SourceChange {
    static shared_ptr<SourceAssignment> create(const LuaToken& token, const string& replacement) {
        auto result = make_shared<SourceAssignment>();
        result->token = token;
        result->replacement = replacement;
        return result;
    }

    LuaToken token;
    string replacement;

    virtual string to_string() const override {
        return token.to_string() + " -> " + replacement + " [" + hint + "]";
    }

    virtual void accept(SourceChangeVisitor& v) const override {
        v.visit(*this);
    }
};

inline source_change_t operator| (const source_change_t& lhs, const source_change_t& rhs) {
    if (lhs && rhs) {
        auto sc_or = make_shared<SourceChangeOr>();
        sc_or->alternatives = {*lhs, *rhs};

        return move(sc_or);
    }

    return lhs ? lhs : rhs;
}

inline source_change_t operator& (const source_change_t& lhs, const source_change_t& rhs) {
    if (lhs && rhs) {
        auto sc_and = make_shared<SourceChangeAnd>();
        sc_and->changes = {*lhs, *rhs};

        return move(sc_and);
    }

    return lhs ? lhs : rhs;
}

// adds a source change to an eval_result_t
inline eval_result_t operator<< (const eval_result_t& lhs, const source_change_t& rhs) {
    if (holds_alternative<string>(lhs))
        return lhs;

    return eval_success(get_val(lhs), get_sc(lhs) & rhs);
}

struct sourceexp : std::enable_shared_from_this<sourceexp> {
    virtual ~sourceexp();
    virtual source_change_t forceValue(const val& v) const = 0;
    virtual eval_result_t reevaluate() = 0;
    virtual bool isDirty() const = 0;
    virtual vector<LuaToken> get_all_tokens() const = 0;

    string identifier = "";
};

struct sourceval : sourceexp {
    static shared_ptr<sourceval> create(const LuaToken& t) {
        auto ptr = make_shared<sourceval>();
        ptr->location.push_back(t);
        return ptr;
    }

    static shared_ptr<sourceval> create(const vector<LuaToken>& t) {
        auto ptr = make_shared<sourceval>();
        ptr->location = t;
        return ptr;
    }

    source_change_t forceValue(const val& v) const override;
    eval_result_t reevaluate() override;
    bool isDirty() const override;

    vector<LuaToken> get_all_tokens() const override {
        return location;
    }

    vector<LuaToken> location;
};

struct sourcebinop : sourceexp {
    static shared_ptr<sourcebinop> create(const val& lhs, const val& rhs, const LuaToken& op) {
        if (!lhs.source && !rhs.source)
            return nullptr;

        auto ptr = make_shared<sourcebinop>();
        ptr->lhs = lhs;
        ptr->rhs = rhs;
        ptr->op = op;
        return ptr;
    }

    source_change_t forceValue(const val& v) const override;
    eval_result_t reevaluate() override;
    bool isDirty() const override;

    vector<LuaToken> get_all_tokens() const override {
        vector<LuaToken> result = {op};
        if (lhs.source) {
            auto lhs_tokens = lhs.source->get_all_tokens();
            result.insert(end(result), begin(lhs_tokens), end(lhs_tokens));
        }
        if (rhs.source) {
            auto rhs_tokens = rhs.source->get_all_tokens();
            result.insert(end(result), begin(rhs_tokens), end(rhs_tokens));
        }
        return result;
    }

    val lhs;
    val rhs;
    LuaToken op;
};

struct sourceunop : sourceexp {
    static shared_ptr<sourceunop> create(const val& v, const LuaToken& op) {
        if (!v.source)
            return nullptr;

        auto ptr = make_shared<sourceunop>();
        ptr->v = v;
        ptr->op = op;
        return ptr;
    }

    source_change_t forceValue(const val& new_v) const override;
    eval_result_t reevaluate() override;
    bool isDirty() const override;

    vector<LuaToken> get_all_tokens() const override {
        vector<LuaToken> result = {op};
        if (v.source) {
            auto v_tokens = v.source->get_all_tokens();
            result.insert(end(result), begin(v_tokens), end(v_tokens));
        }
        return result;
    }

    val v;
    LuaToken op;
};

} // rt
} // lua

#endif // LUAINTERPRETER_H
