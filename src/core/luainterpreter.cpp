#include "luainterpreter.h"
#include <sstream>
#include <cmath>

namespace lua {
namespace rt {

eval_result_t op_add(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {get<double>(a) + get<double>(b), sourcebinop::create(a, b, tok)};

    return string{"could not add values of type other than number (" + a.type() + ", " + b.type() + ")"};
}

eval_result_t op_sub(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {get<double>(a) - get<double>(b), sourcebinop::create(a, b, tok)};

    return string{"could not subtract variables of type other than number"};
}

eval_result_t op_mul(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {get<double>(a) * get<double>(b), sourcebinop::create(a, b, tok)};

    return string{"could not multiply variables of type other than number"};
}

eval_result_t op_div(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {get<double>(a) / get<double>(b), sourcebinop::create(a, b, tok)};

    return string{"could not divide variables of type other than number"};
}

eval_result_t op_pow(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {pow(get<double>(a), get<double>(b)), sourcebinop::create(a, b, tok)};

    return string{"could not exponentiate variables of type other than number"};
}

eval_result_t op_mod(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return lua::rt::val {fmod(get<double>(a), get<double>(b)), sourcebinop::create(a, b, tok)};

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
    }, static_cast<lua::rt::val::value_t>(a));
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

eval_result_t op_strip(val v) {
    v.source.reset();
    return move(v);
}

eval_result_t op_not(val v) {
    if (holds_alternative<nil>(v))
        return val {true};

    if (holds_alternative<bool>(v))
        return val {!get<bool>(v)};

    return val {false};
}

eval_result_t op_neg(val v, const LuaToken& tok) {
    if (holds_alternative<double>(v)) {
        return val {-get<double>(v), sourceunop::create(v, tok)};
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

void Environment::assign(const val& var, const val& newval, bool is_local) {
    // cout << "assignment " << var << "=" << newval << (is_local ? " (local)" : "") << endl;

    if (is_local) {
        t[var] = newval;
        return;
    }

    // search environments for variable
    for (Environment *env = this; env != nullptr; env = env->parent.get()) {
        if (env->t.count(var)) {
            env->t[var] = newval;
            return;
        }
    }

    // not yet assigned, assign to global environment
    (*global)[var] = newval;
}

val Environment::getvar(const val& var) {
    // search environments for variable
    for (Environment *env = this; env != nullptr; env = env->parent.get()) {
        if (env->t.count(var)) {
            return env->t[var];
        }
    }
    return nil();
}

void Environment::populate_stdlib() {
    t[string {"print"}] = make_shared<cfunction>([](const vallist& args) -> vallist {
        for (int i = 0; i < static_cast<int>(args.size()) - 1; ++i) {
            cout << args[i].to_string() << "\t";
        }
        if (args.size() > 0) {
            cout << args.back();
        }
        cout << endl;
        return {};
    });

    auto math = make_shared<table>();
    t[string {"math"}] = math;
    (*math)[string {"sin"}] = make_shared<cfunction>([](const vallist& args) -> vallist {
        if (args.size() != 1 || args[0].type() != "number") {
            return {nil(), string {"sin: one number argument expected"}};
        }

        val result = sin(get<double>(args[0]));
        if (args[0].source)
            result.source = sourcelambda::create([v = args[0]](const val& newval) -> optional<shared_ptr<SourceChange>> {
                    if (newval.type() == "number")
                        if (double x = asin(get<double>(newval)); isfinite(x))
                            return v.forceValue(x);
                    return nullopt;
                });
        return {result};
    });

    (*math)[string {"cos"}] = make_shared<cfunction>([](const vallist& args) -> vallist {
        if (args.size() != 1 || args[0].type() != "number") {
            return {nil(), string {"cos: one number argument expected"}};
        }

        val result = cos(get<double>(args[0]));
        if (args[0].source)
            result.source = sourcelambda::create([v = args[0]](const val& newval) -> optional<shared_ptr<SourceChange>> {
                    if (newval.type() == "number")
                        if (double x = acos(get<double>(newval)); isfinite(x))
                            return v.forceValue(x);
                    return nullopt;
                });
        return {result};
    });

    (*math)[string {"tan"}] = make_shared<cfunction>([](const vallist& args) -> vallist {
        if (args.size() != 1 || args[0].type() != "number") {
            return {nil(), string {"tan: one number argument expected"}};
        }

        val result = tan(get<double>(args[0]));
        if (args[0].source)
            result.source = sourcelambda::create([v = args[0]](const val& newval) -> optional<shared_ptr<SourceChange>> {
                    if (newval.type() == "number")
                        if (double x = atan(get<double>(newval)); isfinite(x))
                            return v.forceValue(x);
                    return nullopt;
                });
        return {result};
    });

    t[string {"_G"}] = shared_ptr<table>(shared_from_this(), &t);

    t[string {"__visit_count"}] = 0.0;
    t[string {"__visit_limit"}] = 1000.0;
}

eval_result_t ASTEvaluator::visit(const _LuaName& name, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit name" << endl;
    if (assign) {
        env->assign(val{name.token.match}, get<val>(*assign), get<bool>(*assign));
    }
    return val{name.token.match};
}

eval_result_t ASTEvaluator::visit(const _LuaOp& op, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit op" << endl;

    EVAL(lhs, op.lhs, env);

    EVAL(rhs, op.rhs, env);

    lhs = fst(lhs);
    rhs = fst(rhs);

    switch (op.op.type) {
    case LuaToken::Type::ADD:
        return op_add(lhs, rhs, op.op);
    case LuaToken::Type::SUB:
        return op_sub(lhs, rhs, op.op);
    case LuaToken::Type::MUL:
        return op_mul(lhs, rhs, op.op);
    case LuaToken::Type::DIV:
        return op_div(lhs, rhs, op.op);
    case LuaToken::Type::POW:
        return op_pow(lhs, rhs, op.op);
    case LuaToken::Type::MOD:
        return op_mod(lhs, rhs, op.op);
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

eval_result_t ASTEvaluator::visit(const _LuaUnop& op, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit unop" << endl;

    EVAL(rhs, op.exp, env);

    rhs = fst(rhs);

    switch (op.op.type) {
    case LuaToken::Type::SUB:
        return op_neg(rhs, op.op);
    case LuaToken::Type::LEN:
        return op_len(rhs);
    case LuaToken::Type::NOT:
        return op_not(rhs);
    case LuaToken::Type::STRIP:
        return op_strip(rhs);
    default:
        return string {op.op.match + " is not a unary operator"};
    }
}

eval_result_t ASTEvaluator::visit(const _LuaExplist& explist, const shared_ptr<Environment>& env, const assign_t& assign) const {
    // cout << "visit explist" << endl;

    auto t = make_shared<vallist>();
    for (unsigned i = 0; i < explist.exps.size(); ++i) {
        if (!assign) {
            EVAL(exp, explist.exps[i], env);
            t->push_back(exp);
        } else {
            if (!holds_alternative<vallist_p>(get<val>(*assign)))
                return string {"only a vallist can be assigned to a vallist"};
            bool local = get<bool>(*assign);
            EVALL(exp, explist.exps[i], env, make_tuple(get<vallist_p>(get<val>(*assign))->size() > i ?
                                                  get<vallist_p>(get<val>(*assign))->at(i) : nil(), local));
            t->push_back(exp);
        }
    }

    return move(t);
}

eval_result_t ASTEvaluator::visit(const _LuaFunctioncall& exp, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit functioncall" << endl;

    EVAL(func, exp.function, env);

    EVAL(_args, exp.args, env);
    vallist args = flatten(*get<vallist_p>(_args));

    // call builtin function
    if (holds_alternative<cfunction_p>(func)) {
        return make_shared<vallist>(get<cfunction_p>(func)->f(args));
    }

    // call lua function
    if (holds_alternative<lfunction_p>(func)) {
        EVALL(params, get<lfunction_p>(func)->params, get<lfunction_p>(func)->env, make_tuple(make_shared<vallist>(args), true));

        EVAL(result, get<lfunction_p>(func)->f, get<lfunction_p>(func)->env);

        if (holds_alternative<vallist_p>(result))
            return move(result);

        return make_shared<vallist>();
    }

    if (holds_alternative<nil>(func)) {
        return string {"attempted to call a nil value"};
    }

    return string{"functioncall unimplemented"};
}

eval_result_t ASTEvaluator::visit(const _LuaAssignment &assignment, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit assignment" << assignment.local << endl;

    EVAL(_exps, assignment.explist, env);
    vallist exps = flatten(*get<vallist_p>(_exps));
    EVALL(_vars, assignment.varlist, env, make_tuple(make_shared<vallist>(exps), assignment.local));

    return nil();
}

eval_result_t ASTEvaluator::visit(const _LuaNameVar& var, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit namevar " << var.name->token << endl;

    EVAL(name, var.name, env);

    return env->getvar(name);
}

eval_result_t ASTEvaluator::visit(const _LuaIndexVar& var, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit indexvar" << endl;

    EVAL(index, var.index, env);
    EVALR(table, var.table, env);

    if (holds_alternative<table_p>(table)) {

        if (assign) {
            (*get<table_p>(table))[index] = get<val>(*assign);
        }

        return (*get<table_p>(table))[index];
    } else {
        return string{"cannot access index on " + table.type()};
    }
}

eval_result_t ASTEvaluator::visit(const _LuaMemberVar& var, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit membervar" << endl;
    EVAL(index, var.member, env);
    EVALR(table, var.table, env);

    if (holds_alternative<table_p>(table)) {

        if (assign) {
            (*get<table_p>(table))[index] = get<val>(*assign);
        }

        return (*get<table_p>(table))[index];
    } else {
        return string{"cannot access member on " + table.type()};
    }
}

eval_result_t ASTEvaluator::visit(const _LuaReturnStmt& stmt, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit returnstmt" << endl;

    EVAL(result, stmt.explist, env);
    return make_shared<vallist>(flatten(*get<vallist_p>(result)));
}

eval_result_t ASTEvaluator::visit(const _LuaBreakStmt& stmt, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit breakstmt" << endl;
    return val{true};
}

eval_result_t ASTEvaluator::visit(const _LuaValue& value, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit value " << value.token << endl;

    switch (value.token.type) {
    case LuaToken::Type::NIL:
        return val{nil(), sourceval::create(value.token)};
    case LuaToken::Type::FALSE:
        return val{false, sourceval::create(value.token)};
    case LuaToken::Type::TRUE:
        return val{true, sourceval::create(value.token)};
    case LuaToken::Type::NUMLIT:
        try {
            return val{atof(("0" + value.token.match).c_str()), sourceval::create(value.token)};
        } catch (const invalid_argument& invalid) {
            return string {"invalid_argument to stod: "} + invalid.what();
        }

    case LuaToken::Type::STRINGLIT:
        return val{string(value.token.match.begin()+1, value.token.match.end()-1), sourceval::create(value.token)};
    default:
        return string{"value unimplemented"};
    }
}

eval_result_t ASTEvaluator::visit(const _LuaChunk& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit chunk" << endl;

    for (const auto& stmt : chunk.statements) {
        EVAL(result, stmt, env);

        if (!holds_alternative<nil>(result) && !dynamic_pointer_cast<_LuaFunctioncall>(stmt)) {
                return move(result);
        }
    }

    return nil();
}

eval_result_t ASTEvaluator::visit(const _LuaForStmt& for_stmt, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit for" << endl;

    auto newenv = make_shared<lua::rt::Environment>(env);

    EVAL(start, for_stmt.start, env);

    EVALL(var, for_stmt.var, env, make_tuple(start, true));

    for (;;) {
        val current = newenv->getvar(var);

        EVAL(end, for_stmt.end, newenv);

        auto gt = op_gt(current, end);
        if (holds_alternative<string>(gt)) {
            return gt;
        }
        if (get<bool>(get<val>(gt)))
            return nil();

        EVAL(result, for_stmt.body, newenv);
        if (holds_alternative<vallist_p>(result))
            return move(result);

        if (holds_alternative<bool>(result))
            return nil();

        EVAL(step, for_stmt.step, newenv);

        auto sum = op_add(current, step);
        if (holds_alternative<val>(sum)) {
            EVALL(var, for_stmt.var, env, make_tuple(get<val>(sum), true));
        } else {
            return sum;
        }
    }
}

eval_result_t ASTEvaluator::visit(const _LuaLoopStmt &loop_stmt, const shared_ptr<Environment>& env, const assign_t& assign) const
{
//    cout << "visit loop" << endl;

    if (loop_stmt.head_controlled) {
        EVAL(condition, loop_stmt.end, env);

        auto neq = op_neq(val{true}, condition);
        if (holds_alternative<string>(neq)) {
            return neq;
        }
        if (get<bool>(get<val>(neq))) {
            return nil();
        }
    }

    for (;;) {
        auto newenv = make_shared<lua::rt::Environment>(env);

        EVAL(result, loop_stmt.body, newenv);
        if (holds_alternative<vallist_p>(result))
            return move(result);

        if (holds_alternative<bool>(result))
            return nil();

        // check loop condition
        EVAL(condition, loop_stmt.end, newenv);

        auto neq = op_neq(val{true}, condition);
        if (holds_alternative<string>(neq)) {
            return neq;
        }
        if (get<bool>(get<val>(neq))) {
            return nil();
        }
    }
}

eval_result_t ASTEvaluator::visit(const _LuaTableconstructor& tableconst, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit tableconstructor" << endl;
    table_p result = make_shared<table>();

    double default_idx = 1.0;
    for (const LuaField& field : tableconst.fields) {
        EVAL(rhs, field->rhs, env);

        if (!field->lhs) {
            (*result)[val(default_idx)] = rhs;
            default_idx++;
        } else {
            EVAL(lhs, field->lhs, env);
            (*result)[lhs] = rhs;
        }
    }

    return move(result);
}

eval_result_t ASTEvaluator::visit(const _LuaFunction& exp, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit function" << endl;

    return make_shared<lfunction>(exp.body, exp.params, make_shared<Environment>(env));
}

eval_result_t ASTEvaluator::visit(const _LuaIfStmt &stmt, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit if" << endl;

    for (const auto& branch : stmt.branches) {
        EVAL(condition, branch.first, env);

        auto eq = op_eq(val{true}, condition);
        if (holds_alternative<string>(eq)) {
            return eq;
        }

        if (get<bool>(get<val>(eq))) {
            auto newenv = make_shared<lua::rt::Environment>(env);

            EVAL(result, branch.second, newenv);
            if (!holds_alternative<nil>(result))
                return move(result);
            break;
        }
    }

    return lua::rt::nil();
}

SourceChange::~SourceChange() {}

string SourceChangeOr::to_string() const {
    stringstream ss;
    ss << "(";
    if (alternatives.size() != 0) {
        ss << alternatives[0]->to_string();
    }
    for (unsigned i = 1; i < alternatives.size(); ++i)
        ss << " | " << alternatives[i]->to_string();
    ss << ")";
    return ss.str();
}

string SourceChangeAnd::to_string() const {
    stringstream ss;
    ss << "(";
    if (changes.size() != 0) {
        ss << changes[0]->to_string();
    }
    for (unsigned i = 1; i < changes.size(); ++i)
        ss << " & " << changes[i]->to_string();
    ss << ")";
    return ss.str();
}

void SourceChangeOr::apply(vector<LuaToken>& tokens) const {
    if (!alternatives.empty())
        alternatives[0]->apply(tokens);
}

void SourceChangeAnd::apply(vector<LuaToken>& tokens) const {
    for (const auto& c : changes)
        c->apply(tokens);
}

void SourceAssignment::apply(vector<LuaToken>& tokens) const {
    for (auto& t : tokens)
        if (t.pos == token.pos && t.length == token.length)
            t.match = replacement;
}

sourceexp::~sourceexp() {}

}
}
