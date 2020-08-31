#ifndef SOURCECHANGE_H
#define SOURCECHANGE_H

#include "val.hpp"
#include "luatoken.hpp"

namespace lua {
namespace rt {

struct SourceChangeVisitor {
    virtual ~SourceChangeVisitor();

    virtual void visit(const struct SourceChangeOr& sc) = 0;
    virtual void visit(const struct SourceChangeAnd& sc) = 0;
    virtual void visit(const struct SourceAssignment& sc) = 0;
};

struct ApplySCVisitor : public SourceChangeVisitor {
    void visit(const SourceChangeOr& sc) override;
    void visit(const SourceChangeAnd& sc) override;
    void visit(const SourceAssignment& sc) override;

    vector<LuaToken> apply_changes(const vector<LuaToken>& tokens);

    std::vector<SourceAssignment> changes;
};

struct SourceChange {
    virtual ~SourceChange();
    virtual string to_string() const = 0;

    vector<LuaToken> apply(const vector<LuaToken>& tokens) {
        ApplySCVisitor vis;
        accept(vis);

        return vis.apply_changes(tokens);
    }

    virtual void accept(SourceChangeVisitor& v) const = 0;
    std::string hint = "?";
};

struct SourceChangeOr : SourceChange {
    vector<shared_ptr<SourceChange>> alternatives;

    virtual string to_string() const override;

    virtual void accept(SourceChangeVisitor& v) const override {
        v.visit(*this);
    }
};

struct SourceChangeAnd : SourceChange {
    vector<shared_ptr<SourceChange>> changes;

    virtual string to_string() const override;

    virtual void accept(SourceChangeVisitor& v) const override {
        v.visit(*this);
    }
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
        return token.to_string() + " -> " + replacement + " [" + hint + "]";
    }

    virtual void accept(SourceChangeVisitor& v) const override {
        v.visit(*this);
    }
};

inline source_change_t operator| (const source_change_t& lhs, const source_change_t& rhs) {
    if (lhs && rhs) {
        auto sc_or = make_shared<SourceChangeOr>();
        sc_or->alternatives = {*lhs, *rhs};

        return move(sc_or);
    }

    return lhs ? lhs : rhs;
}

inline source_change_t operator& (const source_change_t& lhs, const source_change_t& rhs) {
    if (lhs && rhs) {
        auto sc_and = make_shared<SourceChangeAnd>();
        sc_and->changes = {*lhs, *rhs};

        return move(sc_and);
    }

    return lhs ? lhs : rhs;
}

// adds a source change to an eval_result_t
inline eval_result_t operator<< (const eval_result_t& lhs, const source_change_t& rhs) {
    if (holds_alternative<string>(lhs))
        return lhs;

    return eval_success(get_val(lhs), get_sc(lhs) & rhs);
}

}
}

#endif
