#ifndef LUAINTERPRETER_H
#define LUAINTERPRETER_H

#include "luaast.h"

#include <variant>
#include <string>
#include <memory>

using namespace std;

namespace lua {
namespace rt {

#define EVAL(varname, exp, eval, env) \
val varname; \
if (auto eval_result = (exp)->accept((eval), (env), rvalue); holds_alternative<string>(eval_result)) {\
    return eval_result; \
} else { \
    varname = get<val>(eval_result); \
}

#define EVALL(varname, exp, eval, env) \
val varname; \
if (auto eval_result = (exp)->accept((eval), (env), false); holds_alternative<string>(eval_result)) {\
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

eval_result_t op_add(val a, val b);
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
eval_result_t op_neg(val v);

val fst(const val& v);
vallist flatten(const vallist& list);

struct ASTEvaluator {
    eval_result_t visit(const _LuaAST& ast, Environment& env, bool rvalue) const {
        return string {"unimplemented"};
    }

    eval_result_t visit(const _LuaName& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaOp& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaUnop& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaExplist& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaFunctioncall& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaAssignment& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaValue& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaNameVar& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaIndexVar& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaMemberVar& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaReturnStmt& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaBreakStmt& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaForStmt& for_stmt, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaLoopStmt& loop_stmt, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaChunk& chunk, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaTableconstructor& stmt, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaFunction& exp, Environment& env, bool rvalue) const;
    eval_result_t visit(const _LuaIfStmt& stmt, Environment& env, bool rvalue) const;
};

} // rt
} // lua

#endif // LUAINTERPRETER_H
