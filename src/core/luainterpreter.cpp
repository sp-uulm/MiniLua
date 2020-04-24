#include "luainterpreter.h"
#include <sstream>
#include <cmath>

namespace lua {
namespace rt {

eval_result_t op_add(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success({get<double>(a) + get<double>(b), sourcebinop::create(a, b, tok)});

    return string{"could not add values of type other than number (" + a.type() + ", " + b.type() + ")"};
}

eval_result_t op_sub(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(lua::rt::val {get<double>(a) - get<double>(b), sourcebinop::create(a, b, tok)});

    return string{"could not subtract variables of type other than number"};
}

eval_result_t op_mul(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(lua::rt::val {get<double>(a) * get<double>(b), sourcebinop::create(a, b, tok)});

    return string{"could not multiply variables of type other than number"};
}

eval_result_t op_div(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(lua::rt::val {get<double>(a) / get<double>(b), sourcebinop::create(a, b, tok)});

    return string{"could not divide variables of type other than number"};
}

eval_result_t op_pow(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(lua::rt::val {pow(get<double>(a), get<double>(b)), sourcebinop::create(a, b, tok)});

    return string{"could not exponentiate variables of type other than number"};
}

eval_result_t op_mod(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(lua::rt::val {fmod(get<double>(a), get<double>(b)), sourcebinop::create(a, b, tok)});

    return string{"could not mod variables of type other than number"};
}

eval_result_t op_concat(lua::rt::val a, lua::rt::val b) {
    if ((holds_alternative<double>(a) || holds_alternative<string>(a)) &&
        (holds_alternative<double>(b) || holds_alternative<string>(b))) {

        stringstream ss;
        ss << a << b;
        return eval_success(lua::rt::val {ss.str()});
    }

    return string{"could not concatenate other types than strings or numbers"};
}

eval_result_t op_eval(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    //cout << a.literal() << "\\" << b.literal() << endl;

    val result = a;
    result.source = sourcebinop::create(a, b, tok);

    // replace the right operand with the value of the left only if it is a literal
    // TODO: make this work with rhs expressions
    if (b.source && dynamic_pointer_cast<sourceval>(b.source)) {
        auto sc = make_shared<SourceChangeAnd>();

        for (const auto& tok : dynamic_pointer_cast<sourceval>(b.source)->location) {
            sc->changes.push_back(SourceAssignment::create(tok, ""));
        }

        dynamic_pointer_cast<SourceAssignment>(sc->changes[0])->replacement = a.literal();

        return eval_success(result, sc);
    }

    return eval_success(result);
}

eval_result_t op_postfix_eval(val a, const LuaToken& tok) {
    //cout << a.literal() << "\\" << endl;

    val result = a;
    result.source = sourceunop::create(a, tok);

    return eval_success(result, SourceAssignment::create(tok, "\\" + a.literal()));
}

eval_result_t op_lt(lua::rt::val a, lua::rt::val b) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(lua::rt::val {get<double>(a) < get<double>(b)});

    if (holds_alternative<string>(a) && holds_alternative<string>(b))
        return eval_success(lua::rt::val {get<string>(a) < get<string>(b)});

    return string{"only strings and numbers can be compared"};
}

eval_result_t op_leq(lua::rt::val a, lua::rt::val b) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(lua::rt::val {get<double>(a) <= get<double>(b)});

    if (holds_alternative<string>(a) && holds_alternative<string>(b))
        return eval_success(lua::rt::val {get<string>(a) <= get<string>(b)});

    return string{"only strings and numbers can be compared"};
}

eval_result_t op_gt(lua::rt::val a, lua::rt::val b) {
    auto leq = op_leq(a, b);

    if (holds_alternative<eval_success_t>(leq))
        return op_not(get_val(leq));

    return string{"only strings and numbers can be compared"};
}

eval_result_t op_geq(lua::rt::val a, lua::rt::val b) {
    auto leq = op_lt(a, b);

    if (holds_alternative<eval_success_t>(leq))
        return op_not(get_val(leq));

    return string{"only strings and numbers can be compared"};
}

eval_result_t op_eq(lua::rt::val a, lua::rt::val b) {
    if (a.index() != b.index())
        return eval_success(false);

    return eval_success(visit([&b](auto&& a){
        using T = std::decay_t<decltype(a)>;
        return val {a == get<T>(b)};
    }, static_cast<val::value_t>(a)));
}

eval_result_t op_neq(val a, val b) {
    return op_not(get_val(op_eq(a, b)));
}

eval_result_t op_and(val a, val b) {
    return eval_success(a.to_bool() ? b : a);
}

eval_result_t op_or(val a, val b) {
    return eval_success(a.to_bool() ? a : b);
}

eval_result_t op_len(val v) {
    if (!v.istable()) {
        return string{"unary # can only be applied to a table (is " + v.type() + ")"};
    }

    table& t = *get<table_p>(v);

    int i = 1;
    for ( ;; i++) {
         if (auto idx = t.find(i); idx == t.end() || idx->second.isnil())
            break;
    }
    return eval_success(i-1);
}

eval_result_t op_strip(val v) {
    v.source.reset();
    return eval_success(v);
}

eval_result_t op_not(val v) {
    return eval_success(!v.to_bool());
}

eval_result_t op_neg(val v, const LuaToken& tok) {
    if (holds_alternative<double>(v)) {
        return eval_success(val {-get<double>(v), sourceunop::create(v, tok)});
    }

    return string{"unary - can only be applied to a number"};
}

eval_result_t op_sqrt(val v) {
    if (holds_alternative<double>(v)) {
        struct sqrt_exp : sourceexp {
            sqrt_exp(const val& v) : v(v) {}

            optional<shared_ptr<SourceChange>> forceValue(const val& newval) const override{
                if (newval.isnumber())
                    if (double x = get<double>(newval)*get<double>(newval); isfinite(x))
                        return v.forceValue(x);
                return nullopt;
            }

            eval_result_t reevaluate() override {
                if (holds_alternative<double>(v)) {
                    return eval_success(sqrt(get<double>(v.reevaluate())));
                }
                return string{"sqrt can only be applied to a number"};
            }

            bool isDirty() const override {
                return v.source && v.source->isDirty();
            }

            vector<LuaToken> get_all_tokens() const override {
                if (v.source)
                    return v.source->get_all_tokens();
                return {};
            }

            val v;
        };

        return eval_success(val {sqrt(get<double>(v)), std::make_shared<sqrt_exp>(v)});
    }

    return string{"sqrt can only be applied to a number"};
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
    t["print"] = make_shared<cfunction>([](const vallist& args) -> cfunction::result {
        for (int i = 0; i < static_cast<int>(args.size()) - 1; ++i) {
            cout << args[i].to_string() << "\t";
        }
        if (args.size() > 0) {
            cout << args.back();
        }
        cout << endl;
        return {};
    });

    t["type"] = make_shared<cfunction>([](const vallist& args) -> cfunction::result {
        if (args.size() != 1) {
            return vallist{nil(), string {"type: one argument expected"}};
        }

        return {val(args[0].type())};
    });

    auto math = make_shared<table>();
    t["math"] = math;
    (*math)["sin"] = make_shared<cfunction>([](const vallist& args) -> cfunction::result {
        if (args.size() != 1 || args[0].type() != "number") {
            return vallist{nil(), string {"sin: one number argument expected"}};
        }

        val result = sin(get<double>(args[0]));
        if (args[0].source) {
            struct sin_exp : sourceexp {
                sin_exp(const val& v) : v(v) {}

                optional<shared_ptr<SourceChange>> forceValue(const val& newval) const override{
                    if (newval.isnumber())
                        if (double x = asin(get<double>(newval)); isfinite(x))
                            return v.forceValue(x);
                    return nullopt;
                }

                eval_result_t reevaluate() override {
                    if (holds_alternative<double>(v)) {
                        return eval_success(sin(get<double>(v.reevaluate())));
                    }
                    return string{"sin can only be applied to a number"};
                }

                bool isDirty() const override {
                    return v.source && v.source->isDirty();
                }

                vector<LuaToken> get_all_tokens() const override {
                    if (v.source)
                        return v.source->get_all_tokens();
                    return {};
                }

                val v;
            };

            result.source = std::make_shared<sin_exp>(args[0]);
        }

        return {result};
    });

    (*math)["cos"] = make_shared<cfunction>([](const vallist& args) -> cfunction::result {
        if (args.size() != 1 || args[0].type() != "number") {
            return vallist{nil(), string {"cos: one number argument expected"}};
        }

        val result = cos(get<double>(args[0]));
        if (args[0].source) {
            struct cos_exp : sourceexp {
                cos_exp(const val& v) : v(v) {}

                optional<shared_ptr<SourceChange>> forceValue(const val& newval) const override{
                    if (newval.isnumber())
                        if (double x = acos(get<double>(newval)); isfinite(x))
                            return v.forceValue(x);
                    return nullopt;
                }

                eval_result_t reevaluate() override {
                    if (holds_alternative<double>(v)) {
                        return eval_success(cos(get<double>(v.reevaluate())));
                    }
                    return string{"cos can only be applied to a number"};
                }

                bool isDirty() const override {
                    return v.source && v.source->isDirty();
                }

                vector<LuaToken> get_all_tokens() const override {
                    if (v.source)
                        return v.source->get_all_tokens();
                    return {};
                }

                val v;
            };

            result.source = std::make_shared<cos_exp>(args[0]);
        }

        return {result};
    });

    (*math)["tan"] = make_shared<cfunction>([](const vallist& args) -> cfunction::result {
        if (args.size() != 1 || args[0].type() != "number") {
            return vallist{nil(), string {"tan: one number argument expected"}};
        }

        val result = tan(get<double>(args[0]));
        if (args[0].source) {
            struct tan_exp : sourceexp {
                tan_exp(const val& v) : v(v) {}

                optional<shared_ptr<SourceChange>> forceValue(const val& newval) const override{
                    if (newval.isnumber())
                        if (double x = atan(get<double>(newval)); isfinite(x))
                            return v.forceValue(x);
                    return nullopt;
                }

                eval_result_t reevaluate() override {
                    if (holds_alternative<double>(v)) {
                        return eval_success(tan(get<double>(v.reevaluate())));
                    }
                    return string{"sin can only be applied to a number"};
                }

                bool isDirty() const override {
                    return v.source && v.source->isDirty();
                }

                vector<LuaToken> get_all_tokens() const override {
                    if (v.source)
                        return v.source->get_all_tokens();
                    return {};
                }

                val v;
            };

            result.source = std::make_shared<tan_exp>(args[0]);
        }

        return {result};
    });

    (*math)["atan2"] = make_shared<cfunction>([](const vallist& args) -> cfunction::result {
        if (args.size() != 2 || !args[0].isnumber() || !args[1].isnumber()) {
            return vallist{nil(), string {"atan2: two number arguments expected"}};
        }

        val result = atan2(get<double>(args[0]), get<double>(args[1]));
        if (args[0].source || args[1].source) {
            struct atan2_exp : sourceexp {
                atan2_exp(const val& y, const val& x) : y(y), x(x) {}

                optional<shared_ptr<SourceChange>> forceValue(const val& newval) const override{
                    if (newval.isnumber())
                        if (double result = tan(get<double>(newval)); isfinite(result))
                            return (y/x).forceValue(result);
                    return nullopt;
                }

                eval_result_t reevaluate() override {
                    if (holds_alternative<double>(y) && holds_alternative<double>(x)) {
                        return eval_success(atan2(get<double>(y.reevaluate()), get<double>(x.reevaluate())));
                    }
                    return string{"atan2 can only be applied to numbers"};
                }

                bool isDirty() const override {
                    return (y.source && y.source->isDirty()) || (x.source && x.source->isDirty());
                }

                vector<LuaToken> get_all_tokens() const override {
                    vector<LuaToken> result;
                    if (y.source) {
                        auto lhs_tokens = y.source->get_all_tokens();
                        result.insert(end(result), begin(lhs_tokens), end(lhs_tokens));
                    }
                    if (x.source) {
                        auto rhs_tokens = x.source->get_all_tokens();
                        result.insert(end(result), begin(rhs_tokens), end(rhs_tokens));
                    }
                    return result;
                }

                val y, x;
            };

            result.source = std::make_shared<atan2_exp>(args[0], args[1]);
        }

        return {result};
    });

    (*math)["sqrt"] = make_shared<cfunction>([](const vallist& args) -> cfunction::result {
        if (args.size() != 1 || args[0].type() != "number") {
            return vallist{nil(), string {"sqrt: one number argument expected"}};
        }

        val result = unwrap(op_sqrt(args[0]));
        return {result};
    });

    (*math)["abs"] = make_shared<cfunction>([](const vallist& args) -> cfunction::result {
        if (args.size() != 1 || !args[0].isnumber()) {
            return vallist{nil(), string {"abs: one number argument expected"}};
        }

        val result = fabs(get<double>(args[0]));
        if (args[0].source) {
            struct abs_exp : sourceexp {
                abs_exp(const val& v) : v(v) {}

                optional<shared_ptr<SourceChange>> forceValue(const val& newval) const override{
                    if (newval.isnumber() && get<double>(newval) >= 0) {
                        if (get<double>(v) >= 0) {
                            return v.forceValue(newval);
                        } else {
                            return v.forceValue(-newval);
                        }
                    }
                    return nullopt;
                }

                eval_result_t reevaluate() override {
                    if (holds_alternative<double>(v)) {
                        return eval_success(fabs(get<double>(v.reevaluate())));
                    }
                    return string{"abs can only be applied to a number"};
                }

                bool isDirty() const override {
                    return v.source && v.source->isDirty();
                }

                vector<LuaToken> get_all_tokens() const override {
                    if (v.source)
                        return v.source->get_all_tokens();
                    return {};
                }

                val v;
            };

            result.source = std::make_shared<abs_exp>(args[0]);
        }

        return {result};
    });

    (*math)["pi"] = 3.1415926;

    t["_G"] = shared_ptr<table>(shared_from_this(), &t);

    t["__visit_count"] = 0.0;
    t["__visit_limit"] = 1000.0;
}

eval_result_t ASTEvaluator::visit(const _LuaName& name, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit name" << endl;
    if (assign) {
        env->assign(val{name.token.match}, get<val>(*assign), get<bool>(*assign));
    }
    return eval_success(name.token.match);
}

eval_result_t ASTEvaluator::visit(const _LuaOp& op, const shared_ptr<Environment>& env, const assign_t& assign) const {
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
        return string {op.op.match + " is not a binary operator"};
    }
}

eval_result_t ASTEvaluator::visit(const _LuaUnop& op, const shared_ptr<Environment>& env, const assign_t& assign) const {
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
        return string {op.op.match + " is not a unary operator"};
    }
}

eval_result_t ASTEvaluator::visit(const _LuaExplist& explist, const shared_ptr<Environment>& env, const assign_t& assign) const {
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
                return string {"only a vallist can be assigned to a vallist"};
            bool local = get<bool>(*assign);
            EVALL(exp, explist.exps[i], env, make_tuple(get<vallist_p>(get<val>(*assign))->size() > i ?
                                                  get<vallist_p>(get<val>(*assign))->at(i) : nil(), local));
            t->push_back(exp);
            sc = sc & exp_sc;
        }
    }

    return eval_success(t, sc);
}

eval_result_t ASTEvaluator::visit(const _LuaFunctioncall& exp, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit functioncall" << endl;

    EVAL(func, exp.function, env);

    EVAL(_args, exp.args, env);
    vallist args = flatten(*get<vallist_p>(_args));

    // call builtin function
    if (holds_alternative<cfunction_p>(func)) {
        if (auto result = get<cfunction_p>(func)->f(args, exp); holds_alternative<vallist>(result))
            return eval_success(make_shared<vallist>(get<vallist>(result)), func_sc & _args_sc);
        else {
            return get<string>(result);
        }
    }

    // call lua function
    if (holds_alternative<lfunction_p>(func)) {
        EVALL(params, get<lfunction_p>(func)->params, get<lfunction_p>(func)->env, make_tuple(make_shared<vallist>(args), true));

        EVAL(result, get<lfunction_p>(func)->f, get<lfunction_p>(func)->env);

        if (holds_alternative<vallist_p>(result))
            return eval_success(result, func_sc & _args_sc & params_sc & result_sc);

        return eval_success(make_shared<vallist>(), func_sc & _args_sc & params_sc);
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

    return eval_success(nil(), _exps_sc & _vars_sc);
}

eval_result_t ASTEvaluator::visit(const _LuaNameVar& var, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit namevar " << var.name->token << endl;

    EVAL(name, var.name, env);

    return eval_success(env->getvar(name), name_sc);
}

eval_result_t ASTEvaluator::visit(const _LuaIndexVar& var, const shared_ptr<Environment>& env, const assign_t& assign) const {
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

eval_result_t ASTEvaluator::visit(const _LuaMemberVar& var, const shared_ptr<Environment>& env, const assign_t& assign) const {
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

eval_result_t ASTEvaluator::visit(const _LuaReturnStmt& stmt, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit returnstmt" << endl;

    EVAL(result, stmt.explist, env);
    return eval_success(make_shared<vallist>(flatten(*get<vallist_p>(result))), result_sc);
}

eval_result_t ASTEvaluator::visit(const _LuaBreakStmt& stmt, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit breakstmt" << endl;
    return eval_success(true);
}

eval_result_t ASTEvaluator::visit(const _LuaValue& value, const shared_ptr<Environment>& env, const assign_t& assign) const {
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
            return eval_success(val{atof(("0" + value.token.match).c_str()), sourceval::create(value.token)});
        } catch (const invalid_argument& invalid) {
            return string {"invalid_argument to stod: "} + invalid.what();
        }

    case LuaToken::Type::STRINGLIT:
        return eval_success(val{string(value.token.match.begin()+1, value.token.match.end()-1), sourceval::create(value.token)});
    default:
        return string{"value unimplemented"};
    }
}

eval_result_t ASTEvaluator::visit(const _LuaChunk& chunk, const shared_ptr<Environment>& env, const assign_t& assign) const {
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

eval_result_t ASTEvaluator::visit(const _LuaForStmt& for_stmt, const shared_ptr<Environment>& env, const assign_t& assign) const {
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

        auto gt = op_gt(current, end);
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

eval_result_t ASTEvaluator::visit(const _LuaLoopStmt &loop_stmt, const shared_ptr<Environment>& env, const assign_t& assign) const
{
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

eval_result_t ASTEvaluator::visit(const _LuaTableconstructor& tableconst, const shared_ptr<Environment>& env, const assign_t& assign) const {
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

eval_result_t ASTEvaluator::visit(const _LuaFunction& exp, const shared_ptr<Environment>& env, const assign_t& assign) const {
//    cout << "visit function" << endl;

    return eval_success(make_shared<lfunction>(exp.body, exp.params, make_shared<Environment>(env)));
}

eval_result_t ASTEvaluator::visit(const _LuaIfStmt &stmt, const shared_ptr<Environment>& env, const assign_t& assign) const {
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

SourceChangeVisitor::~SourceChangeVisitor() {}

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

void ApplySCVisitor::visit(const SourceChangeOr& sc_or) {
    if (!sc_or.alternatives.empty())
        sc_or.alternatives[0]->accept(*this);
}

void ApplySCVisitor::visit(const SourceChangeAnd& sc_and) {
    for (const auto& c : sc_and.changes) {
        c->accept(*this);
    }
}

void ApplySCVisitor::visit(const SourceAssignment& sc_ass) {
    changes.push_back(sc_ass);
}

vector<LuaToken> ApplySCVisitor::apply_changes(const vector<LuaToken>& tokens) {
    auto new_tokens = tokens;

    // sort the collected changes according to their position
    std::sort(changes.begin(), changes.end(), [](const auto& a, const auto& b){
        return a.token.pos > b.token.pos;
    });

    // apply the changes from back to front
    for (const auto& sc : changes) {
        for (auto& t : new_tokens) {
            if (t.pos == sc.token.pos) {
                t.match = sc.replacement;
                t.length = static_cast<long>(sc.replacement.length());
            }
        }
    }

    // delete list of changes
    changes.clear();

    return new_tokens;
}

sourceexp::~sourceexp() {}

source_change_t sourceval::forceValue(const val& v) const {

    auto sc = make_shared<SourceChangeAnd>();

    for (const auto& tok : location) {
        sc->changes.push_back(SourceAssignment::create(tok, ""));
    }

    dynamic_pointer_cast<SourceAssignment>(sc->changes[0])->replacement = v.literal();

    return move(sc);
}

eval_result_t sourceval::reevaluate() {
    // should not be necessary, as the value cannot change
    return string{"reevaluate unimplemented"};
}

bool sourceval::isDirty() const {
    return false;
}

source_change_t sourcebinop::forceValue(const val& v) const {
    if (!holds_alternative<double>(v))
        return nullopt;

    switch (op.type) {
    case LuaToken::Type::ADD:
    {
        auto res_or = make_shared<SourceChangeOr>();
        if (lhs.source && holds_alternative<double>(rhs)) {
            if (auto result = lhs.source->forceValue(val {get<double>(v) - get<double>(rhs)}); result) {
                res_or->alternatives.push_back(*result);
            }
        }
        if (rhs.source && holds_alternative<double>(lhs)) {
            if (auto result = rhs.source->forceValue(val {get<double>(v) - get<double>(lhs)}); result) {
                res_or->alternatives.push_back(*result);
            }
        }

        if (!res_or->alternatives.empty())
            return res_or;
        return nullopt;
    }
    case LuaToken::Type::SUB:
    {
        auto res_or = make_shared<SourceChangeOr>();
        if (lhs.source && holds_alternative<double>(rhs)) {
            if (auto result = lhs.source->forceValue(val {get<double>(v) + get<double>(rhs)}); result) {
                res_or->alternatives.push_back(*result);
            }
        }
        if (rhs.source && holds_alternative<double>(lhs)) {
            if (auto result = rhs.source->forceValue(val {get<double>(lhs) - get<double>(v)}); result) {
                res_or->alternatives.push_back(*result);
            }
        }

        if (!res_or->alternatives.empty())
            return res_or;
        return nullopt;
    }
    case LuaToken::Type::MUL:
    {
        auto res_or = make_shared<SourceChangeOr>();
        if (lhs.source && holds_alternative<double>(rhs)) {
            if (auto result = lhs.source->forceValue(val {get<double>(v) / get<double>(rhs)}); result) {
                res_or->alternatives.push_back(*result);
            }
        }
        if (rhs.source && holds_alternative<double>(lhs)) {
            if (auto result = rhs.source->forceValue(val {get<double>(v) / get<double>(lhs)}); result) {
                res_or->alternatives.push_back(*result);
            }
        }

        if (!res_or->alternatives.empty())
            return res_or;
        return nullopt;
    }
    case LuaToken::Type::DIV:
    {
        auto res_or = make_shared<SourceChangeOr>();
        if (lhs.source && holds_alternative<double>(rhs)) {
            if (auto result = lhs.source->forceValue(val {get<double>(v) * get<double>(rhs)}); result) {
                res_or->alternatives.push_back(*result);
            }
        }
        if (rhs.source && holds_alternative<double>(lhs)) {
            if (auto result = rhs.source->forceValue(val {get<double>(lhs) / get<double>(v)}); result) {
                res_or->alternatives.push_back(*result);
            }
        }

        if (!res_or->alternatives.empty())
            return res_or;
        return nullopt;
    }
    case LuaToken::Type::POW:
    {
        auto res_or = make_shared<SourceChangeOr>();
        if (lhs.source && holds_alternative<double>(rhs)) {
            if (auto result = lhs.source->forceValue(val {pow(get<double>(v), 1/get<double>(rhs))}); result) {
                res_or->alternatives.push_back(*result);
            }
        }
        if (rhs.source && holds_alternative<double>(lhs)) {
            auto new_rhs = log(get<double>(v)) / log(get<double>(lhs));
            if (!isnan(new_rhs))
                if (auto result = rhs.source->forceValue(val {new_rhs}); result) {
                    res_or->alternatives.push_back(*result);
                }
        }

        if (!res_or->alternatives.empty())
            return res_or;
        return nullopt;
    }
    case LuaToken::Type::MOD:
    {
        auto res_or = make_shared<SourceChangeOr>();
        if (lhs.source && holds_alternative<double>(rhs)) {
            if (auto result = lhs.source->forceValue(v); result && get<double>(rhs) > get<double>(v)) {
                res_or->alternatives.push_back(*result);
            }
        }
        if (rhs.source && holds_alternative<double>(lhs)) {
            //TODO: doesn't work if lhs < v
            if (auto result = rhs.source->forceValue(val {get<double>(lhs) - get<double>(v)}); result) {
                res_or->alternatives.push_back(*result);
            }
        }

        if (!res_or->alternatives.empty())
            return res_or;
        return nullopt;
    }
    case LuaToken::Type::EVAL:
    {
        auto res_and = make_shared<SourceChangeAnd>();
        if (lhs.source) {
            if (auto result = lhs.source->forceValue(v); result) {
                res_and->changes.push_back(*result);
            }
        }
        if (rhs.source) {
            if (auto result = rhs.source->forceValue(v); result) {
                res_and->changes.push_back(*result);
            }
        }

        if (!res_and->changes.empty())
            return res_and;
        return nullopt;
    }
    default:
        return nullopt;
    }
}

eval_result_t sourcebinop::reevaluate() {
    auto _lhs = fst(lhs.reevaluate());
    auto _rhs = fst(rhs.reevaluate());

    switch (op.type) {
    case LuaToken::Type::ADD:
        return op_add(_lhs, _rhs, op);
    case LuaToken::Type::SUB:
        return op_sub(_lhs, _rhs, op);
    case LuaToken::Type::MUL:
        return op_mul(_lhs, _rhs, op);
    case LuaToken::Type::DIV:
        return op_div(_lhs, _rhs, op);
    case LuaToken::Type::POW:
        return op_pow(_lhs, _rhs, op);
    case LuaToken::Type::MOD:
        return op_mod(_lhs, _rhs, op);
    case LuaToken::Type::CONCAT:
        return op_concat(_lhs, _rhs);
    case LuaToken::Type::EVAL:
        return op_eval(_lhs, _rhs, op);
    case LuaToken::Type::LT:
        return op_lt(_lhs, _rhs);
    case LuaToken::Type::LEQ:
        return op_leq(_lhs, _rhs);
    case LuaToken::Type::GT:
        return op_gt(_lhs, _rhs);
    case LuaToken::Type::GEQ:
        return op_geq(_lhs, _rhs);
    case LuaToken::Type::EQ:
        return op_eq(_lhs, _rhs);
    case LuaToken::Type::NEQ:
        return op_neq(_lhs, _rhs);
    case LuaToken::Type::AND:
        return op_and(_lhs, _rhs);
    case LuaToken::Type::OR:
        return op_or(_lhs, _rhs);
    default:
        return string {op.match + " cannot be reevaluated"};
    }
}

bool sourcebinop::isDirty() const {
    return (lhs.source && lhs.source->isDirty())
        || (rhs.source && rhs.source->isDirty());
}

source_change_t sourceunop::forceValue(const val& new_v) const {
    if (!holds_alternative<double>(new_v))
        return nullopt;

    if (!v.source)
        return nullopt;

    switch (op.type) {
    case LuaToken::Type::SUB:
    {
        auto res_or = make_shared<SourceChangeOr>();
        if (auto result = v.source->forceValue(val {-get<double>(new_v)}); result) {
            if (auto p = dynamic_pointer_cast<sourceval>(v.source); !p || p->location[0].pos != op.pos+op.length) {
                res_or->alternatives.push_back(*result);
            }
        }
        if (auto result = v.source->forceValue(val {get<double>(new_v)}); result) {
            auto res_and = make_shared<SourceChangeAnd>();
            res_and->changes.push_back(*result);
            res_and->changes.push_back(SourceAssignment::create(op, ""));
            res_or->alternatives.push_back(res_and);
        }

        if (!res_or->alternatives.empty())
            return move(res_or);
        return nullopt;
    }
    case LuaToken::Type::EVAL:
        return v.source->forceValue(new_v);
    default:
        return nullopt;
    }
}

eval_result_t sourceunop::reevaluate() {
    auto _v = fst(v.reevaluate());

    switch (op.type) {
    case LuaToken::Type::SUB:
        return op_neg(_v, op);
    case LuaToken::Type::LEN:
        return op_len(_v);
    case LuaToken::Type::NOT:
        return op_not(_v);
    case LuaToken::Type::STRIP:
        return op_strip(_v);
    case LuaToken::Type::EVAL:
        return op_postfix_eval(_v);
    default:
        return string {op.match + " is not a unary operator"};
    }
}

bool sourceunop::isDirty() const {
    return v.source && v.source->isDirty();
}

}
}
