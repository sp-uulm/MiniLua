#include <cmath>
#include <iostream>
#include <stdexcept>

#include "MiniLua/math.hpp"
#include "MiniLua/stdlib.hpp"
#include "MiniLua/utils.hpp"

namespace minilua::math {

auto abs(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);
    if (res != Nil()) {
        auto num = get<Number>(res);

        return Value(std::abs(num.value));
    } else {
        throw std::runtime_error("bad argument #1 to abs (number expected, got " + x.type() + ")");
    }
}
} // namespace minilua::math