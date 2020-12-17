#ifndef SOURCEEXP_H
#define SOURCEEXP_H

#include "luatoken.hpp"
#include "val.hpp"

namespace lua {
namespace rt {

struct sourceexp : std::enable_shared_from_this<sourceexp> {
    virtual ~sourceexp();
    virtual source_change_t forceValue(const val& v) const = 0;
    virtual eval_result_t reevaluate() = 0;
    virtual bool isDirty() const = 0;
    virtual vector<LuaToken> get_all_tokens() const = 0;

    string identifier = "";
};

struct sourceval : sourceexp {
    static shared_ptr<sourceval> create(const LuaToken& t) {
        auto ptr = make_shared<sourceval>();
        ptr->location.push_back(t);
        return ptr;
    }

    static shared_ptr<sourceval> create(const vector<LuaToken>& t) {
        auto ptr = make_shared<sourceval>();
        ptr->location = t;
        return ptr;
    }

    source_change_t forceValue(const val& v) const override;
    eval_result_t reevaluate() override;
    bool isDirty() const override;

    vector<LuaToken> get_all_tokens() const override { return location; }

    vector<LuaToken> location;
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

    source_change_t forceValue(const val& v) const override;
    eval_result_t reevaluate() override;
    bool isDirty() const override;

    vector<LuaToken> get_all_tokens() const override {
        vector<LuaToken> result = {op};
        if (lhs.source) {
            auto lhs_tokens = lhs.source->get_all_tokens();
            result.insert(end(result), begin(lhs_tokens), end(lhs_tokens));
        }
        if (rhs.source) {
            auto rhs_tokens = rhs.source->get_all_tokens();
            result.insert(end(result), begin(rhs_tokens), end(rhs_tokens));
        }
        return result;
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

    source_change_t forceValue(const val& new_v) const override;
    eval_result_t reevaluate() override;
    bool isDirty() const override;

    vector<LuaToken> get_all_tokens() const override {
        vector<LuaToken> result = {op};
        if (v.source) {
            auto v_tokens = v.source->get_all_tokens();
            result.insert(end(result), begin(v_tokens), end(v_tokens));
        }
        return result;
    }

    val v;
    LuaToken op;
};

} // namespace rt
} // namespace lua

#endif
