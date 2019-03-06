#include "include/luaast.h"
#include "include/luainterpreter.h"

ostream& operator<<(ostream& os, const LuaToken& token) {
    return os << "[" << static_cast<int>(token.type) << "]" << token.match;
}

namespace lua {
namespace rt {

ostream& operator<<(ostream& os, const val& value) {
    visit([&os](auto&& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (is_same_v<T, nil>) {
            os << "nil";
        }
        if constexpr (is_same_v<T, bool>) {
            os << (value ? "true" : "false");
        }
        if constexpr (is_same_v<T, double>) {
            os << value;
        }
        if constexpr (is_same_v<T, string>) {
            os << value;
        }
        if constexpr (is_same_v<T, shared_ptr<table>>) {
            os << value.get();
        }
    }, value);
    return os;
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
