#include "MiniLua/sourceexp.hpp"
#include "MiniLua/operators.hpp"
#include "MiniLua/sourcechange.hpp"

#include <cmath>

namespace lua {
namespace rt {

sourceexp::~sourceexp() {}

source_change_t sourceval::forceValue(const val& v) const {

    auto sc = make_shared<SourceChangeAnd>();

    for (const auto& tok : location) {
        sc->changes.push_back(SourceAssignment::create(tok, ""));
    }

    dynamic_pointer_cast<SourceAssignment>(sc->changes[0])->replacement = v.literal();
    sc->changes[0]->hint = identifier;

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
            res_and->changes.back()->hint = identifier;
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
