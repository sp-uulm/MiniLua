#include <MiniLua/MiniLua.hpp>
#include <algorithm>
#include <catch2/catch.hpp>
#include <functional>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <variant>

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

auto fn(minilua::CallContext) -> minilua::CallResult { return minilua::CallResult(); }
auto fn_ref(const minilua::CallContext&) -> minilua::CallResult { return minilua::CallResult(); }

auto fn_vallist(minilua::CallContext) -> minilua::Vallist { return minilua::Vallist(); }
auto fn_ref_vallist(const minilua::CallContext&) -> minilua::Vallist { return minilua::Vallist(); }

auto fn_value(minilua::CallContext) -> minilua::Value { return minilua::Value(); }
auto fn_ref_value(const minilua::CallContext&) -> minilua::Value { return minilua::Value(); }

auto fn_string(minilua::CallContext) -> std::string { return std::string(); }
auto fn_ref_string(const minilua::CallContext&) -> std::string { return std::string(); }

void fn_void(minilua::CallContext) {}
void fn_ref_void(const minilua::CallContext&) {}

TEST_CASE("Lua Values") {
    SECTION("nil") {
        SECTION("via default constructor of Value") {
            const minilua::Value value{};
            CHECK(std::holds_alternative<minilua::Nil>(value.get()));
        }
        SECTION("via explicit construction of Nil") {
            const minilua::Value value{minilua::Nil()};
            CHECK(std::holds_alternative<minilua::Nil>(value.get()));
        }
    }

    SECTION("bool") {
        SECTION("true") {
            const minilua::Value value{true};
            CHECK(std::holds_alternative<minilua::Bool>(value.get()));
            CHECK(std::get<minilua::Bool>(value.get()) == true);
            CHECK(std::get<minilua::Bool>(value.get()).value == true);
        }
        SECTION("false") {
            const minilua::Value value{false};
            CHECK(std::holds_alternative<minilua::Bool>(value.get()));
            CHECK(std::get<minilua::Bool>(value.get()) == false);
            CHECK(std::get<minilua::Bool>(value.get()).value == false);
        }
    }

    SECTION("number") {
        SECTION("2") {
            const minilua::Value value{2};
            CHECK(std::holds_alternative<minilua::Number>(value.get()));
            CHECK(std::get<minilua::Number>(value.get()) == 2);
            CHECK(std::get<minilua::Number>(value.get()).value == 2);
        }
        SECTION("-5e27") {
            const double expected_value = -5e27;
            const minilua::Value value{expected_value};
            CHECK(std::holds_alternative<minilua::Number>(value.get()));
            CHECK(std::get<minilua::Number>(value.get()) == expected_value);
            CHECK(std::get<minilua::Number>(value.get()).value == expected_value);
        }
    }

    SECTION("string") {
        SECTION("empty") {
            const minilua::Value value{""};
            CHECK(std::holds_alternative<minilua::String>(value.get()));
            CHECK(std::get<minilua::String>(value.get()) == "");
            CHECK(std::get<minilua::String>(value.get()).value == ""); // NOLINT
        }
        SECTION("small") {
            const minilua::Value value{"string"};
            CHECK(std::holds_alternative<minilua::String>(value.get()));
            CHECK(std::get<minilua::String>(value.get()) == "string");
            CHECK(std::get<minilua::String>(value.get()).value == "string");
        }
        SECTION("big") {
            const auto* const expected_value =
                "string string string string string string string string string";
            const minilua::Value value{expected_value};
            CHECK(std::holds_alternative<minilua::String>(value.get()));
            CHECK(std::get<minilua::String>(value.get()) == expected_value);
            CHECK(std::get<minilua::String>(value.get()).value == expected_value);
        }
    }

    SECTION("table") {
        SECTION("empty") {
            minilua::Value value{minilua::Table()};
            SECTION("different tables are not equal") {
                CHECK(std::holds_alternative<minilua::Table>(value.get()));
                CHECK(std::get<minilua::Table>(value.get()) != minilua::Table());
            }

            minilua::Value value_copy = value; // NOLINT
            SECTION("copies of tables are equal") {
                CHECK(std::holds_alternative<minilua::Table>(value_copy.get()));
                CHECK(
                    std::get<minilua::Table>(value_copy.get()) ==
                    std::get<minilua::Table>(value.get()));
            }

            SECTION("changes apply to all copies of a table") {
                auto& table = std::get<minilua::Table>(value.get());
                auto& table_copy = std::get<minilua::Table>(value_copy.get());

                table.set("key2", 7.5);

                CHECK(table == table_copy);
            }
        }

        SECTION("small") {
            minilua::Value value{minilua::Table{{"key1", 22}}};
            SECTION("different tables are not equal") {
                CHECK(std::holds_alternative<minilua::Table>(value.get()));
                CHECK(std::get<minilua::Table>(value.get()) != minilua::Table());
            }

            minilua::Value value_copy = value; // NOLINT
            SECTION("copies of tables are equal") {
                CHECK(std::holds_alternative<minilua::Table>(value_copy.get()));
                CHECK(
                    std::get<minilua::Table>(value_copy.get()) ==
                    std::get<minilua::Table>(value.get()));
            }

            SECTION("changes apply to all copies of a table") {
                auto& table = std::get<minilua::Table>(value.get());
                auto& table_copy = std::get<minilua::Table>(value_copy.get());

                table.set(1, "hello");

                CHECK(table == table_copy);
                CHECK(table_copy.get(1) == "hello");
            }
        }
    }

    SECTION("native function") {
        SECTION("lambda: (CallContext) -> CallResult") {
            minilua::Value value1{fn};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](minilua::CallContext) -> minilua::CallResult {
                return minilua::CallResult();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }
        SECTION("lambda: (const CallContext&) -> CallResult") {
            minilua::Value value1{fn_ref};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](const minilua::CallContext&) -> minilua::CallResult {
                return minilua::CallResult();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }

        SECTION("lambda: (CallContext) -> Vallist") {
            minilua::Value value1{fn_vallist};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](minilua::CallContext) -> minilua::Vallist {
                return minilua::Vallist();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }
        SECTION("lambda: (const CallContext&) -> Vallist") {
            minilua::Value value1{fn_ref_vallist};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](const minilua::CallContext&) -> minilua::Vallist {
                return minilua::Vallist();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }

        SECTION("lambda: (CallContext) -> Value") {
            minilua::Value value1{fn_value};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](minilua::CallContext) -> minilua::Value { return minilua::Value(); };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }
        SECTION("lambda: (const CallContext&) -> Value") {
            minilua::Value value1{fn_ref_value};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](const minilua::CallContext&) -> minilua::Value {
                return minilua::Value();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }

        SECTION("lambda: (CallContext) -> into Value") {
            minilua::Value value1{fn_string};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](minilua::CallContext) -> std::string { return std::string(); };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }
        SECTION("lambda: (const CallContext&) -> into Value") {
            minilua::Value value1{fn_ref_string};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](const minilua::CallContext&) -> std::string { return std::string(); };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }

        SECTION("lambda: (CallContext) -> void") {
            minilua::Value value1{fn_void};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](minilua::CallContext) {};
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }
        SECTION("lambda: (const CallContext&) -> void") {
            minilua::Value value1{fn_ref_void};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](const minilua::CallContext&) {};
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }
    }
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
    // FAIL();
}
