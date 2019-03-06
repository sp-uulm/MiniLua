#include "include/luainterpreter.h"
#include <sstream>
#include <cmath>

namespace lua {
namespace rt {

eval_result_t op_add(lua::rt::val a, lua::rt::val b) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {get<double>(a) + get<double>(b)};

    return string{"could not add values of type other than number (" + to_string(a.index()) + ")"};
}

eval_result_t op_sub(lua::rt::val a, lua::rt::val b) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {get<double>(a) - get<double>(b)};

    return string{"could not subtract variables of type other than number"};
}

eval_result_t op_mul(lua::rt::val a, lua::rt::val b) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {get<double>(a) * get<double>(b)};

    return string{"could not multiply variables of type other than number"};
}

eval_result_t op_div(lua::rt::val a, lua::rt::val b) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {get<double>(a) / get<double>(b)};

    return string{"could not divide variables of type other than number"};
}

eval_result_t op_pow(lua::rt::val a, lua::rt::val b) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {pow(get<double>(a), get<double>(b))};

    return string{"could not exponentiate variables of type other than number"};
}

eval_result_t op_mod(lua::rt::val a, lua::rt::val b) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {fmod(get<double>(a), get<double>(b))};

    return string{"could not mod variables of type other than number"};
}

eval_result_t op_concat(lua::rt::val a, lua::rt::val b) {
    if ((holds_alternative<double>(a) || holds_alternative<string>(a)) &&
        (holds_alternative<double>(b) || holds_alternative<string>(b))) {

        stringstream ss;
        ss << a << b;
        return lua::rt::val {ss.str()};
    }

    return string{"could not concatenate other types than strings or numbers"};
}

eval_result_t op_lt(lua::rt::val a, lua::rt::val b) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {get<double>(a) < get<double>(b)};

    if (holds_alternative<string>(a) && holds_alternative<string>(b))
        return lua::rt::val {get<string>(a) < get<string>(b)};

    return string{"only strings and numbers can be compared"};
}

eval_result_t op_leq(lua::rt::val a, lua::rt::val b) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {get<double>(a) <= get<double>(b)};

    if (holds_alternative<string>(a) && holds_alternative<string>(b))
        return lua::rt::val {get<string>(a) <= get<string>(b)};

    return string{"only strings and numbers can be compared"};
}

eval_result_t op_gt(lua::rt::val a, lua::rt::val b) {
    auto leq = op_leq(a, b);

    if (holds_alternative<val>(leq))
        return op_not(get<val>(leq));

    return string{"only strings and numbers can be compared"};
}

eval_result_t op_geq(lua::rt::val a, lua::rt::val b) {
    auto leq = op_lt(a, b);

    if (holds_alternative<val>(leq))
        return op_not(get<val>(leq));

    return string{"only strings and numbers can be compared"};
}

eval_result_t op_eq(lua::rt::val a, lua::rt::val b) {
    if (a.index() != b.index())
        return val {false};

    return visit([&b](auto&& a){
        using T = std::decay_t<decltype(a)>;
        return val {a == get<T>(b)};
    }, a);
}

eval_result_t op_neq(val a, val b) {
    return op_not(get<val>(op_eq(a, b)));
}

eval_result_t op_and(val a, val b) {
    return string{"op_and unimplemented"};
}

eval_result_t op_or(val a, val b) {
    return string{"op_or unimplemented"};
}

eval_result_t op_len(val v) {
    return string{"op_len unimplemented"};
}

eval_result_t op_not(val v) {
    if (holds_alternative<nil>(v))
        return val {true};

    if (holds_alternative<bool>(v))
        return val {!get<bool>(v)};

    return val {false};
}

eval_result_t op_neg(val v) {
    if (holds_alternative<double>(v)) {
        return val {-get<double>(v)};
    }

    return string{"unary - can only be applied to a number"};
}

val fst(const val& v) {
    if (holds_alternative<vallist_p>(v)) {
        const vallist& vl = *get<vallist_p>(v);
        return vl.size() > 0 ? vl[0] : nil();
    }
    return v;
}

vallist flatten(const vallist& list) {
    if (list.size() == 0)
        return {};

    vallist result;

    for(int i = 0; i < static_cast<int>(list.size())-1; ++i) {
        result.push_back(fst(list[i]));
    }

    if (holds_alternative<vallist_p>(list.back())) {
        const vallist& vl = *get<vallist_p>(list.back());
        for(unsigned i = 0; i < vl.size(); ++i) {
            result.push_back(vl[i]);
        }
    } else {
        result.push_back(list.back());
    }

    return result;
}

void Environment::assign(const val& var, const val& newval) {
    // search environments for variable
    for (Environment *env = this; env != nullptr; env = env->parent) {
        if (env->t.count(var)) {
            env->t[var] = newval;
            return;
        }
    }

    // not yet assigned, assign to local environment
    t[var] = newval;
}

val Environment::getvar(const val& var) {
    // search environments for variable
    for (Environment *env = this; env != nullptr; env = env->parent) {
        if (env->t.count(var)) {
            return env->t[var];
        }
    }
    return nil();
}

void Environment::populate_stdlib() {
    t[string {"print"}] = make_shared<cfunction>([](const vallist& args) -> vallist {
        for (int i = 0; i < static_cast<int>(args.size()) - 1; ++i) {
            cout << args[i] << "\t";
        }
        if (args.size() > 0) {
            cout << args.back();
        }
        cout << endl;
        return {};
    });
}

eval_result_t ASTEvaluator::visit(const _LuaName& name, Environment& env, bool rvalue) const {
    cout << "visit name" << endl;
    return val{name.token.match};
}

eval_result_t ASTEvaluator::visit(const _LuaOp& op, Environment& env, bool rvalue) const {
    cout << "visit op" << endl;

    EVAL(lhs, op.lhs, *this, env);

    EVAL(rhs, op.rhs, *this, env);

    lhs = fst(lhs);
    rhs = fst(rhs);

    switch (op.op.type) {
    case LuaToken::Type::ADD:
        return op_add(lhs, rhs);
    case LuaToken::Type::SUB:
        return op_sub(lhs, rhs);
    case LuaToken::Type::MUL:
        return op_mul(lhs, rhs);
    case LuaToken::Type::DIV:
        return op_div(lhs, rhs);
    case LuaToken::Type::POW:
        return op_pow(lhs, rhs);
    case LuaToken::Type::MOD:
        return op_mod(lhs, rhs);
    case LuaToken::Type::CONCAT:
        return op_concat(lhs, rhs);
    case LuaToken::Type::LT:
        return op_lt(lhs, rhs);
    case LuaToken::Type::LEQ:
        return op_leq(lhs, rhs);
    case LuaToken::Type::GT:
        return op_gt(lhs, rhs);
    case LuaToken::Type::GEQ:
        return op_geq(lhs, rhs);
    case LuaToken::Type::EQ:
        return op_eq(lhs, rhs);
    case LuaToken::Type::NEQ:
        return op_neq(lhs, rhs);
    case LuaToken::Type::AND:
        return op_and(lhs, rhs);
    case LuaToken::Type::OR:
        return op_or(lhs, rhs);
    default:
        return string {op.op.match + " is not a binary operator"};
    }
}

eval_result_t ASTEvaluator::visit(const _LuaUnop& op, Environment& env, bool rvalue) const {
    cout << "visit unop" << endl;

    EVAL(rhs, op.exp, *this, env);

    rhs = fst(rhs);

    switch (op.op.type) {
    case LuaToken::Type::SUB:
        return op_neg(rhs);
    case LuaToken::Type::LEN:
        return op_len(rhs);
    case LuaToken::Type::NOT:
        return op_not(rhs);
    default:
        return string {op.op.match + " is not a unary operator"};
    }
}

eval_result_t ASTEvaluator::visit(const _LuaExplist& explist, Environment& env, bool rvalue) const {
    cout << "visit explist" << endl;

    auto t = make_shared<vallist>();
    for (unsigned i = 0; i < explist.exps.size(); ++i) {
        EVAL(exp, explist.exps[i], *this, env);

        t->push_back(exp);
    }

    return t;
}

eval_result_t ASTEvaluator::visit(const _LuaFunctioncall& exp, Environment& env, bool rvalue) const {
    cout << "visit functioncall" << endl;

    EVAL(func, exp.function, *this, env);

    EVAL(_args, exp.args, *this, env);
    vallist args = flatten(*get<vallist_p>(_args));

    // call builtin function
    if (holds_alternative<cfunction_p>(func)) {
        return make_shared<vallist>(get<cfunction_p>(func)->f(args));
    }

    // call lua function
    if (holds_alternative<lfunction_p>(func)) {
        Environment new_env;
        new_env.parent = &env;

        for (unsigned i = 0; i < get<lfunction_p>(func)->params.size(); ++i) {
            new_env.assign(get<lfunction_p>(func)->params[i], args.size() > i ? args[i] : nil());
        }

        EVAL(result, get<lfunction_p>(func)->f, *this, new_env);

        if (holds_alternative<vallist_p>(result))
            return result;

        return make_shared<vallist>();
    }

    if (holds_alternative<nil>(func)) {
        return string {"attempted to call a nil value"};
    }

    return string{"functioncall unimplemented"};
}

eval_result_t ASTEvaluator::visit(const _LuaAssignment &assignment, Environment &env, bool rvalue) const {
    cout << "visit assignment" << endl;

    EVALL(_vars, assignment.varlist, *this, env);
    vallist_p vars = get<vallist_p>(_vars);

    EVAL(_exps, assignment.explist, *this, env);
    vallist exps = flatten(*get<vallist_p>(_exps));

    for (unsigned i = 0; i < vars->size(); ++i) {
        env.assign(vars->at(i), exps.size() > i ? exps[i] : nil());
    }

    return nil();
}

eval_result_t ASTEvaluator::visit(const _LuaNameVar& var, Environment& env, bool rvalue) const {
    cout << "visit namevar" << endl;

    EVAL(name, var.name, *this, env);

    if (rvalue)
        return env.getvar(name);
    else
        return name;
}

eval_result_t ASTEvaluator::visit(const _LuaIndexVar& var, Environment& env, bool rvalue) const {
    cout << "visit indexvar" << endl;
    return string{"value unimplemented"};
}

eval_result_t ASTEvaluator::visit(const _LuaMemberVar& var, Environment& env, bool rvalue) const {
    cout << "visit membervar" << endl;
    return string{"value unimplemented"};
}

eval_result_t ASTEvaluator::visit(const _LuaReturnStmt& stmt, Environment& env, bool rvalue) const {
    cout << "visit returnstmt" << endl;

    EVAL(result, stmt.explist, *this, env);
    return make_shared<vallist>(flatten(*get<vallist_p>(result)));
}

eval_result_t ASTEvaluator::visit(const _LuaBreakStmt& stmt, Environment& env, bool rvalue) const {
    cout << "visit breakstmt" << endl;
    return val{true};
}

eval_result_t ASTEvaluator::visit(const _LuaValue& value, Environment& env, bool rvalue) const {
    cout << "visit value" << endl;

    switch (value.token.type) {
    case LuaToken::Type::NIL:
        return nil();
    case LuaToken::Type::FALSE:
        return false;
    case LuaToken::Type::TRUE:
        return true;
    case LuaToken::Type::NUMLIT:
        return val{stod(value.token.match)};
    case LuaToken::Type::STRINGLIT:
        return val{string(value.token.match.begin()+1, value.token.match.end()-1)};
    default:
        return string{"value unimplemented"};
    }
}

eval_result_t ASTEvaluator::visit(const _LuaChunk& chunk, Environment& env, bool rvalue) const {
    cout << "visit chunk" << endl;

    for (const auto& stmt : chunk.statements) {
        EVAL(result, stmt, *this, env);

        if (!holds_alternative<nil>(result) && !dynamic_pointer_cast<_LuaFunctioncall>(stmt)) {
                return result;
        }
    }

    return nil();
}

eval_result_t ASTEvaluator::visit(const _LuaForStmt& for_stmt, Environment& env, bool rvalue) const {
    cout << "visit for" << endl;

    lua::rt::Environment newenv;
    newenv.parent = &env;

    EVAL(var, for_stmt.var, *this, env);

    EVAL(start, for_stmt.start, *this, env);

    newenv.assign(var, start);

    for (;;) {
        val current = newenv.getvar(var);

        EVAL(end, for_stmt.end, *this, newenv);

        auto gt = op_gt(current, end);
        if (holds_alternative<string>(gt)) {
            return gt;
        }
        if (get<bool>(get<val>(gt)))
            return nil();

        EVAL(result, for_stmt.body, *this, newenv);
        if (holds_alternative<vallist_p>(result))
            return result;

        if (holds_alternative<bool>(result))
            return nil();

        EVAL(step, for_stmt.step, *this, newenv);

        auto sum = op_add(current, step);
        if (holds_alternative<val>(sum))
            newenv.assign(var, get<val>(sum));
        else
            return sum;
    }
}

eval_result_t ASTEvaluator::visit(const _LuaLoopStmt &loop_stmt, Environment &env, bool rvalue) const
{
    cout << "visit loop" << endl;

    if (loop_stmt.head_controlled) {
        EVAL(condition, loop_stmt.end, *this, env);

        auto neq = op_neq(val{true}, condition);
        if (holds_alternative<string>(neq)) {
            return neq;
        }
        if (get<bool>(get<val>(neq))) {
            return nil();
        }
    }

    for (;;) {
        lua::rt::Environment newenv;
        newenv.parent = &env;

        EVAL(result, loop_stmt.body, *this, newenv);
        if (holds_alternative<vallist_p>(result))
            return result;

        if (holds_alternative<bool>(result))
            return nil();

        // check loop condition
        EVAL(condition, loop_stmt.end, *this, newenv);

        auto neq = op_neq(val{true}, condition);
        if (holds_alternative<string>(neq)) {
            return neq;
        }
        if (get<bool>(get<val>(neq))) {
            return nil();
        }
    }
}

eval_result_t ASTEvaluator::visit(const _LuaTableconstructor& tableconst, Environment& env, bool rvalue) const {
    cout << "visit tableconstructor" << endl;
    return string{"tableconstructor unimplemented"};
}

eval_result_t ASTEvaluator::visit(const _LuaFunction& exp, Environment& env, bool rvalue) const {
    cout << "visit function" << endl;

    EVALL(_params, exp.params, *this, env);
    vallist_p params = get<vallist_p>(_params);

    return make_shared<lfunction>(exp.body, *params);
}

eval_result_t ASTEvaluator::visit(const _LuaIfStmt &stmt, Environment &env, bool rvalue) const {
    cout << "visit if" << endl;

    for (const auto& branch : stmt.branches) {
        EVAL(condition, branch.first, *this, env);

        auto eq = op_eq(val{true}, condition);
        if (holds_alternative<string>(eq)) {
            return eq;
        }

        if (get<bool>(get<val>(eq))) {
            lua::rt::Environment newenv;
            newenv.parent = &env;

            EVAL(result, branch.second, *this, newenv);
            if (!holds_alternative<nil>(result))
                return result;
            break;
        }
    }

    return lua::rt::nil();
}

}
}
