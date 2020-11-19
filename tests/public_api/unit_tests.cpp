#include <MiniLua/MiniLua.hpp>
#include <catch2/catch.hpp>

#include <sstream>

TEST_CASE("minilua::owning_ptr") {
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

// functions for use in testing NativeFunction
auto fn(minilua::CallContext /*unused*/) -> minilua::CallResult { // NOLINT
    return minilua::CallResult();
}
auto fn_ref(const minilua::CallContext& /*unused*/) -> minilua::CallResult {
    return minilua::CallResult();
}

auto fn_vallist(minilua::CallContext /*unused*/) -> minilua::Vallist { // NOLINT
    return minilua::Vallist();
}
auto fn_ref_vallist(const minilua::CallContext& /*unused*/) -> minilua::Vallist {
    return minilua::Vallist();
}

auto fn_value(minilua::CallContext /*unused*/) -> minilua::Value { // NOLINT
    return minilua::Value();
}
auto fn_ref_value(const minilua::CallContext& /*unused*/) -> minilua::Value {
    return minilua::Value();
}

auto fn_string(minilua::CallContext /*unused*/) -> std::string { // NOLINT
    return std::string();
}
auto fn_ref_string(const minilua::CallContext& /*unused*/) -> std::string { return std::string(); }

void fn_void(minilua::CallContext /*unused*/) {} // NOLINT
void fn_ref_void(const minilua::CallContext& /*unused*/) {}

TEST_CASE("minilua::Value") {
    SECTION("nil") {
        static_assert(std::is_nothrow_move_constructible<minilua::Nil>());
        static_assert(std::is_nothrow_move_assignable<minilua::Nil>());
        SECTION("via default constructor of Value") {
            const minilua::Value value{};
            CHECK(std::holds_alternative<minilua::Nil>(value.raw()));
            CHECK(value.is_nil());
        }
        SECTION("via explicit construction of Nil") {
            const minilua::Value value{minilua::Nil()};
            CHECK(std::holds_alternative<minilua::Nil>(value.raw()));
            CHECK_NOTHROW(std::get<minilua::Nil>(value));
            CHECK(value.is_nil());
        }
        SECTION("nils are equal") {
            const minilua::Value value{};
            CHECK(value == minilua::Nil());
        }
        SECTION("nil to literal") {
            const minilua::Value value{};
            CHECK(value.to_literal() == "nil");
        }
    }

    SECTION("bool") {
        static_assert(std::is_nothrow_move_constructible<minilua::Bool>());
        static_assert(std::is_nothrow_move_assignable<minilua::Bool>());
        SECTION("true") {
            const minilua::Value value{true};
            CHECK(std::holds_alternative<minilua::Bool>(value.raw()));
            CHECK(std::get<minilua::Bool>(value.raw()) == true);
            CHECK(std::get<minilua::Bool>(value.raw()).value == true);
            CHECK(std::get<minilua::Bool>(value) == true);
            CHECK(value.is_bool());
        }
        SECTION("false") {
            const minilua::Value value{false};
            CHECK(std::holds_alternative<minilua::Bool>(value.raw()));
            CHECK(std::get<minilua::Bool>(value.raw()) == false);
            CHECK(std::get<minilua::Bool>(value.raw()).value == false);
            CHECK(std::get<minilua::Bool>(value) == false);
            CHECK(value.is_bool());
        }
        SECTION("bool to literal") {
            const minilua::Value value{true};
            CHECK(value.to_literal() == "true");
            const minilua::Value value2{false};
            CHECK(value2.to_literal() == "false");
        }
    }

    SECTION("number") {
        static_assert(std::is_nothrow_move_constructible<minilua::Number>());
        static_assert(std::is_nothrow_move_assignable<minilua::Number>());
        SECTION("2") {
            const minilua::Value value{2};
            CHECK(std::holds_alternative<minilua::Number>(value.raw()));
            CHECK(std::get<minilua::Number>(value.raw()) == 2);
            CHECK(std::get<minilua::Number>(value.raw()).value == 2);
            CHECK(std::get<minilua::Number>(value) == 2);
            CHECK(value.is_number());
        }
        SECTION("-2e12") {
            const double expected_value = -2e12;
            const minilua::Value value{expected_value};
            CHECK(std::holds_alternative<minilua::Number>(value.raw()));
            CHECK(std::get<minilua::Number>(value.raw()) == expected_value);
            CHECK(std::get<minilua::Number>(value.raw()).value == expected_value);
            CHECK(std::get<minilua::Number>(value) == expected_value);
            CHECK(value.is_number());
        }
        SECTION("number to literal") {
            const minilua::Value value{2};
            CHECK(value.to_literal() == "2");
            const minilua::Value value2{-2e12};
            CHECK(value2.to_literal() == "-2000000000000");
        }
    }

    SECTION("string") {
        static_assert(std::is_nothrow_move_constructible<minilua::String>());
        static_assert(std::is_nothrow_move_assignable<minilua::String>());
        SECTION("empty") {
            const minilua::Value value{""};
            CHECK(std::holds_alternative<minilua::String>(value.raw()));
            CHECK(std::get<minilua::String>(value.raw()) == "");
            CHECK(std::get<minilua::String>(value.raw()).value == ""); // NOLINT
            CHECK(std::get<minilua::String>(value) == "");
            CHECK(value.is_string());
        }
        SECTION("small") {
            const minilua::Value value{"string"};
            CHECK(std::holds_alternative<minilua::String>(value.raw()));
            CHECK(std::get<minilua::String>(value.raw()) == "string");
            CHECK(std::get<minilua::String>(value.raw()).value == "string");
            CHECK(std::get<minilua::String>(value) == "string");
            CHECK(value.is_string());
        }
        SECTION("big") {
            const auto* const expected_value =
                "string string string string string string string string string";
            const minilua::Value value{expected_value};
            CHECK(std::holds_alternative<minilua::String>(value.raw()));
            CHECK(std::get<minilua::String>(value.raw()) == expected_value);
            CHECK(std::get<minilua::String>(value.raw()).value == expected_value);
            CHECK(std::get<minilua::String>(value) == expected_value);
            CHECK(value.is_string());
        }
        SECTION("number to literal") {
            const minilua::Value value{""};
            CHECK(value.to_literal() == R"("")");
            const minilua::Value value2{"string"};
            CHECK(value2.to_literal() == R"("string")");
            const minilua::Value value3{R"(string with "quotes".)"};
            CHECK(value3.to_literal() == R"("string with \"quotes\".")");
            const minilua::Value value4{"string with\nnewlines\n."};
            CHECK(value4.to_literal() == R"("string with\nnewlines\n.")");
        }
    }

    SECTION("table") {
        static_assert(std::is_nothrow_move_constructible<minilua::Table>());
        static_assert(std::is_nothrow_move_assignable<minilua::Table>());
        SECTION("empty") {
            minilua::Value value{minilua::Table()};
            CHECK(value.is_table());
            SECTION("different tables are not equal") {
                CHECK(std::holds_alternative<minilua::Table>(value.raw()));
                CHECK(std::get<minilua::Table>(value.raw()) != minilua::Table());
                CHECK(std::get<minilua::Table>(value) != minilua::Table());
            }

            minilua::Value value_copy = value; // NOLINT
            CHECK(value_copy.is_table());
            SECTION("copies of tables are equal") {
                CHECK(std::holds_alternative<minilua::Table>(value_copy.raw()));
                CHECK(
                    std::get<minilua::Table>(value_copy.raw()) ==
                    std::get<minilua::Table>(value.raw()));
                CHECK(std::get<minilua::Table>(value_copy) == std::get<minilua::Table>(value));
            }

            SECTION("changes apply to all copies of a table") {
                auto& table = std::get<minilua::Table>(value.raw());
                auto& table_copy = std::get<minilua::Table>(value_copy.raw());

                table.set("key2", 7.5); // NOLINT

                CHECK(table == table_copy);
            }
        }

        SECTION("small") {
            minilua::Value value{minilua::Table{{"key1", 22}}}; // NOLINT
            CHECK(value.is_table());
            SECTION("different tables are not equal") {
                CHECK(std::holds_alternative<minilua::Table>(value.raw()));
                CHECK(std::get<minilua::Table>(value.raw()) != minilua::Table());
            }

            minilua::Value value_copy = value; // NOLINT
            CHECK(value_copy.is_table());
            SECTION("copies of tables are equal") {
                CHECK(std::holds_alternative<minilua::Table>(value_copy.raw()));
                CHECK(
                    std::get<minilua::Table>(value_copy.raw()) ==
                    std::get<minilua::Table>(value.raw()));
            }

            SECTION("changes apply to all copies of a table") {
                auto& table = std::get<minilua::Table>(value.raw());
                auto& table_copy = std::get<minilua::Table>(value_copy.raw());

                table.set(1, "hello");

                CHECK(table == table_copy);
                CHECK(table_copy.get(1) == "hello");
            }

            SECTION("contains initial values") {
                minilua::Value value{minilua::Table{{5, 22}, {"key1", 17}, {true, 12}}}; // NOLINT
                CHECK(value[5] == 22);
                CHECK(value["key1"] == 17);
                CHECK(value[true] == 12);
            }
        }
        SECTION("table to literal") {
            minilua::Value value{minilua::Table()};
            CHECK(value.to_literal() == R"({})");
            minilua::Value value2{minilua::Table{{"key1", 22}}}; // NOLINT
            CHECK(value2.to_literal() == R"({ key1 = 22 })");
            minilua::Value value3{minilua::Table{{5, 22}}}; // NOLINT
            CHECK(value3.to_literal() == R"({ [5] = 22 })");
            // TODO check by parsing the resulting literal
            // minilua::Value value4{minilua::Table{{5, 22}, {"key1", 17}, {true, 12.5}}}; // NOLINT
            // CHECK(value4.to_literal() == R"({ key1 = 17, [true] = 12.5, [5] = 22 })");
        }
    }

    SECTION("native function") {
        static_assert(std::is_nothrow_move_constructible<minilua::Function>());
        static_assert(std::is_nothrow_move_assignable<minilua::Function>());
        SECTION("lambda: (CallContext) -> CallResult") {
            minilua::Value value1{fn};
            CHECK(std::holds_alternative<minilua::Function>(value1.raw()));
            CHECK(value1.is_function());

            auto lambda = [](minilua::CallContext /*unused*/) -> minilua::CallResult { // NOLINT
                return minilua::CallResult();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::Function>(value2.raw()));
            CHECK(value2.is_function());
        }
        SECTION("lambda: (const CallContext&) -> CallResult") {
            minilua::Value value1{fn_ref};
            CHECK(std::holds_alternative<minilua::Function>(value1.raw()));
            CHECK(value1.is_function());

            auto lambda = [](const minilua::CallContext& /*unused*/) -> minilua::CallResult {
                return minilua::CallResult();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::Function>(value2.raw()));
            CHECK(value2.is_function());
        }

        SECTION("lambda: (CallContext) -> Vallist") {
            minilua::Value value1{fn_vallist};
            CHECK(std::holds_alternative<minilua::Function>(value1.raw()));
            CHECK(value1.is_function());

            auto lambda = [](minilua::CallContext /*unused*/) -> minilua::Vallist { // NOLINT
                return minilua::Vallist();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::Function>(value2.raw()));
            CHECK(value2.is_function());
        }
        SECTION("lambda: (const CallContext&) -> Vallist") {
            minilua::Value value1{fn_ref_vallist};
            CHECK(std::holds_alternative<minilua::Function>(value1.raw()));
            CHECK(value1.is_function());

            auto lambda = [](const minilua::CallContext& /*unused*/) -> minilua::Vallist {
                return minilua::Vallist();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::Function>(value2.raw()));
            CHECK(value2.is_function());
        }

        SECTION("lambda: (CallContext) -> Value") {
            minilua::Value value1{fn_value};
            CHECK(std::holds_alternative<minilua::Function>(value1.raw()));
            CHECK(value1.is_function());

            auto lambda = [](minilua::CallContext /*unused*/) -> minilua::Value { // NOLINT
                return minilua::Value();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::Function>(value2.raw()));
            CHECK(value2.is_function());
        }
        SECTION("lambda: (const CallContext&) -> Value") {
            minilua::Value value1{fn_ref_value};
            CHECK(std::holds_alternative<minilua::Function>(value1.raw()));
            CHECK(value1.is_function());

            auto lambda = [](const minilua::CallContext& /*unused*/) -> minilua::Value {
                return minilua::Value();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::Function>(value2.raw()));
            CHECK(value2.is_function());
        }

        SECTION("lambda: (CallContext) -> into Value") {
            minilua::Value value1{fn_string};
            CHECK(std::holds_alternative<minilua::Function>(value1.raw()));
            CHECK(value1.is_function());

            auto lambda = [](minilua::CallContext /*unused*/) -> std::string { // NOLINT
                return std::string();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::Function>(value2.raw()));
            CHECK(value2.is_function());
        }
        SECTION("lambda: (const CallContext&) -> into Value") {
            minilua::Value value1{fn_ref_string};
            CHECK(std::holds_alternative<minilua::Function>(value1.raw()));
            CHECK(value1.is_function());

            auto lambda = [](const minilua::CallContext& /*unused*/) -> std::string {
                return std::string();
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::Function>(value2.raw()));
            CHECK(value2.is_function());
        }

        SECTION("lambda: (CallContext) -> void") {
            minilua::Value value1{fn_void};
            CHECK(std::holds_alternative<minilua::Function>(value1.raw()));
            CHECK(value1.is_function());

            auto lambda = [](minilua::CallContext /*unused*/) { // NOLINT
            };
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::Function>(value2.raw()));
            CHECK(value2.is_function());
        }
        SECTION("lambda: (const CallContext&) -> void") {
            minilua::Value value1{fn_ref_void};
            CHECK(std::holds_alternative<minilua::Function>(value1.raw()));
            CHECK(value1.is_function());

            auto lambda = [](const minilua::CallContext& /*unused*/) {};
            minilua::Value value2{lambda};
            CHECK(std::holds_alternative<minilua::Function>(value2.raw()));
            CHECK(value2.is_function());
        }
        SECTION("function to literal") {
            minilua::Value value{fn};
            CHECK_THROWS(value.to_literal());
        }
    }

    SECTION("addition") {
        SECTION("can add two numbers") {
            minilua::Value value1{4};
            minilua::Value value2{3};
            REQUIRE(value1 + value2 == minilua::Value(7));
        }
        SECTION("can't add two non numbers") {
            minilua::Value value1{"hi"};
            minilua::Value value2{minilua::Nil()};
            REQUIRE_THROWS(value1 + value2);
        }
        // TODO metatables
    }
    SECTION("subtraction") {
        SECTION("can sub two numbers") {
            minilua::Value value1{4};
            minilua::Value value2{3};
            REQUIRE(value1 - value2 == minilua::Value(1));
        }
        SECTION("can't sub two non numbers") {
            minilua::Value value1{"hi"};
            minilua::Value value2{minilua::Nil()};
            REQUIRE_THROWS(value1 - value2);
        }
        // TODO metatables
    }
    SECTION("multiplication") {
        SECTION("can multiply two numbers") {
            minilua::Value value1{4};
            minilua::Value value2{3};
            REQUIRE(value1 * value2 == minilua::Value(12));
        }
        SECTION("can't multiply two non numbers") {
            minilua::Value value1{"hi"};
            minilua::Value value2{minilua::Nil()};
            REQUIRE_THROWS(value1 * value2);
        }
        // TODO metatables
    }
    SECTION("division") {
        SECTION("can divide two numbers") {
            minilua::Value value1{13};
            minilua::Value value2{4};
            REQUIRE(value1 / value2 == minilua::Value(3.25));
        }
        SECTION("can't divide two non numbers") {
            minilua::Value value1{"hi"};
            minilua::Value value2{minilua::Nil()};
            REQUIRE_THROWS(value1 / value2);
        }
        // TODO metatables
    }
    SECTION("power") {
        SECTION("can take power of two numbers") {
            minilua::Value value1{4};
            minilua::Value value2{3};
            REQUIRE((value1 ^ value2) == minilua::Value(64));
        }
        SECTION("can't take power of two non numbers") {
            minilua::Value value1{"hi"};
            minilua::Value value2{minilua::Nil()};
            REQUIRE_THROWS(value1 ^ value2);
        }
        // TODO metatables
    }
    SECTION("modulo") {
        SECTION("can take modulo of two numbers") {
            minilua::Value value1{5.4};
            minilua::Value value2{2.1};
            REQUIRE(std::fmod(5.4, 2.1) == Approx(1.2));
            REQUIRE(std::get<minilua::Number>((value1 % value2).raw()).value == Approx(1.2));
        }
        SECTION("can't take modulo of two non numbers") {
            minilua::Value value1{"hi"};
            minilua::Value value2{minilua::Nil()};
            REQUIRE_THROWS(value1 % value2);
        }
        // TODO metatables
    }
    SECTION("bitwise and") {
        SECTION("can bitwise and two integers") {
            minilua::Value value1{0b11001};
            minilua::Value value2{0b01100};
            REQUIRE((value1 & value2) == minilua::Value(0b01000));
        }
        SECTION("can't bitwise and two floats") {
            minilua::Value value1{5.2};
            minilua::Value value2{3.1};
            REQUIRE_THROWS(value1 & value2);
        }
        SECTION("can't bitwise and two non numbers") {
            minilua::Value value1{"hi"};
            minilua::Value value2{minilua::Nil()};
            REQUIRE_THROWS(value1 & value2);
        }
        // TODO metatables?
    }
    SECTION("bitwise or") {
        SECTION("can bitwise or two integers") {
            minilua::Value value1{0b11001};
            minilua::Value value2{0b01100};
            REQUIRE((value1 | value2) == minilua::Value(0b11101));
        }
        SECTION("can't bitwise or two floats") {
            minilua::Value value1{5.2};
            minilua::Value value2{3.1};
            REQUIRE_THROWS(value1 | value2);
        }
        SECTION("can't bitwise or two non numbers") {
            minilua::Value value1{"hi"};
            minilua::Value value2{minilua::Nil()};
            REQUIRE_THROWS(value1 | value2);
        }
        // TODO metatables?
    }
    SECTION("as bool") {
        SECTION("false and nil are falsey") {
            REQUIRE(bool(minilua::Value(false)) == false);
            REQUIRE(bool(minilua::Value(minilua::Nil())) == false);
        }
        SECTION("everything else is truthy") {
            REQUIRE(bool(minilua::Value(0)) == true);
            REQUIRE(bool(minilua::Value(4)) == true);
            REQUIRE(bool(minilua::Value(20.5)) == true);
            REQUIRE(bool(minilua::Value("hi")) == true);
            REQUIRE(bool(minilua::Value(minilua::Table())) == true);
        }
    }
    SECTION("logic and") {
        REQUIRE(
            (minilua::Value(minilua::Nil()) && minilua::Value(5)) ==
            minilua::Value(minilua::Nil()));
        REQUIRE((minilua::Value(false) && minilua::Value(5)) == minilua::Value(false));
        REQUIRE((minilua::Value(3) && minilua::Value(5)) == minilua::Value(5));
        REQUIRE((minilua::Value(3) && minilua::Value(false)) == minilua::Value(false));
    }
    SECTION("logic or") {
        REQUIRE((minilua::Value(minilua::Nil()) || minilua::Value(5)) == minilua::Value(5));
        REQUIRE((minilua::Value(false) || minilua::Value(5)) == minilua::Value(5));
        REQUIRE((minilua::Value(3) || minilua::Value(5)) == minilua::Value(3));
        REQUIRE((minilua::Value(3) || minilua::Value(false)) == minilua::Value(3));
    }
}

TEST_CASE("Leaking values", "[leaks]") {
    // TODO fix leaks
    SECTION("self recursive table throws an exception") {
        minilua::Value value5{minilua::Table{}};
        value5["key1"] = value5;
        CHECK_THROWS(value5.to_literal());
    }
}

TEST_CASE("Vallist") {
    SECTION("construction") {
        const minilua::Vallist vallist{1, 3, true, "hi"};
        CHECK(vallist.get(0) == 1);
        CHECK(vallist.get(1) == 3);
        CHECK(vallist.get(2) == true);
        CHECK(vallist.get(3) == "hi");
    }
    SECTION("destructuring") {
        const minilua::Vallist vallist{1, 3, true, "hi"};
        SECTION("exact amount") {
            const auto& [one, three, tru, hi] = vallist.tuple<4>();
            CHECK(one == 1);
            CHECK(three == 3);
            CHECK(tru == true);
            CHECK(hi == "hi");
        }
        SECTION("less bindings") {
            const auto& [one, three, tru] = vallist.tuple<3>();
            CHECK(one == 1);
            CHECK(three == 3);
            CHECK(tru == true);
        }
        SECTION("more bindings") {
            const auto& [one, three, tru, hi, nil1, nil2] = vallist.tuple<6>(); // NOLINT
            CHECK(one == 1);
            CHECK(three == 3);
            CHECK(tru == true);
            CHECK(hi == "hi");
            CHECK(nil1 == minilua::Nil());
            CHECK(nil2 == minilua::Nil());
        }
    }
}

TEST_CASE("minilua::Environment") {
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

TEST_CASE("minilua::Location") {
    minilua::Location loc1{
        .line = 5, // NOLINT
        .column = 0,
        .byte = 25 // NOLINT
    };
    CHECK(loc1 == minilua::Location{.line = 5, .column = 0, .byte = 25});
}

TEST_CASE("minilua::Range") {
    minilua::Location loc1{
        .line = 5, // NOLINT
        .column = 0,
        .byte = 25 // NOLINT
    };
    minilua::Location loc2{
        .line = 5,   // NOLINT
        .column = 7, // NOLINT
        .byte = 32   // NOLINT
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

TEST_CASE("minilua::SourceChange") {
    auto change = minilua::SCSingle(minilua::Range{{0, 0, 0}, {0, 5, 5}}, "replacement"); // NOLINT
    change.hint = "hint";
    change.origin = "origin";
    INFO(change);
    REQUIRE(change == change);
    minilua::SourceChange source_change{change};

    INFO(source_change);
    REQUIRE(source_change.origin() == "origin");
    REQUIRE(source_change.hint() == "hint");
    REQUIRE(source_change == source_change);

    auto change2 = minilua::SCSingle(minilua::Range{{0, 0, 0}, {0, 5, 5}}, "replacement"); // NOLINT
    change2.hint = "hint";
    change2.origin = "origin";
    INFO(change2);
    minilua::SourceChange source_change2{change2};
    REQUIRE(source_change == source_change2);

    auto combined_change = minilua::SCAnd({source_change, source_change2});
    INFO(combined_change);
    minilua::SourceChange source_change3{combined_change};
    INFO(source_change3);
}
