#ifndef SOURCECHANGE_H
#define SOURCECHANGE_H

#include "luatoken.hpp"
#include "val.hpp"

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

    virtual void accept(SourceChangeVisitor& v) const override { v.visit(*this); }
};

struct SourceChangeAnd : SourceChange {
    vector<shared_ptr<SourceChange>> changes;

    virtual string to_string() const override;

    virtual void accept(SourceChangeVisitor& v) const override { v.visit(*this); }
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

    virtual void accept(SourceChangeVisitor& v) const override { v.visit(*this); }
};

inline source_change_t operator|(const source_change_t& lhs, const source_change_t& rhs) {
    if (lhs && rhs) {
        auto sc_or = make_shared<SourceChangeOr>();
        sc_or->alternatives = {*lhs, *rhs};

        return move(sc_or);
    }

    return lhs ? lhs : rhs;
}

inline source_change_t operator&(const source_change_t& lhs, const source_change_t& rhs) {
    if (lhs && rhs) {
        auto sc_and = make_shared<SourceChangeAnd>();
        sc_and->changes = {*lhs, *rhs};

        return move(sc_and);
    }

    return lhs ? lhs : rhs;
}

// adds a source change to an eval_result_t
inline eval_result_t operator<<(const eval_result_t& lhs, const source_change_t& rhs) {
    if (holds_alternative<string>(lhs))
        return lhs;

    return eval_success(get_val(lhs), get_sc(lhs) & rhs);
}

// helper functions to name and choose the source change variants

/*
 * returns a label for the source change that is automatically chosen when v is
 * changed. This can be used e.g. for a mouseover to show the user, what will be
 * changed if the value is manipulated. As an additional hint, the name of the
 * first variable binding of the source value is also encoded. The actual output
 * format might be subject to change.
 *
 * Example (not the exact output):
 *
 * radius = 7.3
 * v = radius + 7
 * default_source_change_label(v) => "location 10,3 [hint: radius]"
 */
optional<string> default_source_change_label(const val& v);

/*
 * returns a list of labels of all possible variants of source changes. The
 * alternatives are named after the location, that is changed when the changes
 * are applied. As an additional hint, the name of the first variable binding of
 * the source value is also encoded.
 */
vector<string> source_change_labels(const val& v);

/*
 * get the source change, so that modifications to v cause a change of the
 * source identified by hint. This is done by inserting $ operator to direct the
 * changes to the desired source and not other alternatives.
 *
 * Example:
 * a = 5
 * b = 3
 * x = a+b
 *
 * get_sc_for_hint(x, "b") => change 5 -> $5
 *
 * The hint is an attempt to enumerate the
 * different alternatives for source changes. A more sophisticated structure
 * than a string is probably necessary in the future.
 */
optional<shared_ptr<SourceChange>> get_sc_for_hint(const val& v, const string& hint);

} // namespace rt
} // namespace lua

#endif
