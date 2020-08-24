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

}
}
