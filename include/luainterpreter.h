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
if (auto eval_result = (exp)->accept(*this, (env), assign); holds_alternative<string>(eval_result)) {\
    return eval_result; \
} else { \
    varname = get<val>(eval_result); \
}

#define EVALR(varname, exp, env) \
val varname; \
    if (auto eval_result = (exp)->accept(*this, (env), {}); holds_alternative<string>(eval_result)) {\
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
eval_result_t op_sub(val a, val b, const LuaToken& tok = {LuaToken::Type::SUB, ""});
eval_result_t op_mul(val a, val b, const LuaToken& tok = {LuaToken::Type::MUL, ""});
eval_result_t op_div(val a, val b, const LuaToken& tok = {LuaToken::Type::DIV, ""});
eval_result_t op_pow(val a, val b, const LuaToken& tok = {LuaToken::Type::POW, ""});
eval_result_t op_mod(val a, val b, const LuaToken& tok = {LuaToken::Type::MOD, ""});
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
    virtual void apply(vector<LuaToken>&) const = 0;
};

struct SourceChangeOr : SourceChange {
    vector<shared_ptr<SourceChange>> alternatives;

    virtual string to_string() const override;
    virtual void apply(vector<LuaToken>&) const override;
};

struct SourceChangeAnd : SourceChange {
    vector<shared_ptr<SourceChange>> changes;

    virtual string to_string() const override;
    virtual void apply(vector<LuaToken>&) const override;
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
    virtual void apply(vector<LuaToken>&) const override;
};

struct sourceexp {
    virtual ~sourceexp();
    virtual optional<shared_ptr<SourceChange>> forceValue(const val& v) const = 0;
};

struct sourcelambda : sourceexp {
    template<typename T>
    static shared_ptr<sourcelambda> create(T func) {
        auto ptr = make_shared<sourcelambda>();
        ptr->f = func;
        return ptr;
    }

    function<optional<shared_ptr<SourceChange>>(const val&)> f;

    optional<shared_ptr<SourceChange>> forceValue(const val& v) const override {
        return f(v);
    }
};

struct sourceval : sourceexp {
    static shared_ptr<sourceval> create(const LuaToken& t) {
        auto ptr = make_shared<sourceval>();
        ptr->location = t;
        return ptr;
    }

    optional<shared_ptr<SourceChange>> forceValue(const val& v) const override{
        return SourceAssignment::create(location, v.to_string());
    }

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

    optional<shared_ptr<SourceChange>> forceValue(const val& v) const override {
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

    optional<shared_ptr<SourceChange>> forceValue(const val& new_v) const override {
        if (!holds_alternative<double>(new_v))
            return nullopt;

        switch (op.type) {
        case LuaToken::Type::SUB:
            if (v.source) {
                auto res_or = make_shared<SourceChangeOr>();
                if (auto result = v.source->forceValue(val {-get<double>(new_v)}); result) {
                    if (auto p = dynamic_pointer_cast<sourceval>(v.source); !p || p->location.pos != op.pos+op.length) {
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
                    return res_or;
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
