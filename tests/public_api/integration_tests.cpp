#include "MiniLua/environment.hpp"
#include "MiniLua/source_change.hpp"
#include <MiniLua/MiniLua.hpp>
#include <algorithm>
#include <catch2/catch.hpp>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <variant>

auto debug_values(const minilua::CallContext& ctx) -> minilua::CallResult {
    std::vector<minilua::Value> values;
    values.reserve(ctx.arguments().size());

    std::transform(
        ctx.arguments().begin(), ctx.arguments().end(), values.begin(),
        [](const minilua::Value& value) {
            std::stringstream ss;
            ss << value;
            return ss.str();
        });

    return values;
}

TEST_CASE("Interpreter integration test") {
    minilua::Interpreter interpreter;

    // populate the environment
    interpreter.environment().add_default_stdlib();

    auto lambda = [](minilua::CallContext /*unused*/) { // NOLINT
        return std::string{"force something"};
    };

    minilua::NativeFunction as_native_function = lambda;

    // add a single variable to the environment
    interpreter.environment().add("func1", lambda);
    interpreter.environment().add("num1", 5); // NOLINT

    // add multiple variables to the environment
    interpreter.environment().add_all(
        {{"num2", 128},  // NOLINT
         {"num3", 1.31}, // NOLINT
         {"func2", debug_values},
         {"func3", [](const minilua::CallContext& /*unused*/) { std::cout << "func3 -> void\n"; }},
         {"func4",
          [](minilua::CallContext /*unused*/) -> minilua::Vallist { // NOLINT
              return {1, std::string{"hi"}};
          }},
         {"tabl", minilua::Table({
                      {std::string("key1"), 25.0}, // NOLINT
                      {std::string("key2"), std::string("value")},
                  })},
         {"forceValue", [](minilua::CallContext ctx) -> minilua::CallResult {
              // auto [arg1, arg2] = ctx.arguments();
              auto arg1 = ctx.arguments().get(0);
              auto arg2 = ctx.arguments().get(1);
              auto change = ctx.force_value(arg1, arg2);
              change.set_origin("forceValue");
              return change;
          }}});

    std::cout << interpreter.environment() << "\n";

    // parse and run a program
    interpreter.parse("x_coord = 10; forceValue(x_coord, 25)");
    minilua::EvalResult result = interpreter.evaluate();

    // choose source changes to apply
    // TODO do we need a vector here or is is ok to assume that one run of the
    //      program only causes one source change?
    const auto* previous_hint = "x_coord";

    for (auto& source_change : result.source_changes) {
        if (source_change.origin() == "gui_drag_line") {
            if (source_change.hint() == previous_hint) {
                interpreter.apply_source_change(source_change);
                break;
            }
        }
    }
}

TEST_CASE("minilua::Table") {
    minilua::Table table;

    table.set(5, "value1"); // NOLINT
    CHECK(table.get(5) == "value1");

    minilua::Value val1 = table.get(5); // NOLINT

    table.set(5, "value2"); // NOLINT
    table.set("hi", "value1");

    CHECK(table.get(5) == "value2");
    CHECK(table.get("hi") == "value1");
    CHECK(val1 == "value1");

    table.set("table", minilua::Table());
    CAPTURE(table);

    auto table2 = std::get<minilua::Table>(table.get("table").get());
    table2.set("x", 22); // NOLINT

    CAPTURE(table);

    minilua::Table table3;
    table3.set("y", 23); // NOLINT

    CHECK(table.get("table") == table2);

    table.set("table", table3);

    CAPTURE(table);
    // FAIL();
}
