#include <MiniLua/MiniLua.hpp>
#include <algorithm>
#include <catch2/catch.hpp>
#include <functional>
#include <iostream>
#include <sstream>
#include <type_traits>

auto debug_values(minilua::CallContext ctx) -> minilua::CallResult {
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

TEST_CASE("Interpreter") {
    minilua::owning_ptr<minilua::Value> x{minilua::make_owning<minilua::Value>(std::string("hi"))};
    minilua::owning_ptr<minilua::Value> y = x;

    std::cout << *x << ", " << *y << "\n";

    minilua::owning_ptr<minilua::Value> z{minilua::make_owning<minilua::Value>(std::string("y"))};
    x = z;

    std::cout << *x << ", " << *y << "\n";

    minilua::Interpreter interpreter;

    // populate the environment
    interpreter.environment().add_default_stdlib();

    auto lambda = [](minilua::CallContext ctx) { return std::string{"force something"}; };

    std::cout << "as_native_function: ";
    minilua::NativeFunction as_native_function = lambda;

    // add a single variable to the environment
    interpreter.environment().add("func1", lambda);
    interpreter.environment().add("num1", 5);

    // add multiple variables to the environment
    interpreter.environment().add_all(
        {{"num2", 128},
         {"num3", 1.31},
         {"func2", debug_values},
         {"func3", [](minilua::CallContext ctx) { std::cout << "func3 -> void\n"; }},
         {"func4",
          [](minilua::CallContext ctx) -> minilua::Vallist {
              return {1, std::string{"hi"}};
          }},
         {"tabl", minilua::Table({
                      {std::string("key1"), 25.0},
                      {std::string("key2"), std::string("value")},
                  })}});

    std::cout << minilua::Value() << "\n";
    std::cout << minilua::Value(minilua::Number(25)) << "\n";
    std::cout << minilua::Value(minilua::String("hi")) << "\n";

    std::cout << interpreter.environment() << "\n";

    // parse and run a program
    interpreter.parse("print(120)");
    minilua::EvalResult result = interpreter.run();

    // chose source changes to apply
    // TODO do we need a vector here or is is ok to assume that one run of the
    //      program only causes one source change?
    const auto* previous_hint = "x_coord";

    for (auto& suggestion : result.source_change_suggestions) {
        if (suggestion.origin == "gui_drag_line") {
            if (suggestion.hint == previous_hint) {
                interpreter.apply_source_changes(std::vector{suggestion.change});
                break;
            }
        }
    }
}

TEST_CASE("table") {
    minilua::Table table;

    table.set(5, "value1");
    CHECK(table.get(5) == "value1");

    minilua::Value val1 = table.get(5);

    table.set(5, "value2");
    table.set("hi", "value1");

    CHECK(table.get(5) == "value2");
    CHECK(table.get("hi") == "value1");
    CHECK(val1 == "value1");

    table.set("table", minilua::Table());
    CAPTURE(table);

    auto table2 = std::get<minilua::Table>(table.get("table").get());
    table2.set("x", 22);

    CAPTURE(table);

    minilua::Table table3;
    table3.set("y", 23);

    CHECK(table.get("table") == table2);

    table.set("table", table3);

    CAPTURE(table);
    FAIL();
}
