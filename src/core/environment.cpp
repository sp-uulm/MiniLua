#include "environment.h"
#include "sourceexp.h"
#include "operators.h"

#include <cmath>

namespace lua {
namespace rt {

void Environment::assign(const val& var, const val& newval, bool is_local) {
    // cout << "assignment " << var << "=" << newval << (is_local ? " (local)" : "") << endl;

    if (newval.source && newval.source->identifier.empty()) {
        newval.source->identifier = var.to_string();
    }

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

    (*math)["atan"] = make_shared<cfunction>([](const vallist& args) -> cfunction::result {
        if (args.size() != 1 || !args[0].isnumber()) {
            return vallist{nil(), string {"atan: one number argument expected"}};
        }

        val result = atan(get<double>(args[0]));
        if (args[0].source) {
            struct atan_exp : sourceexp {
                atan_exp(const val& x) : x(x) {}

                optional<shared_ptr<SourceChange>> forceValue(const val& newval) const override{
                    if (newval.isnumber())
                        if (double result = tan(get<double>(newval)); isfinite(result))
                            return x.forceValue(result);
                    return nullopt;
                }

                eval_result_t reevaluate() override {
                    if (holds_alternative<double>(x)) {
                        return eval_success(atan(get<double>(x.reevaluate())));
                    }
                    return string{"atan can only be applied to numbers"};
                }

                bool isDirty() const override {
                    return (x.source && x.source->isDirty());
                }

                vector<LuaToken> get_all_tokens() const override {
                    vector<LuaToken> result;
                    if (x.source) {
                        auto rhs_tokens = x.source->get_all_tokens();
                        result.insert(end(result), begin(rhs_tokens), end(rhs_tokens));
                    }
                    return result;
                }

                val x;
            };

            result.source = std::make_shared<atan_exp>(args[0]);
        }

        return {result};
    });

    (*math)["acos"] = make_shared<cfunction>([](const vallist& args) -> cfunction::result {
        if (args.size() != 1 || !args[0].isnumber()) {
            return vallist{nil(), string {"acos: one number argument expected"}};
        }

        val result = acos(get<double>(args[0]));
        if (args[0].source) {
            struct acos_exp : sourceexp {
                acos_exp(const val& x) : x(x) {}

                optional<shared_ptr<SourceChange>> forceValue(const val& newval) const override{
                    if (newval.isnumber())
                        if (double result = cos(get<double>(newval)); isfinite(result))
                            return x.forceValue(result);
                    return nullopt;
                }

                eval_result_t reevaluate() override {
                    if (holds_alternative<double>(x)) {
                        return eval_success(acos(get<double>(x.reevaluate())));
                    }
                    return string{"acos can only be applied to numbers"};
                }

                bool isDirty() const override {
                    return (x.source && x.source->isDirty());
                }

                vector<LuaToken> get_all_tokens() const override {
                    vector<LuaToken> result;
                    if (x.source) {
                        auto rhs_tokens = x.source->get_all_tokens();
                        result.insert(end(result), begin(rhs_tokens), end(rhs_tokens));
                    }
                    return result;
                }

                val x;
            };

            result.source = std::make_shared<acos_exp>(args[0]);
        }

        return {result};
    });

    (*math)["asin"] = make_shared<cfunction>([](const vallist& args) -> cfunction::result {
        if (args.size() != 1 || !args[0].isnumber()) {
            return vallist{nil(), string {"asin: one number argument expected"}};
        }

        val result = asin(get<double>(args[0]));
        if (args[0].source) {
            struct asin_exp : sourceexp {
                asin_exp(const val& x) : x(x) {}

                optional<shared_ptr<SourceChange>> forceValue(const val& newval) const override{
                    if (newval.isnumber())
                        if (double result = sin(get<double>(newval)); isfinite(result))
                            return x.forceValue(result);
                    return nullopt;
                }

                eval_result_t reevaluate() override {
                    if (holds_alternative<double>(x)) {
                        return eval_success(asin(get<double>(x.reevaluate())));
                    }
                    return string{"asin can only be applied to numbers"};
                }

                bool isDirty() const override {
                    return (x.source && x.source->isDirty());
                }

                vector<LuaToken> get_all_tokens() const override {
                    vector<LuaToken> result;
                    if (x.source) {
                        auto rhs_tokens = x.source->get_all_tokens();
                        result.insert(end(result), begin(rhs_tokens), end(rhs_tokens));
                    }
                    return result;
                }

                val x;
            };

            result.source = std::make_shared<asin_exp>(args[0]);
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

    (*math)["floor"] = make_shared<cfunction>([](const vallist& args) -> cfunction::result {
        if (args.size() != 1 || !args[0].isnumber()) {
            return vallist{nil(), string {"floor: one number argument expected"}};
        }

        val result = floor(get<double>(args[0]));
        result.source = args[0].source;

        return {result};
    });

    (*math)["pi"] = 3.1415926;

    t["_G"] = shared_ptr<table>(shared_from_this(), &t);

    t["__visit_count"] = 0.0;
    t["__visit_limit"] = 500.0;
}

}
}
