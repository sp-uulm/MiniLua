#include "MiniLua/luatoken.hpp"

string LuaToken::to_string() const {
    return "[" + std::to_string(static_cast<int>(type)) + "]" + match + " start:" + std::to_string(pos) + " length:" + std::to_string(length);
}

ostream& operator<<(ostream& os, const LuaToken& token) {
    return os << token.to_string();
}
