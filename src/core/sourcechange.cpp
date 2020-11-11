#include "MiniLua/sourcechange.hpp"

#include <sstream>

namespace lua {
namespace rt {

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

void ApplySCVisitor::visit(const SourceAssignment& sc_ass) { changes.push_back(sc_ass); }

vector<LuaToken> ApplySCVisitor::apply_changes(const vector<LuaToken>& tokens) {
    auto new_tokens = tokens;

    // sort the collected changes according to their position
    std::sort(changes.begin(), changes.end(), [](const auto& a, const auto& b) {
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

optional<string> default_source_change_label(const val& v) {
    if (!v.source)
        return nullopt;

    auto possible_changes = v.forceValue(v);

    if (!possible_changes)
        return nullopt;

    ApplySCVisitor visitor;
    (*possible_changes)->accept(visitor);
    for (const auto& c : visitor.changes) {
        if (c.hint != "" && c.hint != "?")
            return c.to_string();
    }

    return visitor.changes.front().to_string();
}

vector<string> source_change_labels(const val& v) {
    if (!v.source)
        return vector<string>();

    auto possible_changes = v.forceValue(v);

    if (!possible_changes)
        return vector<string>();

    vector<string> result;

    struct SCLabelVisior : ApplySCVisitor {
        virtual void visit(const SourceChangeOr& sc_or) override {
            for (const auto& c : sc_or.alternatives) {
                c->accept(*this);
            }
        }

        virtual void visit(const SourceChangeAnd& sc_and) override {
            if (!sc_and.changes.empty())
                sc_and.changes.back()->accept(*this);
        }

    } visitor;

    (*possible_changes)->accept(visitor);
    for (const auto& c : visitor.changes) {
        result.push_back(c.to_string());
    }

    return result;
}

/*
 * removes the first alternative from a source change recursively. It does a
 * depth first search for a source assignment and removes it, removing empty
 * or/and nodes in the process.
 *
 * Precondition: the source changes must result from a call to x.forceValue(x)
 * as it compares the match and replacement in the SourceAssignment to discard
 * any SourceAssignments that do not influence the current source change.
 * (probably not needed?)
 *
 * It returns true if a matching assignment was found, false otherwise.
 */

static bool remove_alternative(shared_ptr<SourceChange>& changes) {
    if (auto node = dynamic_pointer_cast<SourceAssignment>(changes)) {
        if (node->token.match == node->replacement) {
            // the node is a possibility to change the source -> return true
            return true;
        }
    } else if (auto node = dynamic_pointer_cast<SourceChangeAnd>(changes)) {
        for (unsigned i = 0; i < node->changes.size(); ++i) {
            auto& c = node->changes[i];
            if (remove_alternative(c)) {
                if (auto child_node = dynamic_pointer_cast<SourceChangeOr>(c); child_node && child_node->alternatives.empty()) {
                    node->changes.erase(begin(node->changes) + i);
                } else if (auto child_node = dynamic_pointer_cast<SourceChangeAnd>(c); child_node && child_node->changes.empty()) {
                    node->changes.erase(begin(node->changes) + i);
                } else if (auto child_node = dynamic_pointer_cast<SourceAssignment>(c); child_node) {
                    node->changes.erase(begin(node->changes) + i);
                }
                return true;
            }
        }
        return false;
    } else if (auto node = dynamic_pointer_cast<SourceChangeOr>(changes)) {
        for (unsigned i = 0; i < node->alternatives.size(); ++i) {
            auto& c = node->alternatives[i];
            if (remove_alternative(c)) {
                if (auto child_node = dynamic_pointer_cast<SourceChangeOr>(c); child_node && child_node->alternatives.empty()) {
                    node->alternatives.erase(begin(node->alternatives) + i);
                } else if (auto child_node = dynamic_pointer_cast<SourceChangeAnd>(c); child_node && child_node->changes.empty()) {
                    node->alternatives.erase(begin(node->alternatives) + i);
                } else if (auto child_node = dynamic_pointer_cast<SourceAssignment>(c); child_node) {
                    node->alternatives.erase(begin(node->alternatives) + i);
                }
                return true;
            }
        }
        return false;
    }
    return false;
}

optional<shared_ptr<SourceChange>> get_sc_for_hint(const val& v, const string& hint) {
    if (!v.source)
        return nullopt;

    auto possible_changes = v.forceValue(v);

    if (!possible_changes)
        return nullopt;

    auto source_changes = make_shared<SourceChangeAnd>();

    for(;;) {
        ApplySCVisitor visitor;
        (*possible_changes)->accept(visitor);
        for (const auto& c : visitor.changes) {
            std::cout << c.hint << std::endl;
            if (c.to_string() == hint) {
                // we don't have to do more!
                return source_changes;
            }
        }

        if (visitor.changes.empty())
            break;

        // remove current alternative and add it as a source change with $ to source_changes
        remove_alternative(*possible_changes);
        for (const auto& c : visitor.changes) {
            if (c.token.match == c.replacement) {
                source_changes->changes.push_back(SourceAssignment::create(c.token, "$" + c.replacement));
            }
        }
    }

    // We could not change the source to modify hint :-(
    return nullopt;
}

} // namespace rt
} // namespace lua
