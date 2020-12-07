#ifndef LUAINTERPRETER_H
#define LUAINTERPRETER_H

#include "environment.hpp"
#include "luaast.hpp"
#include "operators.hpp"
#include "sourcechange.hpp"
#include "sourceexp.hpp"

#include <memory>

using namespace std;

namespace lua {
namespace rt {

#define EVAL(varname, exp, env)                                                                    \
    val varname;                                                                                   \
    source_change_t varname##_sc;                                                                  \
    if (auto eval_result = (exp)->accept(*this, (env), assign);                                    \
        holds_alternative<string>(eval_result)) {                                                  \
        return eval_result;                                                                        \
    } else {                                                                                       \
        varname = get_val(eval_result);                                                            \
        varname##_sc = get_sc(eval_result);                                                        \
    }

#define EVALR(varname, exp, env)                                                                   \
    val varname;                                                                                   \
    source_change_t varname##_sc;                                                                  \
    if (auto eval_result = (exp)->accept(*this, (env), {});                                        \
        holds_alternative<string>(eval_result)) {                                                  \
        return eval_result;                                                                        \
    } else {                                                                                       \
        varname = get_val(eval_result);                                                            \
        varname##_sc = get_sc(eval_result);                                                        \
    }

#define EVALL(varname, exp, env, newval)                                                           \
    val varname;                                                                                   \
    source_change_t varname##_sc;                                                                  \
    if (auto eval_result = (exp)->accept(*this, (env), newval);                                    \
        holds_alternative<string>(eval_result)) {                                                  \
        return eval_result;                                                                        \
    } else {                                                                                       \
        varname = get_val(eval_result);                                                            \
        varname##_sc = get_sc(eval_result);                                                        \
    }

struct ASTEvaluator {
    eval_result_t visit(const _LuaAST&, const shared_ptr<Environment>&, const assign_t&) const {
        return string{"unimplemented10"};
    }

    eval_result_t
    visit(const _LuaName& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t
    visit(const _LuaOp& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t
    visit(const _LuaUnop& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(
        const _LuaExplist& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(
        const _LuaFunctioncall& chunk, const shared_ptr<Environment>& env,
        const assign_t& assign) const;
    eval_result_t visit(
        const _LuaAssignment& chunk, const shared_ptr<Environment>& env,
        const assign_t& assign) const;
    eval_result_t
    visit(const _LuaValue& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(
        const _LuaNameVar& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(
        const _LuaIndexVar& chunk, const shared_ptr<Environment>& env,
        const assign_t& assign) const;
    eval_result_t visit(
        const _LuaMemberVar& chunk, const shared_ptr<Environment>& env,
        const assign_t& assign) const;
    eval_result_t visit(
        const _LuaReturnStmt& chunk, const shared_ptr<Environment>& env,
        const assign_t& assign) const;
    eval_result_t visit(
        const _LuaBreakStmt& chunk, const shared_ptr<Environment>& env,
        const assign_t& assign) const;
    eval_result_t visit(
        const _LuaForStmt& for_stmt, const shared_ptr<Environment>& env,
        const assign_t& assign) const;
    eval_result_t visit(
        const _LuaLoopStmt& loop_stmt, const shared_ptr<Environment>& env,
        const assign_t& assign) const;
    eval_result_t
    visit(const _LuaChunk& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(
        const _LuaTableconstructor& stmt, const shared_ptr<Environment>& env,
        const assign_t& assign) const;
    eval_result_t visit(
        const _LuaFunction& exp, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t
    visit(const _LuaIfStmt& stmt, const shared_ptr<Environment>& env, const assign_t& assign) const;
    eval_result_t visit(
        const _LuaComment& stmt, const shared_ptr<Environment>& env, const assign_t& assign) const;
};

} // namespace rt
} // namespace lua

#endif // LUAINTERPRETER_H
