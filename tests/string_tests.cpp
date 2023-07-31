#include <catch2/catch.hpp>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <list>
#include <random>
#include <string>
#include <vector>

#include "MiniLua/environment.hpp"
#include "MiniLua/source_change.hpp"
#include "MiniLua/string.hpp"
#include "MiniLua/values.hpp"

using Catch::Matchers::Contains;

TEST_CASE("string.byte") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("String, Nil, Nil") {
        auto testFunction = [&ctx](auto str, auto expected_result) {
            ctx = ctx.make_new({str});
            auto result_list = minilua::string::byte(ctx);
            CHECK(result_list.size() == 1);
            CHECK(result_list.get(0) == minilua::Value(expected_result));
        };
        std::string s = "a";
        testFunction(s, 97);

        s = "Hallo";
        testFunction(s, 72);

        s = "\n";
        testFunction(s, 10);

        s = "";
        testFunction(s, minilua::Nil());
    }

    SECTION("Number, Nil, Nil") {
        auto testFunction = [&ctx](auto str, auto expected_result) {
            ctx = ctx.make_new({str});
            auto result_list = minilua::string::byte(ctx);
            CHECK(result_list.size() == 1);
            CHECK(result_list.get(0) == minilua::Value(expected_result));
        };
        int s = 1;
        testFunction(s, 49);

        s = 10;
        testFunction(s, 49);

        double new_s = 1.5;
        testFunction(new_s, 49);
    }

    SECTION("String, Number, Nil") {
        auto testFunction = [&ctx](auto str, auto i, auto expected_result, int expected_size = 1) {
            ctx = ctx.make_new({str, i});
            auto result_list = minilua::string::byte(ctx);
            CHECK(result_list.get(0) == minilua::Value(expected_result));
            CHECK(result_list.size() == expected_size);
        };

        std::string s = "Hallo";
        int i = 3;
        testFunction(s, i, 108);

        i = 7;
        testFunction(s, i, minilua::Nil(), 0);

        i = -4;
        testFunction(s, i, 97);
    }

    SECTION("Number, Number, Nil") {
        auto testFunction = [&ctx](auto str, auto i, auto expected_result, int expected_size = 1) {
            ctx = ctx.make_new({str, i});
            auto result_list = minilua::string::byte(ctx);
            CHECK(result_list.size() == expected_size);
            CHECK(result_list.get(0) == minilua::Value(expected_result));
        };

        int s = 123456;
        int i = 3;
        testFunction(s, i, 51);

        i = 0;
        testFunction(s, i, minilua::Nil(), 0);

        i = -3;
        testFunction(s, i, 52);
    }

    SECTION("String, String, Nil") {
        auto testFunction = [&ctx](auto str, auto i, auto expected_result) {
            ctx = ctx.make_new({str, i});
            auto result_list = minilua::string::byte(ctx);
            CHECK(result_list.size() == 1);
            CHECK(result_list.get(0) == minilua::Value(expected_result));
        };

        std::string s = "Hallo";
        std::string i = "3";
        testFunction(s, i, 108);
    }

    SECTION("String, Nil, Number") {
        auto testFunction = [&ctx](
                                auto str, auto j, int num_expected_results,
                                std::initializer_list<minilua::Value> expected_results) {
            ctx = ctx.make_new({str, minilua::Nil(), j});
            auto result_list = minilua::string::byte(ctx);
            CHECK(result_list.size() == num_expected_results);
            int idx = 0;
            for (const auto& result : expected_results) {
                CHECK(result_list.get(idx) == result);
                ++idx;
            }
        };

        std::string s = "Hallo";
        int j = 3;
        testFunction(s, j, 3, {72, 97, 108});
    }

    SECTION("Number, Nil, Number") {
        auto testFunction = [&ctx](
                                auto str, auto j, int num_expected_results,
                                std::initializer_list<minilua::Value> expected_results) {
            ctx = ctx.make_new({str, minilua::Nil(), j});
            auto result_list = minilua::string::byte(ctx);
            CHECK(result_list.size() == num_expected_results);
            int idx = 0;
            for (const auto& result : expected_results) {
                CHECK(result_list.get(idx) == result);
                ++idx;
            }
        };

        int s = 123456;
        int j = 3;
        testFunction(s, j, 3, {49, 50, 51});

        j = -3;
        testFunction(s, j, 4, {49, 50, 51, 52});
    }

    SECTION("String, Number, Number") {
        auto testFunction = [&ctx](
                                auto str, auto i, auto j, int num_expected_results,
                                std::initializer_list<minilua::Value> expected_results) {
            ctx = ctx.make_new({str, i, j});
            auto result_list = minilua::string::byte(ctx);
            CHECK(result_list.size() == num_expected_results);
            int idx = 0;
            for (const auto& result : expected_results) {
                CHECK(result_list.get(idx) == result);
                ++idx;
            }
        };

        std::string s = "hallo";
        int i = 2;
        int j = 4;
        testFunction(s, i, j, 3, {97, 108, 108});

        j = -2;
        testFunction(s, i, j, 3, {97, 108, 108});

        i = -4;
        j = 4;
        testFunction(s, i, j, 3, {97, 108, 108});

        i = -4;
        j = -2;
        testFunction(s, i, j, 3, {97, 108, 108});

        i = 2;
        j = 8;
        testFunction(s, i, j, 4, {97, 108, 108, 111});

        i = 6;
        j = 8;
        testFunction(s, i, j, 0, {minilua::Nil()});

        testFunction(s, 2.0, 8.0, 4, {97, 108, 108, 111});
    }

    SECTION("String, String, Number") {
        auto testFunction = [&ctx](
                                auto str, auto i, auto j, int num_expected_results,
                                std::initializer_list<minilua::Value> expected_results) {
            ctx = ctx.make_new({str, i, j});
            auto resultList = minilua::string::byte(ctx);
            CHECK(resultList.size() == num_expected_results);
            int idx = 0;
            for (const auto& result : expected_results) {
                CHECK(resultList.get(idx) == result);
                ++idx;
            }
        };

        std::string s = "hallo";
        std::string i = "2";
        int j = 4;
        testFunction(s, i, j, 3, {97, 108, 108});

        j = -2;
        testFunction(s, i, j, 3, {97, 108, 108});

        i = "-4";
        j = 4;
        testFunction(s, i, j, 3, {97, 108, 108});

        i = "-4";
        j = -2;
        testFunction(s, i, j, 3, {97, 108, 108});

        i = "2";
        j = 8;
        testFunction(s, i, j, 4, {97, 108, 108, 111});

        i = "6";
        j = 8;
        testFunction(s, i, j, 0, {minilua::Nil()});
    }

    SECTION("String, Number, String") {
        auto testFunction = [&ctx](
                                auto str, auto i, auto j, int num_expected_results,
                                std::initializer_list<minilua::Value> expected_results) {
            ctx = ctx.make_new({str, i, j});
            auto resultList = minilua::string::byte(ctx);
            CHECK(resultList.size() == num_expected_results);
            int idx = 0;
            for (const auto& result : expected_results) {
                CHECK(resultList.get(idx) == result);
                ++idx;
            }
        };

        std::string s = "hallo";
        int i = 2;
        std::string j = "4";
        testFunction(s, i, j, 3, {97, 108, 108});

        j = "-2";
        testFunction(s, i, j, 3, {97, 108, 108});

        i = -4;
        j = "4";
        testFunction(s, i, j, 3, {97, 108, 108});

        i = -4;
        j = "-2";
        testFunction(s, i, j, 3, {97, 108, 108});

        i = 2;
        j = "8";
        testFunction(s, i, j, 4, {97, 108, 108, 111});

        i = 6;
        j = "8";
        testFunction(s, i, j, 0, {minilua::Nil()});
    }

    SECTION("String, String, String") {
        auto testFunction = [&ctx](
                                auto str, auto i, auto j, int num_expected_results,
                                std::initializer_list<minilua::Value> expected_results) {
            ctx = ctx.make_new({str, i, j});
            auto resultList = minilua::string::byte(ctx);
            CHECK(resultList.size() == num_expected_results);
            int idx = 0;
            for (const auto& result : expected_results) {
                CHECK(resultList.get(idx) == result);
                ++idx;
            }
        };

        std::string s = "hallo";
        std::string i = "2";
        std::string j = "4";
        testFunction(s, i, j, 3, {97, 108, 108});

        j = "-2";
        testFunction(s, i, j, 3, {97, 108, 108});

        i = "-4";
        j = "4";
        testFunction(s, i, j, 3, {97, 108, 108});

        i = "-4";
        j = "-2";
        testFunction(s, i, j, 3, {97, 108, 108});

        i = "2";
        j = "8";
        testFunction(s, i, j, 4, {97, 108, 108, 111});

        i = "6";
        j = "8";
        testFunction(s, i, j, 0, {minilua::Nil()});
    }

    SECTION("Number, Number, Number") {
        auto testFunction = [&ctx](
                                auto str, auto i, auto j, int num_expected_results,
                                std::initializer_list<minilua::Value> expected_results) {
            ctx = ctx.make_new({str, i, j});
            auto resultList = minilua::string::byte(ctx);
            CHECK(resultList.size() == num_expected_results);
            int idx = 0;
            for (const auto& result : expected_results) {
                CHECK(resultList.get(idx) == result);
                ++idx;
            }
        };

        int s = 12345;
        int i = 2;
        int j = 4;
        testFunction(s, i, j, 3, {50, 51, 52});

        j = -2;
        testFunction(s, i, j, 3, {50, 51, 52});

        i = -4;
        j = 4;
        testFunction(s, i, j, 3, {50, 51, 52});

        i = -4;
        j = -2;
        testFunction(s, i, j, 3, {50, 51, 52});

        i = 2;
        j = 8;
        testFunction(s, i, j, 4, {50, 51, 52, 53});

        i = 6;
        j = 8;
        testFunction(s, i, j, 0, {minilua::Nil()});
    }

    SECTION("Number, String, Number") {
        auto testFunction = [&ctx](
                                auto str, auto i, auto j, int num_expected_results,
                                std::initializer_list<minilua::Value> expected_results) {
            ctx = ctx.make_new({str, i, j});
            auto resultList = minilua::string::byte(ctx);
            CHECK(resultList.size() == num_expected_results);
            int idx = 0;
            for (const auto& result : expected_results) {
                CHECK(resultList.get(idx) == result);
                ++idx;
            }
        };

        int s = 12345;
        std::string i = "2";
        int j = 4;
        testFunction(s, i, j, 3, {50, 51, 52});

        j = -2;
        testFunction(s, i, j, 3, {50, 51, 52});

        i = "-4";
        j = 4;
        testFunction(s, i, j, 3, {50, 51, 52});

        i = "-4";
        j = -2;
        testFunction(s, i, j, 3, {50, 51, 52});

        i = "2";
        j = 8;
        testFunction(s, i, j, 4, {50, 51, 52, 53});

        i = "6";
        j = 8;
        testFunction(s, i, j, 0, {minilua::Nil()});
    }

    SECTION("Number, Number, String") {
        auto testFunction = [&ctx](
                                auto str, auto i, auto j, int num_expected_results,
                                std::initializer_list<minilua::Value> expected_results) {
            ctx = ctx.make_new({str, i, j});
            auto resultList = minilua::string::byte(ctx);
            CHECK(resultList.size() == num_expected_results);
            int idx = 0;
            for (const auto& result : expected_results) {
                CHECK(resultList.get(idx) == result);
                ++idx;
            }
        };

        int s = 12345;
        int i = 2;
        std::string j = "4";
        testFunction(s, i, j, 3, {50, 51, 52});

        j = "-2";
        testFunction(s, i, j, 3, {50, 51, 52});

        i = -4;
        j = "4";
        testFunction(s, i, j, 3, {50, 51, 52});

        i = -4;
        j = "-2";
        testFunction(s, i, j, 3, {50, 51, 52});

        i = 2;
        j = "8";
        testFunction(s, i, j, 4, {50, 51, 52, 53});

        i = 6;
        j = "8";
        testFunction(s, i, j, 0, {minilua::Nil()});
    }

    SECTION("Number, String, String") {
        auto testFunction = [&ctx](
                                auto str, auto i, auto j, int num_expected_results,
                                std::initializer_list<minilua::Value> expected_results) {
            ctx = ctx.make_new({str, i, j});
            auto resultList = minilua::string::byte(ctx);
            CHECK(resultList.size() == num_expected_results);
            int idx = 0;
            for (const auto& result : expected_results) {
                CHECK(resultList.get(idx) == result);
                ++idx;
            }
        };

        int s = 12345;
        std::string i = "2";
        std::string j = "4";
        testFunction(s, i, j, 3, {50, 51, 52});

        j = "-2";
        testFunction(s, i, j, 3, {50, 51, 52});

        i = "-4";
        j = "4";
        testFunction(s, i, j, 3, {50, 51, 52});

        i = "-4";
        j = "-2";
        testFunction(s, i, j, 3, {50, 51, 52});

        i = "2";
        j = "8";
        testFunction(s, i, j, 4, {50, 51, 52, 53});

        i = "6";
        j = "8";
        testFunction(s, i, j, 0, {minilua::Nil()});
    }

    SECTION("Invalid Input") {
        auto testFunction = [&ctx](
                                auto str, auto i, auto j,
                                const std::string& expected_error_message_part_1,
                                const std::string& expected_error_message_part_2) {
            ctx = ctx.make_new({str, i, j});
            CHECK_THROWS_WITH(
                minilua::string::byte(ctx),
                Contains(expected_error_message_part_1) && Contains(expected_error_message_part_2));
        };
        SECTION("invalid s") {
            bool s = false;
            int i = 1;
            int j = 3;
            testFunction(s, i, j, "bad argument #1", "string expected");

            minilua::Table table = {};
            testFunction(table, i, j, "bad argument #1", "string expected");

            testFunction(
                minilua::Nil(), minilua::Nil(), minilua::Nil(), "bad argument #1",
                "string expected");
        }

        SECTION("invalid i") {
            SECTION("invalid type") {
                std::string s = "Hallo Welt!";
                std::string i = "zwei";
                int j = 6;

                testFunction(s, i, minilua::Nil(), "bad argument #2", "number expected");

                testFunction(s, true, j, "bad argument #2", "number expected");
            }

            SECTION("i is float") {
                std::string s = "Hallo Welt!";

                testFunction(s, 3.5, 6, "bad argument #2", "number has no integer representation");
                testFunction(
                    s, 3.5, 6.5, "bad argument #2", "number has no integer representation");
                testFunction(
                    s, "3.5", 6, "bad argument #2", "number has no integer representation");
            }
        }

        SECTION("invalid j") {
            SECTION("invalid type") {
                std::string s = "Hallo Welt!";
                int i = 2;
                std::string j = "6und20";

                testFunction(s, i, j, "bad argument #3", "number expected");

                minilua::Table table = {};
                testFunction(s, i, table, "bad argument #3", "number expected");
            }

            SECTION("j is float") {
                std::string s = "Hallo Welt!";

                testFunction(s, 3, 6.5, "bad argument #3", "number has no integer representation");
                testFunction(
                    s, "3", 6.5, "bad argument #3", "number has no integer representation");
                testFunction(
                    s, 3, "6.5", "bad argument #3", "number has no integer representation");
            }
        }
    }

    SECTION("REVERSE") {
        SECTION("Valid force") {
            std::string s = "Allo";
            minilua::Value v = minilua::Value(s).with_origin(minilua::LiteralOrigin());
            ctx = ctx.make_new({v});
            auto res = minilua::string::byte(ctx).get(0);
            REQUIRE(res == minilua::Value(65));

            auto result = res.force(97);
            std::string expected_string = minilua::Value("allo").to_literal();
            REQUIRE(result.has_value());

            CHECK(result.value().collect_first_alternative()[0] == minilua::SourceChange(minilua::Range(), expected_string));
        }

        SECTION("Invalid force") {

        }
    }
}