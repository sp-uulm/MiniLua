#include <MiniLua/MiniLua.hpp>
#include <catch2/catch.hpp>
#include <functional>
#include <iostream>
#include <type_traits>

minilua::Vallist native_function(minilua::CallContext ctx) { return {1, 2, "hi"}; }

TEST_CASE("Interpreter") {
    minilua::owning_ptr<minilua::Value> x{std::string("hi")};
    minilua::owning_ptr<minilua::Value> y = x;

    std::cout << x << ", " << y << "\n";

    minilua::owning_ptr<minilua::Value> z{std::string("y")};
    x = z;

    std::cout << x << ", " << y << "\n";

    minilua::Interpreter interpreter;

    // populate the environment
    interpreter.environment().add_default_stdlib();

    auto lambda = [](minilua::CallContext ctx) { return std::string{"force something"}; };

    minilua::NativeFunction as_native_function = lambda;

    // add a single variable to the environment
    interpreter.environment().add("func1", lambda);
    interpreter.environment().add("num1", 5);

    // add multiple variables to the environment
    interpreter.environment().add_all(
        {{"num2", 128},
         {"num3", 1.31},
         {"func2", native_function},
         {"func3", [](minilua::CallContext ctx) { std::cout << "func3 -> void\n"; }},
         {"func4",
          [](minilua::CallContext ctx) -> minilua::Vallist {
              return {1, std::string{"hi"}};
          }},
         {"tabl", minilua::Table({
                      {"key1", 25.0},
                      {"key2", std::string("value")},
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
