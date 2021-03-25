#include <MiniLua/MiniLua.hpp>

//! [Using CallResult]
auto add_using_call_result(const minilua::CallContext& ctx) -> minilua::CallResult {
    auto arg1 = ctx.arguments().get(0);
    auto arg2 = ctx.arguments().get(1);

    return minilua::CallResult(minilua::Vallist(arg1 + arg2));
}
//! [Using CallResult]

//! [Using Value]
auto add_using_value(const minilua::CallContext& ctx) -> minilua::Value {
    auto arg1 = ctx.arguments().get(0);
    auto arg2 = ctx.arguments().get(1);

    return arg1 + arg2;
}
//! [Using Value]

//! [Using the global environment]
void add_to_global_env(const minilua::CallContext& ctx) {
    auto arg = ctx.arguments().get(0);
    auto value = ctx.environment().get("global_var");

    ctx.environment().add("global_var", value + arg);
}
//! [Using the global environment]

//! [Creating a table]
auto create_a_table(const minilua::CallContext& ctx) -> minilua::Value {
    auto key = ctx.arguments().get(0);
    auto value = ctx.arguments().get(1);

    auto table = ctx.make_table();
    table[key] = value;

    return table;
}
//! [Creating a table]

void __test_1() {
    minilua::Function _1(add_using_call_result);
    minilua::Function _2(add_using_value);
    minilua::Function _3(add_to_global_env);
    minilua::Function _4(create_a_table);
}
