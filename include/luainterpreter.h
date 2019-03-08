#ifndef LUAINTERPRETER_H
#define LUAINTERPRETER_H

#include "luaast.h"

#include <variant>
#include <string>
#include <memory>

using namespace std;

namespace lua {
namespace rt {

#define EVAL(varname, exp, env) \
val varname; \
if (auto eval_result = (exp)->accept(*this, (env), assign); holds_alternative<string>(eval_result)) {\
    return eval_result; \
} else { \
    varname = get<val>(eval_result); \
}

#define EVALL(varname, exp, env, newval) \
val varname; \
if (auto eval_result = (exp)->accept(*this, (env), newval); holds_alternative<string>(eval_result)) {\
    return eval_result; \
} else { \
    varname = get<val>(eval_result); \
}

struct Environment {
    table t;
    Environment* parent = nullptr;

    void assign(const val& var, const val& newval);
    val getvar(const val& var);

    void populate_stdlib();
};

eval_result_t op_add(val a, val b, const LuaToken& tok = {LuaToken::Type::ADD, ""});
eval_result_t op_sub(val a, val b);
eval_result_t op_mul(val a, val b);
eval_result_t op_div(val a, val b);
eval_result_t op_pow(val a, val b);
eval_result_t op_mod(val a, val b);
eval_result_t op_concat(val a, val b);
eval_result_t op_lt(val a, val b);
eval_result_t op_leq(val a, val b);
eval_result_t op_gt(val a, val b);
eval_result_t op_geq(val a, val b);
eval_result_t op_eq(val a, val b);
eval_result_t op_neq(val a, val b);
eval_result_t op_and(val a, val b);
eval_result_t op_or(val a, val b);
eval_result_t op_len(val v);
eval_result_t op_not(val v);
eval_result_t op_neg(val v, const LuaToken& tok = {LuaToken::Type::SUB, ""});

val fst(const val& v);
vallist flatten(const vallist& list);

struct ASTEvaluator {
    eval_result_t visit(const _LuaAST& ast, Environment& env, const optional<val>&) const {
        return string {"unimplemented"};
    }

    eval_result_t visit(const _LuaName& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaOp& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaUnop& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaExplist& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaFunctioncall& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaAssignment& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaValue& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaNameVar& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaIndexVar& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaMemberVar& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaReturnStmt& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaBreakStmt& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaForStmt& for_stmt, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaLoopStmt& loop_stmt, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaChunk& chunk, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaTableconstructor& stmt, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaFunction& exp, Environment& env, const optional<val>& assign) const;
    eval_result_t visit(const _LuaIfStmt& stmt, Environment& env, const optional<val>& assign) const;
};

struct SourceAssignment {
    LuaToken token;
    string replacement;
};

struct sourceexp {
    virtual optional<vector<SourceAssignment>> forceValue(const val& v) const = 0;
};

struct sourceval : sourceexp {
    static shared_ptr<sourceval> create(const LuaToken& t) {
        auto ptr = make_shared<sourceval>();
        ptr->location = t;
        return ptr;
    }

    optional<vector<SourceAssignment>> forceValue(const val& v) const override{
        return {{SourceAssignment{location, v.to_string()}}};
    }

    LuaToken location;
};

struct sourcebinop : sourceexp {
    static shared_ptr<sourcebinop> create(const val& lhs, const val& rhs, const LuaToken& op) {
        if (!lhs.source || !rhs.source)
            return nullptr;

        auto ptr = make_shared<sourcebinop>();
        ptr->lhs = lhs;
        ptr->rhs = rhs;
        ptr->op = op;
        return ptr;
    }

    optional<vector<SourceAssignment>> forceValue(const val& v) const override {
        if (!holds_alternative<double>(v))
            return nullopt;

        switch (op.type) {
        case LuaToken::Type::ADD:
            if (lhs.source && holds_alternative<double>(rhs)) {
                if (auto result = lhs.source->forceValue(val {get<double>(v) - get<double>(rhs)}); result) {
                    return result;
                }
            }
            if (rhs.source && holds_alternative<double>(lhs)) {
                if (auto result = rhs.source->forceValue(val {get<double>(v) - get<double>(lhs)}); result) {
                    return result;
                }
            }
            return nullopt;
        default:
            return nullopt;
        }
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

    optional<vector<SourceAssignment>> forceValue(const val& new_v) const override {
        if (!holds_alternative<double>(new_v))
            return nullopt;

        switch (op.type) {
        case LuaToken::Type::SUB:
            if (v.source) {
                if (auto result = v.source->forceValue(val {-get<double>(new_v)}); result) {
                    if (auto p = dynamic_pointer_cast<sourceval>(v.source); !p || p->location.pos != op.pos+op.length)
                        return result;
                }
                if (auto result = v.source->forceValue(val {get<double>(new_v)}); result) {
                    result->push_back(SourceAssignment{op, ""});
                    return result;
                }
            }
            return nullopt;
        default:
            return nullopt;
        }
    }

    val v;
    LuaToken op;
};

} // rt
} // lua

#endif // LUAINTERPRETER_H
