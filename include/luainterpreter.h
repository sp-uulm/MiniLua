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

struct SourceChange {
    virtual ~SourceChange();
    virtual string to_string() const = 0;
    virtual vector<LuaToken> apply(vector<LuaToken>&) const = 0;
};

struct SourceChangeOr : SourceChange {
    vector<shared_ptr<SourceChange>> alternatives;

    virtual string to_string() const override;
    virtual vector<LuaToken> apply(vector<LuaToken>&) const override;
};

struct SourceChangeAnd : SourceChange {
    vector<shared_ptr<SourceChange>> changes;

    virtual string to_string() const override;
    virtual vector<LuaToken> apply(vector<LuaToken>&) const override;
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
        return token.to_string() + " -> " + replacement;
    }
    virtual vector<LuaToken> apply(vector<LuaToken>&) const override;
};

struct sourceexp : std::enable_shared_from_this<sourceexp> {
    virtual ~sourceexp();
    virtual source_change_t forceValue(const val& v) const = 0;
    virtual eval_result_t reevaluate() = 0;
    virtual bool isDirty() const = 0;
};

struct sourceval : sourceexp {
    static shared_ptr<sourceval> create(const LuaToken& t) {
        auto ptr = make_shared<sourceval>();
        ptr->location = t;
        return ptr;
    }

    source_change_t forceValue(const val& v) const override;
    eval_result_t reevaluate() override;
    bool isDirty() const override;

    LuaToken location;
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

    val v;
    LuaToken op;
};

} // rt
} // lua

#endif // LUAINTERPRETER_H
