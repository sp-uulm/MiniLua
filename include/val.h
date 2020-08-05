#ifndef VAL_H
#define VAL_H

#include <string>
#include <memory>
#include <variant>
#include <unordered_map>
#include <type_traits>
#include <functional>

using namespace std;

using LuaAST = shared_ptr<struct _LuaAST>;
using LuaName = shared_ptr<struct _LuaName>;
using LuaExp = shared_ptr<struct _LuaExp>;
using LuaOp = shared_ptr<struct _LuaOp>;
using LuaUnop = shared_ptr<struct _LuaUnop>;
using LuaExplist = shared_ptr<struct _LuaExplist>;
using LuaFunctioncall = shared_ptr<struct _LuaFunctioncall>;
using LuaValue = shared_ptr<struct _LuaValue>;
using LuaVar = shared_ptr<struct _LuaVar>;
using LuaAssignment = shared_ptr<struct _LuaAssignment>;
using LuaNameVar = shared_ptr<struct _LuaNameVar>;
using LuaIndexVar = shared_ptr<struct _LuaIndexVar>;
using LuaMemberVar = shared_ptr<struct _LuaMemberVar>;
using LuaStmt = shared_ptr<struct _LuaStmt>;
using LuaReturnStmt = shared_ptr<struct _LuaReturnStmt>;
using LuaBreakStmt = shared_ptr<struct _LuaBreakStmt>;
using LuaForStmt = shared_ptr<struct _LuaForStmt>;
using LuaLoopStmt = shared_ptr<struct _LuaLoopStmt>;
using LuaIfStmt = shared_ptr<struct _LuaIfStmt>;
using LuaChunk = shared_ptr<struct _LuaChunk>;
using LuaTableconstructor = shared_ptr<struct _LuaTableconstructor>;
using LuaField = shared_ptr<struct _LuaField>;
using LuaFunction = shared_ptr<struct _LuaFunction>;

namespace lua {
namespace rt {

using nil = monostate;

using cfunction_p = shared_ptr<struct cfunction>;
using lfunction_p = shared_ptr<struct lfunction>;
using table_p = shared_ptr<struct table>;
using vallist_p = shared_ptr<struct vallist>;

using _val_t = variant<nil, bool, double, string, cfunction_p, table_p, vallist_p, lfunction_p>;
struct val : _val_t {
    using value_t = _val_t;

    val& operator=(const val&) = default;
    val(const val&) = default;
    val() : value_t {nil()} {}

    val(nil v, const shared_ptr<struct sourceexp>& source = nullptr) : value_t {v}, source {source} {}
    val(bool v, const shared_ptr<struct sourceexp>& source = nullptr) : value_t {v}, source {source} {}
    val(double v, const shared_ptr<struct sourceexp>& source = nullptr) : value_t {v}, source {source} {}
    val(int v, const shared_ptr<struct sourceexp>& source = nullptr) : value_t {static_cast<double>(v)}, source {source} {}
    val(string v, const shared_ptr<struct sourceexp>& source = nullptr) : value_t {v}, source {source} {}
    val(const char* v, const shared_ptr<struct sourceexp>& source = nullptr) : value_t {string{v}}, source {source} {}
    val(cfunction_p v, const shared_ptr<struct sourceexp>& source = nullptr) : value_t {v}, source {source} {}
    val(table_p v, const shared_ptr<struct sourceexp>& source = nullptr) : value_t {v}, source {source} {}
    val(vallist_p v, const shared_ptr<struct sourceexp>& source = nullptr) : value_t {v}, source {source} {}
    val(lfunction_p v, const shared_ptr<struct sourceexp>& source = nullptr) : value_t {v}, source {source} {}

    template <typename... T>
    val(function<T...>&& v, const shared_ptr<struct sourceexp>& source = nullptr) : value_t {make_shared<cfunction>(v)}, source {source} {}

    bool to_bool() const {
        return !isnil() && (!isbool() || get<bool>(*this));
    }

    string to_string() const;
    string literal() const;

    string type() const {
        switch(index()) {
        case 0: return "nil";
        case 1: return "bool";
        case 2: return "number";
        case 3: return "string";
        case 4: return "function";
        case 5: return "table";
        case 6: return "vallist";
        case 7: return "function";
        default: return "invalid";
        }
    }

    bool isbool() const {
        return index() == 1;
    }

    bool isnumber() const {
        return index() == 2;
    }

    bool isstring() const {
        return index() == 3;
    }

    bool istable() const {
        return index() == 5;
    }

    bool isnil() const {
        return index() == 0;
    }

    double def_number(double def = 0.0) const {
        if (isnumber())
            return get<double>(*this);
        return def;
    }

    optional<shared_ptr<struct SourceChange>> forceValue(const val& v) const;
    val reevaluate();

    shared_ptr<struct sourceexp> source;
};

}}
namespace std {
template<>
struct hash<lua::rt::val> {
    size_t operator()(const lua::rt::val &v) const {
        return hash<lua::rt::val::value_t>()(v);
    }
};
}

namespace lua {
namespace rt {

using assign_t = optional<tuple<val, bool>>;
struct ASTEvaluator;
struct Environment;

struct table : public unordered_map<val, val> {
    table() {}
    table(const vector<pair<val, val>>& content) {
        for (const auto& p : content)
            operator[](p.first) = p.second;
    }
};

struct vallist : public vector<val> {
    template <typename... T>
    vallist(T&&... v) : vector<val> {forward<T>(v)...} {}
};

struct cfunction {
    using result = variant<vallist, string>;

    template <typename T>
    cfunction(T f) {
        if constexpr (is_convertible<T, function<result(const vallist&, const _LuaFunctioncall&)>>::value) {
            this->f = f;
        } else {
            this->f = [f](const vallist& args, const _LuaFunctioncall&) mutable -> result {return f(args);};
        }
    }
    function<result(const vallist&, const _LuaFunctioncall&)> f;
};

struct lfunction {
    lfunction(const LuaChunk& f, const LuaExplist& params, const shared_ptr<Environment>& env) : f{f}, params{params}, env{env} {}
    LuaChunk f;
    LuaExplist params;
    shared_ptr<Environment> env;
};

using source_change_t = optional<shared_ptr<SourceChange>>;
using eval_success_t = pair<val, source_change_t>;
using eval_result_t = variant<eval_success_t, string>;

inline eval_result_t eval_success(const val& v, optional<shared_ptr<SourceChange>> sc = nullopt) {
    return make_pair(v, sc);
}

inline val get_val(const eval_result_t& result) {
    return get<eval_success_t>(result).first;
}

inline optional<shared_ptr<SourceChange>> get_sc(const eval_result_t& result) {
    return get<eval_success_t>(result).second;
}

inline val unwrap(const eval_result_t& result) {
    if (holds_alternative<string>(result))
        throw runtime_error(get<string>(result));
    return get_val(result);
}

val fst(const val& v);
vallist flatten(const vallist& list);

ostream& operator<<(ostream& os, const val& value);

}
}

#endif
