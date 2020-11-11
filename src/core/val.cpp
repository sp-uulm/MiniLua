#include "MiniLua/val.hpp"
#include "MiniLua/sourceexp.hpp"

#include <sstream>

namespace lua {
namespace rt {

string val::literal() const {
    return visit(
        [](auto&& value) -> string {
            using T = std::decay_t<decltype(value)>;
            if constexpr (is_same_v<T, nil>) {
                return "nil";
            }
            if constexpr (is_same_v<T, bool>) {
                return (value ? "true" : "false");
            }
            if constexpr (is_same_v<T, double>) {
                stringstream ss;
                ss << value;
                return ss.str();
            }
            if constexpr (is_same_v<T, string>) {
                return "'" + value + "'";
            }
            if constexpr (is_same_v<T, shared_ptr<table>>) {
                stringstream ss;
                ss << "{";
                for (auto [k, v] : *value) {
                    ss << "[" << k.literal() << "]=" << v.literal() << ",";
                }
                ss << "}";
                return ss.str();
            }
            return "";
        },
        static_cast<const val::value_t&>(*this));
    //            + (source ? string("@") : "");
}

string val::to_string() const {
    return visit(
        [](auto&& value) -> string {
            using T = std::decay_t<decltype(value)>;
            if constexpr (is_same_v<T, nil>) {
                return "nil";
            }
            if constexpr (is_same_v<T, bool>) {
                return (value ? "true" : "false");
            }
            if constexpr (is_same_v<T, double>) {
                stringstream ss;
                ss << value;
                return ss.str();
            }
            if constexpr (is_same_v<T, string>) {
                return value;
            }
            if constexpr (is_same_v<T, shared_ptr<table>>) {
                return std::to_string(reinterpret_cast<uint64_t>(value.get()));
            }
            return "";
        },
        static_cast<const val::value_t&>(*this));
    //            + (source ? string("@") : "");
}

ostream& operator<<(ostream& os, const val& value) { return os << value.to_string(); }

optional<shared_ptr<SourceChange>> val::forceValue(const val& v) const {
    if (source)
        return source->forceValue(v);
    return nullopt;
}

val val::reevaluate() {
    if (source && source->isDirty()) {
        if (auto result = source->reevaluate(); holds_alternative<eval_success_t>(result))
            return get_val(result);

        // reevaluation failed: return original value
    }
    return *this;
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

    for (int i = 0; i < static_cast<int>(list.size()) - 1; ++i) {
        result.push_back(fst(list[i]));
    }

    if (holds_alternative<vallist_p>(list.back())) {
        const vallist& vl = *get<vallist_p>(list.back());
        for (unsigned i = 0; i < vl.size(); ++i) {
            result.push_back(vl[i]);
        }
    } else {
        result.push_back(list.back());
    }

    return result;
}

} // namespace rt
} // namespace lua
