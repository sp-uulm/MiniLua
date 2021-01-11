#include <cmath>
#include <iostream>

#include "MiniLua/math.hpp"
#include "MiniLua/stdlib.hpp"
#include "MiniLua/utils.hpp"

namespace minilua::math {

auto abs(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    cout << minilua::to_number(ctx).raw() << endl;
    auto num = get<Number>(minilua::to_number(ctx));

    cout << std::abs(num.value) << endl;

    return Value(std::abs(num.value));
}
} // namespace minilua::math