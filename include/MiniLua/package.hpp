#ifndef MINILUA_PACKAGE
#define MINILUA_PACKAGE
#endif

#include <MiniLua/values.hpp>

namespace minilua {
auto require(const CallContext& ctx) -> Value;

namespace package {} // end namespace package
} // end namespace minilua