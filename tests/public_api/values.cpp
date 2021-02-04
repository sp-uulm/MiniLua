#include <MiniLua/environment.hpp>
#include <MiniLua/values.hpp>
#include <algorithm>
#include <catch2/catch.hpp>
#include <iterator>
#include <string>

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

TEST_CASE("nil Value is constructable") {
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
}

TEST_CASE("nil Values are equal") {
    const minilua::Value value{};
    CHECK(value == minilua::Nil());
}

TEST_CASE("nil Value to literal") {
    const minilua::Value value{};
    CHECK(value.to_literal() == "nil");
}

TEST_CASE("bool Value is constructable") {
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
}

TEST_CASE("bool Value to literal") {
    const minilua::Value value{true};
    CHECK(value.to_literal() == "true");
    const minilua::Value value2{false};
    CHECK(value2.to_literal() == "false");
}

TEST_CASE("number Value is constructable") {
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
}

TEST_CASE("number Value to literal") {
    const minilua::Value value{2};
    CHECK(value.to_literal() == "2");
    const minilua::Value value2{-2e12};
    CHECK(value2.to_literal() == "-2000000000000");
}

TEST_CASE("string Value is constructable") {
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
}

TEST_CASE("string Value to literal") {
    const minilua::Value value{""};
    CHECK(value.to_literal() == R"("")");
    const minilua::Value value2{"string"};
    CHECK(value2.to_literal() == R"("string")");
    const minilua::Value value3{R"(string with "quotes".)"};
    CHECK(value3.to_literal() == R"("string with \"quotes\".")");
    const minilua::Value value4{"string with\nnewlines\n."};
    CHECK(value4.to_literal() == R"("string with\nnewlines\n.")");
}

TEST_CASE("table Value is constructable") {
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
            INFO(value);
            CHECK(value[5] == 22);
            CHECK(value["key1"] == 17);
            CHECK(value[true] == 12);
        }
    }
}

TEST_CASE("table Value to literal") {
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

TEST_CASE("table is iterable") {
    minilua::Table table{{1, 25}, {"hi", 17}, {17, 21}}; // NOLINT
    const std::vector<std::pair<const minilua::Value, minilua::Value>> expected{
        {1, 25}, {"hi", 17}, {17, 21}}; // NOLINT

    SECTION("increment") {
        auto iter = table.begin();
        CHECK(iter == iter);
        auto iter2 = iter;
        iter++;
        CHECK(iter2 != iter);
        CHECK(++iter2 == iter);
    }

    SECTION("derefence") {
        auto iter = table.begin();

        CHECK(std::find(expected.begin(), expected.end(), *iter) != expected.end());
        CHECK(iter->second.is_number());
    }

    SECTION("const iteration") {
        std::vector<std::pair<const minilua::Value, minilua::Value>> pairs{};
        std::copy(table.cbegin(), table.cend(), std::back_inserter(pairs));

        CHECK_THAT(pairs, Catch::Matchers::UnorderedEquals(expected));
    }

    SECTION("mutating iteration") {
        for (auto& [key, value] : table) {
            value = value + 1;
        }

        std::vector<std::pair<minilua::Value, minilua::Value>> pairs{};
        std::copy(table.begin(), table.end(), std::back_inserter(pairs));

        std::vector<std::pair<minilua::Value, minilua::Value>> expected{
            {1, 26}, {"hi", 18}, {17, 22}}; // NOLINT
        CHECK_THAT(pairs, Catch::Matchers::UnorderedEquals(expected));
    }
}

TEST_CASE("function Value is constructable") {
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
}

TEST_CASE("function Value to literal") {
    minilua::Value value{fn};
    CHECK_THROWS(value.to_literal());
}

TEST_CASE("calling a function from a function") {
    minilua::Value simple_fn{
        [](const minilua::CallContext& ctx) { return ctx.arguments().get(0); }}; // NOLINT
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    REQUIRE(simple_fn.call(ctx, 1) == minilua::CallResult({1}));

    auto lambda = [](minilua::CallContext ctx) -> minilua::CallResult { // NOLINT
        const auto& callback = ctx.arguments().get(0);

        auto x = callback.bind(ctx)({1});
        auto y = callback.call(ctx, 1);
        auto z = callback.call(ctx, 1, "unues_arg");

        return minilua::CallResult({x.values().get(0) + y.values().get(0) + z.values().get(0)});
    };
    minilua::Value value{minilua::Function(lambda)};

    auto res = value.bind(ctx)({simple_fn});
    INFO(res);
    CHECK(res == minilua::CallResult({3}));
}

TEST_CASE("addition of two Values") {
    SECTION("can add two numbers") {
        minilua::Value value1{4};
        minilua::Value value2{3};

        REQUIRE(value1 + value2 == minilua::Value(7));
        REQUIRE(value1.add(value2) == minilua::Value(7));

        auto range = minilua::Range{{0, 0, 0}, {0, 10, 10}}; // NOLINT
        REQUIRE(
            value1.add(value2, range) ==
            minilua::Value(7).with_origin(minilua::LiteralOrigin{range}));
    }
    SECTION("can't add two non numbers") {
        minilua::Value value1{"hi"};
        minilua::Value value2{minilua::Nil()};
        REQUIRE_THROWS(value1 + value2);
        REQUIRE_THROWS(value1.add(value2));
        REQUIRE_THROWS(value1.add(value2, minilua::Range()));
    }
    // TODO metatables
}

TEST_CASE("subtraction of two Values") {
    SECTION("can sub two numbers") {
        minilua::Value value1{4};
        minilua::Value value2{3};

        REQUIRE(value1 - value2 == minilua::Value(1));
        REQUIRE(value1.sub(value2) == minilua::Value(1));

        auto range = minilua::Range{{0, 0, 0}, {0, 10, 10}}; // NOLINT
        REQUIRE(
            value1.sub(value2, range) ==
            minilua::Value(1).with_origin(minilua::LiteralOrigin{range}));
    }
    SECTION("can't sub two non numbers") {
        minilua::Value value1{"hi"};
        minilua::Value value2{minilua::Nil()};
        REQUIRE_THROWS(value1 - value2);
        REQUIRE_THROWS(value1.sub(value2));
        REQUIRE_THROWS(value1.sub(value2, minilua::Range()));
    }
    // TODO metatables
}

TEST_CASE("multiplication of two Values") {
    SECTION("can multiply two numbers") {
        minilua::Value value1{4};
        minilua::Value value2{3};

        REQUIRE(value1 * value2 == minilua::Value(12));
        REQUIRE(value1.mul(value2) == minilua::Value(12));

        auto range = minilua::Range{{0, 0, 0}, {0, 10, 10}}; // NOLINT
        REQUIRE(
            value1.mul(value2, range) ==
            minilua::Value(12).with_origin(minilua::LiteralOrigin{range}));
    }
    SECTION("can't multiply two non numbers") {
        minilua::Value value1{"hi"};
        minilua::Value value2{minilua::Nil()};
        REQUIRE_THROWS(value1 * value2);
        REQUIRE_THROWS(value1.mul(value2));
        REQUIRE_THROWS(value1.mul(value2, minilua::Range()));
    }
    // TODO metatables
}

TEST_CASE("division of two Values") {
    SECTION("can divide two numbers") {
        minilua::Value value1{13}; // NOLINT
        minilua::Value value2{4};

        REQUIRE(value1 / value2 == minilua::Value(3.25));
        REQUIRE(value1.div(value2) == minilua::Value(3.25));

        auto range = minilua::Range{{0, 0, 0}, {0, 10, 10}}; // NOLINT
        REQUIRE(
            value1.div(value2, range) ==
            minilua::Value(3.25).with_origin(minilua::LiteralOrigin{range}));
    }
    SECTION("can't divide two non numbers") {
        minilua::Value value1{"hi"};
        minilua::Value value2{minilua::Nil()};
        REQUIRE_THROWS(value1 / value2);
        REQUIRE_THROWS(value1.div(value2));
        REQUIRE_THROWS(value1.div(value2, minilua::Range()));
    }
    // TODO metatables
}

TEST_CASE("power of two Values") {
    SECTION("can take power of two numbers") {
        minilua::Value value1{4};
        minilua::Value value2{3};

        REQUIRE((value1 ^ value2) == minilua::Value(64));
        REQUIRE(value1.pow(value2) == minilua::Value(64));

        auto range = minilua::Range{{0, 0, 0}, {0, 10, 10}}; // NOLINT
        REQUIRE(
            value1.pow(value2, range) ==
            minilua::Value(64).with_origin(minilua::LiteralOrigin{range}));
    }
    SECTION("can't take power of two non numbers") {
        minilua::Value value1{"hi"};
        minilua::Value value2{minilua::Nil()};
        REQUIRE_THROWS(value1 ^ value2);
        REQUIRE_THROWS(value1.pow(value2));
        REQUIRE_THROWS(value1.pow(value2, minilua::Range()));
    }
    // TODO metatables
}

TEST_CASE("modulo of two Values") {
    SECTION("can take modulo of two numbers") {
        minilua::Value value1{5.4}; // NOLINT
        minilua::Value value2{2.1}; // NOLINT

        REQUIRE(std::fmod(5.4, 2.1) == Approx(1.2));                                      // NOLINT
        REQUIRE(std::get<minilua::Number>((value1 % value2).raw()).value == Approx(1.2)); // NOLINT

        REQUIRE(std::get<minilua::Number>(value1.mod(value2).raw()).value == Approx(1.2)); // NOLINT

        auto range = minilua::Range{{0, 0, 0}, {0, 10, 10}}; // NOLINT
        auto res = value1.mod(value2, range);
        REQUIRE(std::get<minilua::Number>(res).value == Approx(1.2)); // NOLINT
        REQUIRE(std::get<minilua::BinaryOrigin>(res.origin().raw()).location == range);
    }
    SECTION("can't take modulo of two non numbers") {
        minilua::Value value1{"hi"};
        minilua::Value value2{minilua::Nil()};
        REQUIRE_THROWS(value1 % value2);
        REQUIRE_THROWS(value1.mod(value2));
        REQUIRE_THROWS(value1.mod(value2, minilua::Range()));
    }
    // TODO metatables
}

TEST_CASE("bitwise-and of two Values") {
    SECTION("can bitwise and two integers") {
        minilua::Value value1{0b11001}; // NOLINT
        minilua::Value value2{0b01100}; // NOLINT

        REQUIRE((value1 & value2) == minilua::Value(0b01000));
        REQUIRE(value1.bit_and(value2) == minilua::Value(0b01000));

        auto range = minilua::Range{{0, 0, 0}, {0, 10, 10}}; // NOLINT
        REQUIRE(
            value1.bit_and(value2, range) ==
            minilua::Value(0b01000).with_origin(minilua::LiteralOrigin{range}));
    }
    SECTION("can't bitwise and two floats") {
        minilua::Value value1{5.2}; // NOLINT
        minilua::Value value2{3.1}; // NOLINT
        REQUIRE_THROWS(value1 & value2);
        REQUIRE_THROWS(value1.bit_and(value2));
        REQUIRE_THROWS(value1.bit_and(value2, minilua::Range()));
    }
    SECTION("can't bitwise and two non numbers") {
        minilua::Value value1{"hi"};
        minilua::Value value2{minilua::Nil()};
        REQUIRE_THROWS(value1 & value2);
        REQUIRE_THROWS(value1.bit_and(value2));
        REQUIRE_THROWS(value1.bit_and(value2, minilua::Range()));
    }
    // TODO metatables?
}

TEST_CASE("bitwise-or of two Values") {
    SECTION("can bitwise or two integers") {
        minilua::Value value1{0b11001}; // NOLINT
        minilua::Value value2{0b01100}; // NOLINT

        REQUIRE((value1 | value2) == minilua::Value(0b11101));
        REQUIRE(value1.bit_or(value2) == minilua::Value(0b11101));

        auto range = minilua::Range{{0, 0, 0}, {0, 10, 10}}; // NOLINT
        REQUIRE(
            value1.bit_or(value2, range) ==
            minilua::Value(0b11101).with_origin(minilua::LiteralOrigin{range}));
    }
    SECTION("can't bitwise or two floats") {
        minilua::Value value1{5.2};
        minilua::Value value2{3.1};
        REQUIRE_THROWS(value1 | value2);
        REQUIRE_THROWS(value1.bit_or(value2));
        REQUIRE_THROWS(value1.bit_or(value2, minilua::Range()));
    }
    SECTION("can't bitwise or two non numbers") {
        minilua::Value value1{"hi"};
        minilua::Value value2{minilua::Nil()};
        REQUIRE_THROWS(value1 | value2);
        REQUIRE_THROWS(value1.bit_or(value2));
        REQUIRE_THROWS(value1.bit_or(value2, minilua::Range()));
    }
    // TODO metatables?
}

TEST_CASE("Value as bool") {
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

TEST_CASE("logic-and of two Values") {
    REQUIRE(
        (minilua::Value(minilua::Nil()) && minilua::Value(5)) == minilua::Value(minilua::Nil()));
    REQUIRE((minilua::Value(false) && minilua::Value(5)) == minilua::Value(false));
    REQUIRE((minilua::Value(3) && minilua::Value(5)) == minilua::Value(5));
    REQUIRE((minilua::Value(3) && minilua::Value(false)) == minilua::Value(false));

    auto value1 = minilua::Value(true);
    auto value2 = minilua::Value(false);
    REQUIRE((value1 && value2) == minilua::Value(false));
    REQUIRE(value1.logic_and(value2) == minilua::Value(false));

    auto range = minilua::Range{{0, 0, 0}, {0, 10, 10}}; // NOLINT
    REQUIRE(
        value1.logic_and(value2, range) ==
        minilua::Value(false).with_origin(minilua::LiteralOrigin{range}));
}

TEST_CASE("logic-or of two Values") {
    REQUIRE((minilua::Value(minilua::Nil()) || minilua::Value(5)) == minilua::Value(5));
    REQUIRE((minilua::Value(false) || minilua::Value(5)) == minilua::Value(5));
    REQUIRE((minilua::Value(3) || minilua::Value(5)) == minilua::Value(3));
    REQUIRE((minilua::Value(3) || minilua::Value(false)) == minilua::Value(3));

    auto value1 = minilua::Value(true);
    auto value2 = minilua::Value(false);
    REQUIRE((value1 || value2) == minilua::Value(true));
    REQUIRE(value1.logic_or(value2) == minilua::Value(true));

    auto range = minilua::Range{{0, 0, 0}, {0, 10, 10}}; // NOLINT
    REQUIRE(
        value1.logic_or(value2, range) ==
        minilua::Value(true).with_origin(minilua::LiteralOrigin{range}));
}

TEST_CASE("Leaking values", "[leaks]") {
    // TODO fix leaks
    SECTION("self recursive table throws an exception") {
        minilua::Value value5{minilua::Table{}};
        value5["key1"] = value5;
        CHECK_THROWS(value5.to_literal());
    }
}

TEST_CASE("construction of Vallist") {
    const minilua::Vallist vallist{1, 3, true, "hi"};
    CHECK(vallist.get(0) == 1);
    CHECK(vallist.get(1) == 3);
    CHECK(vallist.get(2) == true);
    CHECK(vallist.get(3) == "hi");
}

TEST_CASE("destructuring of Vallist") {
    const minilua::Vallist vallist{1, 3, true, "hi"};

    SECTION("same amount of values") {
        const auto& [one, three, tru, hi] = vallist.tuple<4>();
        CHECK(one == 1);
        CHECK(three == 3);
        CHECK(tru == true);
        CHECK(hi == "hi");
    }

    SECTION("fewer bindings") {
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

TEST_CASE("next(table [, index]") {
    SECTION("empty table") {
        const minilua::Table value;
        CHECK(value.next(minilua::Nil()) == minilua::Vallist());
    }
    SECTION("filled table") {
        const minilua::Table value{{"key1", 22}, {1, "Hallo "}, {2, "Welt!"}, {100, 42}};
        SECTION("access last element of table") {
            auto start = value.begin();
            std::advance(start, 3);
            CHECK(value.next((*start).first) == minilua::Vallist());
        }
        SECTION("access a non existent key") { CHECK_THROWS(value.next(minilua::Value(42))); }

        SECTION("access an element of the table") {
            auto p = std::pair<const minilua::Value, minilua::Value>{2, "Welt"};
            auto exp_erg = ++std::find_if(
                value.begin(), value.end(), [](const auto& kv) { return kv.first == 2; });
            CHECK(
                value.next(minilua::Value(2)) ==
                minilua::Vallist({exp_erg->first, exp_erg->second}));
        }
        SECTION("access the first element of the table") {
            CHECK(
                value.next(minilua::Nil()) ==
                minilua::Vallist({value.begin()->first, value.begin()->second}));
        }
    }
}

TEST_CASE("comparison operators for Strings") {
    SECTION("<") {
        SECTION("strings of equal length") {
            std::string value1 = "welt";
            std::string value2 = "wela";
            minilua::String s1(value1);
            minilua::String s2(value2);

            CHECK_FALSE(s1 < s2);
            CHECK(s2 < s1);
        }

        SECTION("strings of different length") {
            SECTION("first string is longer") {
                std::string value1 = "minilua";
                std::string value2 = "welt";
                minilua::String s1(value1);
                minilua::String s2(value2);

                CHECK(s1 < s2);
                CHECK_FALSE(s2 < s1);
            }

            SECTION("second string is longer") {
                std::string value1 = "welt";
                std::string value2 = "minilua";
                minilua::String s1(value1);
                minilua::String s2(value2);

                CHECK(s2 < s1);
                CHECK_FALSE(s1 < s2);
            }
        }
    }

    SECTION(">") {
        SECTION("strings of equal length") {
            std::string value1 = "welt";
            std::string value2 = "wela";
            minilua::String s1(value1);
            minilua::String s2(value2);

            CHECK_FALSE(s2 > s1);
            CHECK(s1 > s2);
        }

        SECTION("strings of different length") {
            SECTION("first string is longer") {
                std::string value1 = "minilua";
                std::string value2 = "welt";
                minilua::String s1(value1);
                minilua::String s2(value2);

                CHECK(s2 > s1);
                CHECK_FALSE(s1 > s2);
            }

            SECTION("second string is longer") {
                std::string value1 = "welt";
                std::string value2 = "minilua";
                minilua::String s1(value1);
                minilua::String s2(value2);

                CHECK(s1 > s2);
                CHECK_FALSE(s2 > s1);
            }
        }
    }

    SECTION("<=") {
        SECTION("strings of equal length") {
            std::string value1 = "welt";
            std::string value2 = "wela";
            minilua::String s1(value1);
            minilua::String s2(value2);

            CHECK_FALSE(s1 <= s2);
        }

        SECTION("strings of different length") {
            SECTION("first string is longer") {
                std::string value1 = "minilua";
                std::string value2 = "welt";
                minilua::String s1(value1);
                minilua::String s2(value2);

                CHECK(s1 <= s2);
            }

            SECTION("second string is longer") {
                std::string value1 = "welt";
                std::string value2 = "minilua";
                minilua::String s1(value1);
                minilua::String s2(value2);

                CHECK_FALSE(s1 <= s2);
            }
        }
    }

    SECTION(">=") {
        SECTION("strings of equal length") {
            std::string value1 = "welt";
            std::string value2 = "wela";
            minilua::String s1(value1);
            minilua::String s2(value2);

            CHECK(s1 >= s2);
        }

        SECTION("strings of different length") {
            SECTION("first string is longer") {
                std::string value1 = "minilua";
                std::string value2 = "welt";
                minilua::String s1(value1);
                minilua::String s2(value2);

                CHECK_FALSE(s1 >= s2);
            }

            SECTION("second string is longer") {
                std::string value1 = "welt";
                std::string value2 = "minilua";
                minilua::String s1(value1);
                minilua::String s2(value2);

                CHECK(s1 >= s2);
            }
        }
    }
}
