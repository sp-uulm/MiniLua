#include "MiniLua/environment.hpp"
#include <MiniLua/MiniLua.hpp>
#include <algorithm>
#include <catch2/catch.hpp>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <variant>

TEST_CASE("owning_ptr") {
    SECTION("creating a new object") {
        minilua::owning_ptr<std::string> x = minilua::make_owning<std::string>("hi"); // NOLINT
        REQUIRE(*x.get() == "hi");
        REQUIRE(*x == "hi");
    }

    SECTION("owning_ptr can be copy constructed") {
        const minilua::owning_ptr<std::string> x =
            minilua::make_owning<std::string>("hi"); // NOLINT
        const minilua::owning_ptr<std::string> y{x}; // NOLINT
        REQUIRE(x == y);
        REQUIRE(*x == *y);
    }
    SECTION("owning_ptr can be copy assigned") {
        const minilua::owning_ptr<std::string> x =
            minilua::make_owning<std::string>("hi"); // NOLINT
        minilua::owning_ptr<std::string> y;
        y = x;
        REQUIRE(x == y);
        REQUIRE(*x == *y);
    }
    SECTION("owning_ptr can be move constructed") {
        minilua::owning_ptr<std::string> x = minilua::make_owning<std::string>("hi"); // NOLINT
        const minilua::owning_ptr<std::string> y{std::move(x)};                       // NOLINT
        REQUIRE(x != y);
        REQUIRE(*x != *y);
    }
    SECTION("owning_ptr can be move assigned") {
        minilua::owning_ptr<std::string> x = minilua::make_owning<std::string>("hi"); // NOLINT
        minilua::owning_ptr<std::string> y;
        y = std::move(x);
        REQUIRE(x != y);
        REQUIRE(*x != *y);
    }
    SECTION("without default constructor") {
        struct X {
            X() = delete;
        };

        minilua::owning_ptr<X> x = minilua::make_owning<X>(X{});
        REQUIRE(x.get() != nullptr);
    }
}

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

auto fn(minilua::CallContext /*unused*/) -> minilua::CallResult { // NOLINT
    return minilua::CallResult();
}
auto fn_ref(const minilua::CallContext & /*unused*/) -> minilua::CallResult {
    return minilua::CallResult();
}

auto fn_vallist(minilua::CallContext /*unused*/) -> minilua::Vallist { // NOLINT
    return minilua::Vallist();
}
auto fn_ref_vallist(const minilua::CallContext & /*unused*/) -> minilua::Vallist {
    return minilua::Vallist();
}

auto fn_value(minilua::CallContext /*unused*/) -> minilua::Value { // NOLINT
    return minilua::Value();
}
auto fn_ref_value(const minilua::CallContext & /*unused*/) -> minilua::Value {
    return minilua::Value();
}

auto fn_string(minilua::CallContext /*unused*/) -> std::string { // NOLINT
    return std::string();
}
auto fn_ref_string(const minilua::CallContext & /*unused*/) -> std::string { return std::string(); }

void fn_void(minilua::CallContext /*unused*/) {} // NOLINT
void fn_ref_void(const minilua::CallContext& /*unused*/) {}

TEST_CASE("Lua Values") {
    SECTION("nil") {
        static_assert(std::is_nothrow_move_constructible<minilua::Nil>());
        static_assert(std::is_nothrow_move_assignable<minilua::Nil>());
        SECTION("via default constructor of Value") {
            const minilua::Value value{};
            CHECK(std::holds_alternative<minilua::Nil>(value.get()));
        }
        SECTION("via explicit construction of Nil") {
            const minilua::Value value{minilua::Nil()};
            CHECK(std::holds_alternative<minilua::Nil>(value.get()));
        }
        SECTION("nils are equal") {
            const minilua::Value value{};
            CHECK(value == minilua::Nil());
        }
    }

    SECTION("bool") {
        static_assert(std::is_nothrow_move_constructible<minilua::Bool>());
        static_assert(std::is_nothrow_move_assignable<minilua::Bool>());
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
        static_assert(std::is_nothrow_move_constructible<minilua::Number>());
        static_assert(std::is_nothrow_move_assignable<minilua::Number>());
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
        static_assert(std::is_nothrow_move_constructible<minilua::String>());
        static_assert(std::is_nothrow_move_assignable<minilua::String>());
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
        static_assert(std::is_nothrow_move_constructible<minilua::Table>());
        static_assert(std::is_nothrow_move_assignable<minilua::Table>());
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

                table.set("key2", 7.5); // NOLINT

                CHECK(table == table_copy);
            }
        }

        SECTION("small") {
            minilua::Value value{minilua::Table{{"key1", 22}}}; // NOLINT
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
        static_assert(std::is_nothrow_move_constructible<minilua::NativeFunction>());
        static_assert(std::is_nothrow_move_assignable<minilua::NativeFunction>());
        SECTION("lambda: (CallContext) -> CallResult") {
            minilua::Value value1{fn};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](minilua::CallContext /*unused*/) -> minilua::CallResult { // NOLINT
                return minilua::CallResult();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }
        SECTION("lambda: (const CallContext&) -> CallResult") {
            minilua::Value value1{fn_ref};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](const minilua::CallContext & /*unused*/) -> minilua::CallResult {
                return minilua::CallResult();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }

        SECTION("lambda: (CallContext) -> Vallist") {
            minilua::Value value1{fn_vallist};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](minilua::CallContext /*unused*/) -> minilua::Vallist { // NOLINT
                return minilua::Vallist();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }
        SECTION("lambda: (const CallContext&) -> Vallist") {
            minilua::Value value1{fn_ref_vallist};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](const minilua::CallContext & /*unused*/) -> minilua::Vallist {
                return minilua::Vallist();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }

        SECTION("lambda: (CallContext) -> Value") {
            minilua::Value value1{fn_value};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](minilua::CallContext /*unused*/) -> minilua::Value { // NOLINT
                return minilua::Value();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }
        SECTION("lambda: (const CallContext&) -> Value") {
            minilua::Value value1{fn_ref_value};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](const minilua::CallContext & /*unused*/) -> minilua::Value {
                return minilua::Value();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }

        SECTION("lambda: (CallContext) -> into Value") {
            minilua::Value value1{fn_string};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](minilua::CallContext /*unused*/) -> std::string { // NOLINT
                return std::string();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }
        SECTION("lambda: (const CallContext&) -> into Value") {
            minilua::Value value1{fn_ref_string};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](const minilua::CallContext & /*unused*/) -> std::string {
                return std::string();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }

        SECTION("lambda: (CallContext) -> void") {
            minilua::Value value1{fn_void};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](minilua::CallContext /*unused*/) { // NOLINT
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }
        SECTION("lambda: (const CallContext&) -> void") {
            minilua::Value value1{fn_ref_void};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value1.get()));

            auto lambda = [](const minilua::CallContext& /*unused*/) {};
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::NativeFunction>(value2.get()));
        }
    }
}

TEST_CASE("new Environment") {
    SECTION("from unordered_map") {
        std::unordered_map<std::string, int> map;
        map.insert_or_assign("hi", 25); // NOLINT
        std::unordered_map<std::string, int> map2{std::move(map)};
        REQUIRE_THROWS_AS(map.at("hi"), std::out_of_range);
        REQUIRE(map.empty());
        REQUIRE(map2.at("hi") == 25);
    }
    SECTION("environment can be copied") {
        static_assert(std::is_copy_constructible<minilua::Environment>());
        static_assert(std::is_copy_assignable<minilua::Environment>());

        minilua::Environment env;
        env.add("val1", 24); // NOLINT

        minilua::Environment env_copy{static_cast<const minilua::Environment&>(env)}; // NOLINT
        REQUIRE(env == env_copy);

        minilua::Environment env_copy2; // NOLINT
        env_copy2 = static_cast<const minilua::Environment&>(env);
        REQUIRE(env == env_copy2);
    }
    SECTION("environment can be moved") {
        static_assert(std::is_move_constructible<minilua::Environment>());
        static_assert(std::is_move_assignable<minilua::Environment>());

        minilua::Environment env;
        env.add("val1", 24); // NOLINT

        minilua::Environment env2{std::move(env)}; // NOLINT
        REQUIRE_THROWS_AS(env.get("val1"), std::out_of_range);
        REQUIRE(env2.get("val1") == 24);
        REQUIRE(env != env2);

        minilua::Environment env3;
        env3 = std::move(env2);
        REQUIRE_THROWS_AS(env2.get("val1"), std::out_of_range);
        REQUIRE(env3.get("val1") == 24);
        REQUIRE(env2 != env3);
    }
    SECTION("environments can be swapped") {
        static_assert(std::is_swappable<minilua::Environment>());
        minilua::Environment env;
        env.add("val1", 24); // NOLINT
        minilua::Environment env2;

        swap(env, env2);
        REQUIRE_THROWS_AS(env.get("val1"), std::out_of_range);
        REQUIRE(env2.get("val1") == 24);
        REQUIRE(env != env2);
    }
    SECTION("new environment is empty") {
        minilua::Environment env;
        REQUIRE(env.size() == 0);
    }
    SECTION("environment contains the inserted value") {
        minilua::Environment env;

        env.add("val1", 24); // NOLINT
        REQUIRE(env.size() == 1);
        REQUIRE(env.get("val1") == 24);

        std::string key = "val2";
        env.add(key, 35); // NOLINT
        REQUIRE(env.size() == 2);
        REQUIRE(env.get("val2") == 35);
    }
    SECTION("environment contains the mass inserted value") {
        minilua::Environment env;

        env.add_all({
            {"val1", 24}, // NOLINT
            {"val2", 35}, // NOLINT
        });
        REQUIRE(env.size() == 2);
        REQUIRE(env.get("val1") == 24);
        REQUIRE(env.get("val2") == 35);

        std::unordered_map<std::string, minilua::Value> map{
            {"val3", 66}, // NOLINT
            {"val4", 17}, // NOLINT
        };
        env.add_all(map);
        REQUIRE(env.size() == 4);
        REQUIRE(env.get("val3") == 66);
        REQUIRE(env.get("val4") == 17);
    }
    SECTION("setting I/O") {
        minilua::Environment env;

        std::stringstream in;
        env.set_stdin(&in);
        REQUIRE(&in == env.get_stdin());

        std::stringstream out;
        env.set_stdout(&out);
        REQUIRE(&out == env.get_stdout());

        std::stringstream err;
        env.set_stderr(&err);
        REQUIRE(&err == env.get_stderr());
    }
}

TEST_CASE("new Location") {
    minilua::Location loc1{
        .line = 5, // NOLINT
        .column = 0,
        .byte = 25 // NOLINT
    };
    CHECK(loc1 == minilua::Location{.line = 5, .column = 0, .byte = 25});
}

TEST_CASE("new Range") {
    minilua::Location loc1{
        .line = 5, // NOLINT
        .column = 0,
        .byte = 25 // NOLINT
    };
    minilua::Location loc2{
        .line = 5, // NOLINT
        .column = 7,
        .byte = 32 // NOLINT
    };
    minilua::Range range{
        .start = loc1,
        .end = loc2,
    };
    CHECK(
        range == minilua::Range{
                     .start =
                         {
                             .line = 5,
                             .column = 0,
                             .byte = 25,
                         },
                     .end = {
                         .line = 5,
                         .column = 7,
                         .byte = 32,
                     }});
}

TEST_CASE("Interpreter") {
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

    // chose source changes to apply
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

TEST_CASE("table") {
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
