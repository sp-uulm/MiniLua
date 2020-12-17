#include "MiniLua/luainterpreter.hpp"

namespace lua {
namespace rt {

eval_result_t ASTEvaluator::visit(
    const _LuaName& name, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit name" << endl;
    if (assign) {
        env->assign(val{name.token.match}, get<val>(*assign), get<bool>(*assign));
    }
    return eval_success(name.token.match);
}

eval_result_t ASTEvaluator::visit(
    const _LuaOp& op, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit op" << endl;

    EVAL(lhs, op.lhs, env);

    EVAL(rhs, op.rhs, env);

    lhs = fst(lhs);
    rhs = fst(rhs);

    switch (op.op.type) {
    case LuaToken::Type::ADD:
        return op_add(lhs, rhs, op.op) << (lhs_sc & rhs_sc);
    case LuaToken::Type::SUB:
        return op_sub(lhs, rhs, op.op) << (lhs_sc & rhs_sc);
    case LuaToken::Type::MUL:
        return op_mul(lhs, rhs, op.op) << (lhs_sc & rhs_sc);
    case LuaToken::Type::DIV:
        return op_div(lhs, rhs, op.op) << (lhs_sc & rhs_sc);
    case LuaToken::Type::POW:
        return op_pow(lhs, rhs, op.op) << (lhs_sc & rhs_sc);
    case LuaToken::Type::MOD:
        return op_mod(lhs, rhs, op.op) << (lhs_sc & rhs_sc);
    case LuaToken::Type::CONCAT:
        return op_concat(lhs, rhs) << (lhs_sc & rhs_sc);
    case LuaToken::Type::EVAL:
        return op_eval(lhs, rhs, op.op) << (lhs_sc & rhs_sc);
    case LuaToken::Type::LT:
        return op_lt(lhs, rhs) << (lhs_sc & rhs_sc);
    case LuaToken::Type::LEQ:
        return op_leq(lhs, rhs) << (lhs_sc & rhs_sc);
    case LuaToken::Type::GT:
        return op_gt(lhs, rhs) << (lhs_sc & rhs_sc);
    case LuaToken::Type::GEQ:
        return op_geq(lhs, rhs) << (lhs_sc & rhs_sc);
    case LuaToken::Type::EQ:
        return op_eq(lhs, rhs) << (lhs_sc & rhs_sc);
    case LuaToken::Type::NEQ:
        return op_neq(lhs, rhs) << (lhs_sc & rhs_sc);
    case LuaToken::Type::AND:
        return op_and(lhs, rhs) << (lhs_sc & rhs_sc);
    case LuaToken::Type::OR:
        return op_or(lhs, rhs) << (lhs_sc & rhs_sc);
    default:
        return string{op.op.match + " is not a binary operator"};
    }
}

eval_result_t ASTEvaluator::visit(
    const _LuaUnop& op, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit unop" << endl;

    EVAL(rhs, op.exp, env);

    rhs = fst(rhs);

    switch (op.op.type) {
    case LuaToken::Type::SUB:
        return op_neg(rhs, op.op) << rhs_sc;
    case LuaToken::Type::LEN:
        return op_len(rhs) << rhs_sc;
    case LuaToken::Type::NOT:
        return op_not(rhs) << rhs_sc;
    case LuaToken::Type::STRIP:
        return op_strip(rhs) << rhs_sc;
    case LuaToken::Type::EVAL:
        return op_postfix_eval(rhs, op.op) << rhs_sc;
    default:
        return string{op.op.match + " is not a unary operator"};
    }
}

eval_result_t ASTEvaluator::visit(
    const _LuaExplist& explist, const shared_ptr<Environment>& env, const assign_t& assign) const {
    // cout << "visit explist" << endl;

    auto t = make_shared<vallist>();
    source_change_t sc;

    for (unsigned i = 0; i < explist.exps.size(); ++i) {
        if (!assign) {
            EVAL(exp, explist.exps[i], env);
            t->push_back(exp);
            sc = sc & exp_sc;
        } else {
            if (!holds_alternative<vallist_p>(get<val>(*assign)))
                return string{"only a vallist can be assigned to a vallist"};
            bool local = get<bool>(*assign);
            EVALL(
                exp, explist.exps[i], env,
                make_tuple(
                    get<vallist_p>(get<val>(*assign))->size() > i
                        ? get<vallist_p>(get<val>(*assign))->at(i)
                        : nil(),
                    local));
            t->push_back(exp);
            sc = sc & exp_sc;
        }
    }

    return eval_success(t, sc);
}

eval_result_t ASTEvaluator::visit(
    const _LuaFunctioncall& exp, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit functioncall" << endl;

    EVAL(func, exp.function, env);

    EVAL(_args, exp.args, env);
    vallist args = flatten(*get<vallist_p>(_args));

    // call builtin function
    if (holds_alternative<cfunction_p>(func)) {
        auto result = get<cfunction_p>(func)->f(args, exp);

        if (holds_alternative<std::shared_ptr<SourceChange>>(result)) {
            auto change = get<std::shared_ptr<SourceChange>>(result);
            return eval_success(make_shared<vallist>(), func_sc & _args_sc & change);
        } else if (holds_alternative<vallist>(result)) {
            return eval_success(make_shared<vallist>(get<vallist>(result)), func_sc & _args_sc);
        } else {
            return get<string>(result);
        }
    }

    // call lua function
    if (holds_alternative<lfunction_p>(func)) {
        EVALL(
            params, get<lfunction_p>(func)->params, get<lfunction_p>(func)->env,
            make_tuple(make_shared<vallist>(args), true));

        EVAL(result, get<lfunction_p>(func)->f, get<lfunction_p>(func)->env);

        if (holds_alternative<vallist_p>(result))
            return eval_success(result, func_sc & _args_sc & params_sc & result_sc);

        return eval_success(make_shared<vallist>(), func_sc & _args_sc & params_sc);
    }

    if (holds_alternative<nil>(func)) {
        return string{"attempted to call a nil value"};
    }

    return string{"functioncall unimplemented"};
}

eval_result_t ASTEvaluator::visit(
    const _LuaAssignment& assignment, const shared_ptr<Environment>& env,
    const assign_t& assign) const {
    //    cout << "visit assignment" << assignment.local << endl;

    EVAL(_exps, assignment.explist, env);
    vallist exps = flatten(*get<vallist_p>(_exps));
    EVALL(_vars, assignment.varlist, env, make_tuple(make_shared<vallist>(exps), assignment.local));

    return eval_success(nil(), _exps_sc & _vars_sc);
}

eval_result_t ASTEvaluator::visit(
    const _LuaNameVar& var, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit namevar " << var.name->token << endl;

    EVAL(name, var.name, env);

    return eval_success(env->getvar(name), name_sc);
}

eval_result_t ASTEvaluator::visit(
    const _LuaIndexVar& var, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit indexvar" << endl;

    EVALR(index, var.index, env);
    EVALR(table, var.table, env);

    table = fst(table);

    if (holds_alternative<table_p>(table)) {

        if (assign) {
            (*get<table_p>(table))[index] = get<val>(*assign);
        }

        return eval_success((*get<table_p>(table))[index], index_sc & table_sc);
    } else {
        return string{"cannot access index on " + table.type()};
    }
}

eval_result_t ASTEvaluator::visit(
    const _LuaMemberVar& var, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit membervar" << endl;
    EVAL(index, var.member, env);
    EVALR(table, var.table, env);

    table = fst(table);

    if (holds_alternative<table_p>(fst(table))) {

        if (assign) {
            (*get<table_p>(table))[index] = get<val>(*assign);
        }

        return eval_success((*get<table_p>(table))[index], index_sc & table_sc);
    } else {
        return string{"cannot access member on " + table.type()};
    }
}

eval_result_t ASTEvaluator::visit(
    const _LuaReturnStmt& stmt, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit returnstmt" << endl;

    EVAL(result, stmt.explist, env);
    return eval_success(make_shared<vallist>(flatten(*get<vallist_p>(result))), result_sc);
}

eval_result_t ASTEvaluator::visit(
    const _LuaBreakStmt& stmt, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit breakstmt" << endl;
    return eval_success(true);
}

eval_result_t ASTEvaluator::visit(
    const _LuaValue& value, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit value " << value.token << endl;

    switch (value.token.type) {
    case LuaToken::Type::NIL:
        return eval_success(val{nil(), sourceval::create(value.token)});
    case LuaToken::Type::FALSE:
        return eval_success(val{false, sourceval::create(value.token)});
    case LuaToken::Type::TRUE:
        return eval_success(val{true, sourceval::create(value.token)});
    case LuaToken::Type::NUMLIT:
        try {
            return eval_success(
                val{atof(("0" + value.token.match).c_str()), sourceval::create(value.token)});
        } catch (const invalid_argument& invalid) {
            return string{"invalid_argument to stod: "} + invalid.what();
        }

    case LuaToken::Type::STRINGLIT:
        return eval_success(
            val{string(value.token.match.begin() + 1, value.token.match.end() - 1),
                sourceval::create(value.token)});
    default:
        return string{"value unimplemented"};
    }
}

eval_result_t ASTEvaluator::visit(
    const _LuaChunk& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit chunk" << endl;

    source_change_t sc;

    for (const auto& stmt : chunk.statements) {
        EVAL(result, stmt, env);

        sc = sc & result_sc;

        if (!holds_alternative<nil>(result) && !dynamic_pointer_cast<_LuaFunctioncall>(stmt)) {
            return eval_success(result, sc);
        }
    }

    return eval_success(nil(), sc);
}

eval_result_t ASTEvaluator::visit(
    const _LuaForStmt& for_stmt, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit for" << endl;

    auto newenv = make_shared<lua::rt::Environment>(env);

    source_change_t sc;

    EVAL(start, for_stmt.start, env);
    EVALL(var, for_stmt.var, env, make_tuple(start, true));

    sc = sc & start_sc & var_sc;

    for (;;) {
        val current = newenv->getvar(var);

        EVAL(end, for_stmt.end, newenv);
        sc = sc & end_sc;

        auto gt = op_gt(current, fst(end));
        if (holds_alternative<string>(gt)) {
            return gt;
        }

        // loop end reached
        if (get<bool>(get_val(gt)))
            return eval_success(nil(), sc);

        EVAL(result, for_stmt.body, newenv);
        sc = sc & result_sc;

        // return statement in body
        if (holds_alternative<vallist_p>(result))
            return eval_success(result, sc);

        // break statement in body
        if (holds_alternative<bool>(result))
            return eval_success(nil(), sc);

        // increment loop variable
        EVAL(step, for_stmt.step, newenv);
        sc = sc & step_sc;

        auto sum = op_add(current, step);
        if (holds_alternative<eval_success_t>(sum)) {
            EVALL(var, for_stmt.var, env, make_tuple(get_val(sum), true));
            sc = sc & var_sc;
        } else {
            return sum;
        }
    }
}

eval_result_t ASTEvaluator::visit(
    const _LuaLoopStmt& loop_stmt, const shared_ptr<Environment>& env,
    const assign_t& assign) const {
    //    cout << "visit loop" << endl;

    source_change_t sc;

    if (loop_stmt.head_controlled) {
        EVAL(condition, loop_stmt.end, env);
        sc = sc & condition_sc;

        auto neq = op_neq(val{true}, condition);
        if (holds_alternative<string>(neq)) {
            return neq;
        }
        if (get<bool>(get_val(neq))) {
            return eval_success(nil(), sc);
        }
    }

    for (;;) {
        auto newenv = make_shared<lua::rt::Environment>(env);

        EVAL(result, loop_stmt.body, newenv);
        sc = sc & result_sc;

        // return statement in body
        if (holds_alternative<vallist_p>(result))
            return eval_success(result);

        // break statement in body
        if (holds_alternative<bool>(result))
            return eval_success(nil());

        // check loop condition
        EVAL(condition, loop_stmt.end, newenv);
        sc = sc & condition_sc;

        auto neq = op_neq(val{true}, condition);
        if (holds_alternative<string>(neq)) {
            return neq;
        }
        if (get<bool>(get_val(neq))) {
            return eval_success(nil(), sc);
        }
    }
}

eval_result_t ASTEvaluator::visit(
    const _LuaTableconstructor& tableconst, const shared_ptr<Environment>& env,
    const assign_t& assign) const {
    //    cout << "visit tableconstructor" << endl;
    table_p result = make_shared<table>();
    source_change_t sc;

    double default_idx = 1.0;
    for (const LuaField& field : tableconst.fields) {
        EVAL(rhs, field->rhs, env);
        sc = sc & rhs_sc;

        if (!field->lhs) {
            (*result)[val(default_idx)] = rhs;
            default_idx++;
        } else {
            EVAL(lhs, field->lhs, env);
            sc = sc & lhs_sc;

            (*result)[lhs] = rhs;
        }
    }

    val _result = result;
    _result.source = sourceval::create(tableconst.tokens);

    return eval_success(_result, sc);
}

eval_result_t ASTEvaluator::visit(
    const _LuaFunction& exp, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit function" << endl;

    return eval_success(
        make_shared<lfunction>(exp.body, exp.params, make_shared<Environment>(env)));
}

eval_result_t ASTEvaluator::visit(
    const _LuaIfStmt& stmt, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit if" << endl;

    source_change_t sc;

    for (const auto& branch : stmt.branches) {
        EVAL(condition, branch.first, env);
        sc = sc & condition_sc;

        if (condition.to_bool()) {
            auto newenv = make_shared<lua::rt::Environment>(env);

            EVAL(result, branch.second, newenv);
            sc = sc & result_sc;

            // break or return statement in body
            if (!holds_alternative<nil>(result))
                return eval_success(result, sc);
            break;
        }
    }

    return eval_success(nil(), sc);
}

eval_result_t ASTEvaluator::visit(
    const _LuaComment& comment, const shared_ptr<Environment>& env, const assign_t& assign) const {
    //    cout << "visit function" << endl;

    source_change_t sc;
    return eval_success(nil(), sc);
}

} // namespace rt
} // namespace lua
