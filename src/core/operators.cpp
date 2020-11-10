#include "MiniLua/operators.hpp"
#include "MiniLua/sourcechange.hpp"
#include "MiniLua/sourceexp.hpp"

#include <cmath>
#include <sstream>

namespace lua {
namespace rt {

eval_result_t op_add(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success({get<double>(a) + get<double>(b), sourcebinop::create(a, b, tok)});

    return string{"could not add values of type other than number (" + a.type() + ", " + b.type() +
                  ")"};
}

eval_result_t op_sub(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(
            lua::rt::val{get<double>(a) - get<double>(b), sourcebinop::create(a, b, tok)});

    return string{"could not subtract variables of type other than number"};
}

eval_result_t op_mul(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(
            lua::rt::val{get<double>(a) * get<double>(b), sourcebinop::create(a, b, tok)});

    return string{"could not multiply variables of type other than number"};
}

eval_result_t op_div(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(
            lua::rt::val{get<double>(a) / get<double>(b), sourcebinop::create(a, b, tok)});

    return string{"could not divide variables of type other than number"};
}

eval_result_t op_pow(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(
            lua::rt::val{pow(get<double>(a), get<double>(b)), sourcebinop::create(a, b, tok)});

    return string{"could not exponentiate variables of type other than number"};
}

eval_result_t op_mod(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(
            lua::rt::val{fmod(get<double>(a), get<double>(b)), sourcebinop::create(a, b, tok)});

    return string{"could not mod variables of type other than number"};
}

eval_result_t op_concat(lua::rt::val a, lua::rt::val b) {
    if ((holds_alternative<double>(a) || holds_alternative<string>(a)) &&
        (holds_alternative<double>(b) || holds_alternative<string>(b))) {

        stringstream ss;
        ss << a << b;
        return eval_success(lua::rt::val{ss.str()});
    }

    return string{"could not concatenate other types than strings or numbers"};
}

eval_result_t op_eval(lua::rt::val a, lua::rt::val b, const LuaToken& tok) {
    // cout << a.literal() << "\\" << b.literal() << endl;

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
    // cout << a.literal() << "\\" << endl;

    val result = a;
    result.source = sourceunop::create(a, tok);

    return eval_success(result, SourceAssignment::create(tok, "\\" + a.literal()));
}

eval_result_t op_lt(lua::rt::val a, lua::rt::val b) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(lua::rt::val{get<double>(a) < get<double>(b)});

    if (holds_alternative<string>(a) && holds_alternative<string>(b))
        return eval_success(lua::rt::val{get<string>(a) < get<string>(b)});

    return string{"only strings and numbers can be compared"};
}

eval_result_t op_leq(lua::rt::val a, lua::rt::val b) {
    if (holds_alternative<double>(a) && holds_alternative<double>(b))
        return eval_success(lua::rt::val{get<double>(a) <= get<double>(b)});

    if (holds_alternative<string>(a) && holds_alternative<string>(b))
        return eval_success(lua::rt::val{get<string>(a) <= get<string>(b)});

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

    return eval_success(visit(
        [&b](auto&& a) {
            using T = std::decay_t<decltype(a)>;
            return val{a == get<T>(b)};
        },
        static_cast<val::value_t>(a)));
}

eval_result_t op_neq(val a, val b) { return op_not(get_val(op_eq(a, b))); }

eval_result_t op_and(val a, val b) { return eval_success(a.to_bool() ? b : a); }

eval_result_t op_or(val a, val b) { return eval_success(a.to_bool() ? a : b); }

eval_result_t op_len(val v) {
    if (!v.istable()) {
        return string{"unary # can only be applied to a table (is " + v.type() + ")"};
    }

    table& t = *get<table_p>(v);

    int i = 1;
    for (;; i++) {
        if (auto idx = t.find(i); idx == t.end() || idx->second.isnil())
            break;
    }
    return eval_success(i - 1);
}

eval_result_t op_strip(val v) {
    v.source.reset();
    return eval_success(v);
}

eval_result_t op_not(val v) { return eval_success(!v.to_bool()); }

eval_result_t op_neg(val v, const LuaToken& tok) {
    if (holds_alternative<double>(v)) {
        return eval_success(val{-get<double>(v), sourceunop::create(v, tok)});
    }

    return string{"unary - can only be applied to a number"};
}

eval_result_t op_sqrt(val v) {
    if (holds_alternative<double>(v)) {
        struct sqrt_exp : sourceexp {
            sqrt_exp(const val& v) : v(v) {}

            optional<shared_ptr<SourceChange>> forceValue(const val& newval) const override {
                if (newval.isnumber())
                    if (double x = get<double>(newval) * get<double>(newval); isfinite(x))
                        return v.forceValue(x);
                return nullopt;
            }

            eval_result_t reevaluate() override {
                if (holds_alternative<double>(v)) {
                    return eval_success(sqrt(get<double>(v.reevaluate())));
                }
                return string{"sqrt can only be applied to a number"};
            }

            bool isDirty() const override { return v.source && v.source->isDirty(); }

            vector<LuaToken> get_all_tokens() const override {
                if (v.source)
                    return v.source->get_all_tokens();
                return {};
            }

            val v;
        };

        return eval_success(val{sqrt(get<double>(v)), std::make_shared<sqrt_exp>(v)});
    }

    return string{"sqrt can only be applied to a number"};
}

} // namespace rt
} // namespace lua
