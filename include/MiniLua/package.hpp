#ifndef MINILUA_PACKAGE
#define MINILUA_PACKAGE
#endif

#define MINILUA_ROOT "/usr/"
#define MINILUA_VDIR "5.3"  // hard coded because minilua support is only planned for lua 5.3
#define MINILUA_LDIR MINILUA_ROOT "share/lua/" MINILUA_VDIR "/"
#define MINILUA_CDIR MINILUA_ROOT "lib/lua/" MINILUA_VDIR "/"

#define MINILUA_CPATH_DEFAULT \
        MINILUA_CDIR"?.so;" MINILUA_CDIR"loadall.so;" "./?.so"

#define MINILUA_PATH_DEFAULT \
        MINILUA_LDIR"?.lua;" MINILUA_LDIR"?/init.lua;" \
        MINILUA_CDIR"?.lua;" MINILUA_CDIR"?/init.lua;" \
        "./?.lua;" "./?/init.lua"

#include <MiniLua/values.hpp>

namespace minilua {
auto require(const CallContext& ctx) -> Value;

namespace package {
const Value CPATH = Value(std::getenv("LUA_CPATH_5_3") != nullptr ? std::getenv("LUA_CPATH_5_3") :
            (std::getenv("LUA_CPATH") != nullptr ? std::getenv("LUA_CPATH") : MINILUA_CPATH_DEFAULT));
const Value PATH = Value(std::getenv("LUA_PATH_5_3") != nullptr ? std::getenv("LUA_PATH_5_3") :
            (std::getenv("LUA_PATH") != nullptr ? std::getenv("LUA_PATH") : MINILUA_PATH_DEFAULT));
auto searchpath(const CallContext& ctx) -> Vallist;
} // end namespace package
} // end namespace minilua