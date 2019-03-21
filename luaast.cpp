#include "include/luaast.h"
#include "include/luainterpreter.h"
#include <sstream>

string LuaToken::to_string() const {
    return "[" + std::to_string(static_cast<int>(type)) + "]" + match + " start:" + std::to_string(pos) + " length:" + std::to_string(length);
}

ostream& operator<<(ostream& os, const LuaToken& token) {
    return os << token.to_string();
}

namespace lua {
namespace rt {

string val::to_string() const {
    return visit([](auto&& value) -> string{
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
    }, static_cast<const val::value_t&>(*this));
//            + (source ? string("@") : "");
}

ostream& operator<<(ostream& os, const val& value) {
    return os << value.to_string();
}

optional<shared_ptr<SourceChange>> val::forceValue(const val& v) const {
    if (source)
        return source->forceValue(v);
    return nullopt;
}

}
}

VISITABLE_IMPL(_LuaName)
VISITABLE_IMPL(_LuaExplist)
VISITABLE_IMPL(_LuaValue)
VISITABLE_IMPL(_LuaOp)
VISITABLE_IMPL(_LuaUnop)
VISITABLE_IMPL(_LuaNameVar)
VISITABLE_IMPL(_LuaIndexVar)
VISITABLE_IMPL(_LuaMemberVar)
VISITABLE_IMPL(_LuaFunctioncall)
VISITABLE_IMPL(_LuaReturnStmt)
VISITABLE_IMPL(_LuaBreakStmt)
VISITABLE_IMPL(_LuaForStmt)
VISITABLE_IMPL(_LuaLoopStmt)
VISITABLE_IMPL(_LuaIfStmt)
VISITABLE_IMPL(_LuaChunk)
VISITABLE_IMPL(_LuaTableconstructor)
VISITABLE_IMPL(_LuaAssignment)
VISITABLE_IMPL(_LuaFunction)
VISITABLE_IMPL(_LuaField)
