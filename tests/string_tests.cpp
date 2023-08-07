#include <catch2/catch.hpp>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <list>
#include <random>
#include <sstream>
#include <string>
#include <utility>
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
        auto testFunction =
            [&ctx](auto str, auto i, auto expected_result, unsigned int expected_size = 1) {
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
        auto testFunction =
            [&ctx](auto str, auto i, auto expected_result, unsigned int expected_size = 1) {
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
                                auto str, auto j, unsigned int num_expected_results,
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

        j = -3;
        testFunction(s, j, 3, {72, 97, 108});
    }

    SECTION("Number, Nil, Number") {
        auto testFunction = [&ctx](
                                auto str, auto j, unsigned int num_expected_results,
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
                                auto str, auto i, auto j, unsigned int num_expected_results,
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

        i = 3;
        j = -8;
        testFunction(s, i, j, 0, {minilua::Nil()});
    }

    SECTION("String, String, Number") {
        auto testFunction = [&ctx](
                                auto str, auto i, auto j, unsigned int num_expected_results,
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
                                auto str, auto i, auto j, unsigned int num_expected_results,
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
                                auto str, auto i, auto j, unsigned int num_expected_results,
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
                                auto str, auto i, auto j, unsigned int num_expected_results,
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
                                auto str, auto i, auto j, unsigned int num_expected_results,
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
                                auto str, auto i, auto j, unsigned int num_expected_results,
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
                                auto str, auto i, auto j, unsigned int num_expected_results,
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
            minilua::Value str = minilua::Value(s).with_origin(minilua::LiteralOrigin());
            ctx = ctx.make_new({str});
            auto res = minilua::string::byte(ctx).get(0);
            REQUIRE(res == minilua::Value(65));

            auto result = res.force(97);
            std::string expected_string = minilua::Value("allo").to_literal();
            REQUIRE(result.has_value());

            CHECK(
                result.value().collect_first_alternative()[0] ==
                minilua::SourceChange(minilua::Range(), expected_string));

            minilua::Value i = minilua::Value(3).with_origin(minilua::LiteralOrigin());
            ctx = ctx.make_new({str, i});
            res = minilua::string::byte(ctx).get(0);
            REQUIRE(res == minilua::Value(108));

            result = res.force(76);
            expected_string = minilua::Value("AlLo").to_literal();
            REQUIRE(result.has_value());

            CHECK(
                result.value().collect_first_alternative()[0] ==
                minilua::SourceChange(minilua::Range(), expected_string));
        }

        SECTION("Invalid force") {
            std::string s = "Allo";
            minilua::Value str = minilua::Value(s).with_origin(minilua::LiteralOrigin());
            ctx = ctx.make_new({str});
            auto res = minilua::string::byte(ctx).get(0);
            REQUIRE(res == minilua::Value(65));

            auto result = res.force(true);
            CHECK_FALSE(result.has_value());
        }
    }
}

TEST_CASE("string.char") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    auto test_function = [&ctx](minilua::Vallist args, auto expected_result) {
        ctx = ctx.make_new(std::move(args));
        auto result = minilua::string::Char(ctx);
        CHECK(result == minilua::Value(expected_result));
    };

    SECTION("Numbers") {
        minilua::Vallist args{65, 102, 102, 101};
        test_function(args, "Affe");
    }

    SECTION("Strings") {
        minilua::Vallist args{"65", 102, 102, 101};
        test_function(args, "Affe");
    }

    SECTION("Invalid Input") {
        SECTION("Number out of range") {
            minilua::Vallist args{"65", 102, 102, -1};
            ctx = ctx.make_new(std::move(args));

            CHECK_THROWS_WITH(
                minilua::string::Char(ctx),
                Contains("bad argument #4") && Contains("value out of range"));

            args = {"65", 102, 1020, -1};
            ctx = ctx.make_new(std::move(args));

            CHECK_THROWS_WITH(
                minilua::string::Char(ctx),
                Contains("bad argument #3") && Contains("value out of range"));
        }

        SECTION("Malformated String") {
            minilua::Vallist args{"Baum", 102, 102, -1};
            ctx = ctx.make_new(std::move(args));

            CHECK_THROWS_WITH(
                minilua::string::Char(ctx),
                Contains("bad argument #1") && Contains("number expected, got string"));
        }
    }

    SECTION("Reverse") {
        SECTION("Valid force") {
            std::vector<minilua::Value> v;
            for (minilua::Value arg : {65, 102, 102, 101}) {
                v.push_back(arg.with_origin(minilua::LiteralOrigin()));
            }
            minilua::Vallist args(v);
            ctx = ctx.make_new(std::move(args));
            auto res = minilua::string::Char(ctx);
            REQUIRE(res == minilua::Value("Affe"));

            auto result = res.force("affe");
            std::vector<minilua::SourceChangeTree> expected;
            for (auto a : {97, 102, 102, 101}) {
                expected.emplace_back(minilua::SourceChange(minilua::Range(), std::to_string(a)));
            }

            REQUIRE(result.has_value());

            auto source_changes = result.value().collect_first_alternative();
            CHECK(source_changes.size() == expected.size());

            for (unsigned int i = 0; i < source_changes.size(); ++i) {
                CHECK(source_changes[i] == expected[i]);
            }
        }

        SECTION("Invalid force") {
            std::vector<minilua::Value> v;
            for (minilua::Value arg : {65, 102, 102, 101}) {
                v.push_back(arg.with_origin(minilua::LiteralOrigin()));
            }
            minilua::Vallist args(v);
            ctx = ctx.make_new(std::move(args));
            auto res = minilua::string::Char(ctx);
            REQUIRE(res == minilua::Value("Affe"));

            auto result = res.force(1234);
            CHECK_FALSE(result.has_value());

            result = res.force("123456");
            CHECK_FALSE(result.has_value());
        }
    }
}

TEST_CASE("string.format") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    // There are endless posibilites to combine options and modifiers.
    // Therefore only a single escape string is tested , with multiple options and modifiers

    auto test_function =
        [&ctx](const auto& formatstring, const auto& value, const std::string& expected_result) {
            ctx = ctx.make_new({formatstring, value});
            auto result = minilua::string::format(ctx);

            CHECK(result == expected_result);
        };

    SECTION("Valid input") {
        SECTION("%s") {
            std::string s = "Hallo";
            test_function(s, minilua::Nil(), s);

            s = "Hallo %s";
            test_function(s, "Welt!", "Hallo Welt!");
            test_function(s, 123, "Hallo 123");
            test_function(s, true, "Hallo true");
            test_function(s, 23.45, "Hallo 23.45");
            test_function("hallo %#0-9scon", 12, "hallo 12       con");
            test_function("hallo %#+9scon", 12, "hallo        12con");
            test_function("hallo %scon", 12, "hallo 12con");
        }

        SECTION("%%") {
            test_function("%%", minilua::Nil(), "%");
            test_function("hallo%%welt", minilua::Nil(), "hallo%welt");
        }

        SECTION("Integer format strings") {
            SECTION("%c") {
                std::string s = "Hallo";

                test_function(s, "bla", s);
                test_function("%c", 65, "A");
                test_function("%3cus", 65, "  Aus");
                test_function("%-3.5cus", 65, "A  us");
            }

            SECTION("%d") {
                std::string s = "Hallo";

                test_function(s, "bla", s);
                test_function("%d", 65, "65");
                test_function("%-3dus", 65, "65 us");
                test_function("%8.5dus", 65, "   00065us");
                test_function("%#+3d", 65, "+65");
                test_function("%#3d", -65, "-65");
                test_function("% d", 65, " 65");
            }

            SECTION("%i") {
                std::string s = "Hallo";

                test_function(s, "bla", s);
                test_function("%i", 65, "65");
                test_function("%-3ius", 65, "65 us");
                test_function("%8.5ius", 65, "   00065us");
                test_function("%#+3i", 65, "+65");
                test_function("%#3i", -65, "-65");
                test_function("%#3i", 65, " 65");

                test_function(
                    "The number %i is the answer to everything.", 42,
                    "The number 42 is the answer to everything.");
            }

            SECTION("%o") {
                std::string s = "Hallo";

                test_function(s, "bla", s);
                test_function("%o", 10, "12");
                test_function("%#o", 10, "012");
                test_function("%4o", 10, "  12");
                test_function("%-4o", 10, "12  ");
                test_function("%#4o", 10, " 012");
                test_function("%-#4o", 10, "012 ");
                test_function("% .5o", 10, "00012");
                test_function("%# 6.4o", 10, "  0012");
                test_function("%04o", 10, "0012");
                test_function("%-#04o", 10, "012 ");
            }

            SECTION("%u") {
                std::string s = "Hallo";

                test_function(s, "bla", s);
                test_function("%u", 42, "42");
                test_function("%u", -42, "18446744073709551574");
                test_function("%5u", 42.0, "   42");
                test_function("% 5u", 42., "   42");
                test_function("%05u", 42.0, "00042");
                test_function("%0-5u", 42, "42   ");
                test_function("%u", std::pow(2, 36), "68719476736");
            }

            SECTION("%x, %X") {
                std::string s = "Hallo";

                test_function(s, "bla", s);
                test_function("%x", 15, "f");
                test_function("%#x", 15, "0xf");
                test_function("%04x", 15, "000f");
                test_function("%0#4x", 15, "0x0f");
                test_function("%0#.4x", 15, "0x000f");
                test_function("%0#-4x", 15, "0xf ");

                test_function("%X", 15, "F");
                test_function("%#X", 15, "0XF");
                test_function("%04X", 15, "000F");
                test_function("%0#4X", 15, "0X0F");
                test_function("%0#.4X", 15, "0X000F");
                test_function("%0#-4X", 15, "0XF ");
                test_function("%4X", 15, "   F");
            }
        }

        SECTION("Number format strings") {
            SECTION("%a, %A") {
                std::string s = "Hallo";

                test_function(s, "bla", s);
                test_function("%a", 15, "0x1.ep+3");
                test_function("%a", 1.5, "0x1.8p+0");
                test_function("%#a", 15, "0x1.ep+3");
                test_function("%08a", 15, "0x1.ep+3");
                test_function("%010alm", 20, "0x001.4p+4lm");
                test_function("%-010alm", 20, "0x1.4p+4  lm");
                test_function("%-010.5alm", 20, "0x1.40000p+4lm");
                test_function("% -010.5alm", 20, " 0x1.40000p+4lm");
                test_function("%+-010.5alm", 20, "+0x1.40000p+4lm");
                test_function("%-010.5alm", -20, "-0x1.40000p+4lm");

                test_function("%A", 15, "0X1.EP+3");
                test_function("%A", 1.5, "0X1.8P+0");
                test_function("%#A", 15, "0X1.EP+3");
                test_function("%08A", 15, "0X1.EP+3");
                test_function("%010Alm", 20, "0X001.4P+4lm");
                test_function("%-010Alm", 20, "0X1.4P+4  lm");
                test_function("%-010.5Alm", 20, "0X1.40000P+4lm");
                test_function("%+-010.5Alm", 20, "+0X1.40000P+4lm");
                test_function("%-010.5Alm", -20, "-0X1.40000P+4lm");
                test_function("% -010.5Alm", 20, " 0X1.40000P+4lm");
            }

            SECTION("%e, %E") {
                std::string s = "Hallo";

                test_function(s, "bla", s);
                test_function("%e", 15, "1.500000e+01");
                test_function("%e", 1.5, "1.500000e+00");
                test_function("%.0e", 15, "2e+01");
                test_function("%#.0e", 15, "2.e+01");
                test_function("%.0e", 14.8734, "1e+01");
                test_function("%016e", 15, "00001.500000e+01");
                test_function("%16e", 15, "    1.500000e+01");
                test_function("% 16e", 15, "    1.500000e+01");
                test_function("% -015elm", 0.12345, " 1.234500e-01  lm");
                test_function("% e", 15, " 1.500000e+01");
                test_function("%+ e", 15, "+1.500000e+01");
                test_function("%.3e", 15, "1.500e+01");

                test_function("%E", 15, "1.500000E+01");
                test_function("%E", 1.5, "1.500000E+00");
                test_function("%.0E", 15, "2E+01");
                test_function("%#.0E", 15, "2.E+01");
                test_function("%.0E", 14.8734, "1E+01");
                test_function("%016E", 15, "00001.500000E+01");
                test_function("%16E", 15, "    1.500000E+01");
                test_function("% 16E", 15, "    1.500000E+01");
                test_function("% -015Elm", 0.12345, " 1.234500E-01  lm");
                test_function("% E", 15, " 1.500000E+01");
                test_function("%+ E", 15, "+1.500000E+01");
                test_function("%.3E", 15, "1.500E+01");
            }

            SECTION("%f") {
                std::string s = "Hallo";

                test_function(s, "bla", s);
                test_function("%f", 15, "15.000000");
                test_function("%f", 15.123456789, "15.123457");
                test_function("%f", 15.987654321, "15.987654");
                test_function("%.0f", 15.987654321, "16");
                test_function("%#.0f", 15.21, "15.");
                test_function("%5.0f", 15.987654321, "   16");
                test_function("%05.0f", 15.987654321, "00016");
                test_function("%-05.0f", 15.987654321, "16   ");
                test_function("% .2f", 15.21, " 15.21");
                test_function("%+.2f", 15.21, "+15.21");
                test_function("%-.2f", 15.21, "15.21");
                test_function("%-.2f", -15.21, "-15.21");
            }

            SECTION("%g, %G") {
                std::string s = "Hallo";

                test_function(s, "bla", s);
                test_function("%g", 15, "15");
                test_function("%#g", 15, "15.0000");
                test_function("%#.2g", 15, "15.");
                test_function("%g", 0.123, "0.123");
                test_function("%g", 0.123456, "0.123456");
                test_function("%g", 0.00000123, "1.23e-06");
                test_function("%g", -0.00000123, "-1.23e-06");
                test_function("% g", 0.00000123, " 1.23e-06");
                test_function("%+g", 0.00000123, "+1.23e-06");
                test_function("% g", -0.00000123, "-1.23e-06");
                test_function("%+g", -0.00000123, "-1.23e-06");
                test_function("%5g", 42, "   42");
                test_function("%05g", 42, "00042");
                test_function("%-05g", 42, "42   ");
                test_function("%-5g", 42, "42   ");
                test_function("%#09.6g", 42, "0042.0000");

                test_function("%G", 15, "15");
                test_function("%#G", 15, "15.0000");
                test_function("%#.2G", 15, "15.");
                test_function("%G", 0.123, "0.123");
                test_function("%G", 0.123456, "0.123456");
                test_function("%G", 0.00000123, "1.23E-06");
                test_function("%G", -0.00000123, "-1.23E-06");
                test_function("% G", 0.00000123, " 1.23E-06");
                test_function("%+G", 0.00000123, "+1.23E-06");
                test_function("% G", -0.00000123, "-1.23E-06");
                test_function("%+G", -0.00000123, "-1.23E-06");
                test_function("%5G", 42, "   42");
                test_function("%05G", 42, "00042");
                test_function("%-05G", 42, "42   ");
                test_function("%-5G", 42, "42   ");
                test_function("%#09.6G", 42, "0042.0000");
            }
        }

        SECTION("Combination of escape strings") {
            // Only a couple of combinations will be tested to check for correct behaviour
            auto test_function_2 = [&ctx](
                                       const auto& formatstring, const auto& value_1,
                                       const auto& value_2, const std::string& expected_result) {
                ctx = ctx.make_new({formatstring, value_1, value_2});
                auto result = minilua::string::format(ctx);

                CHECK(result == expected_result);
            };

            test_function_2("%s%s", "Hallo ", "Welt!", "Hallo Welt!");
            test_function_2("%s%i", "Tree", 32, "Tree32");
            test_function_2("Dieser %s ist ein %s", "Text", "Erfolg", "Dieser Text ist ein Erfolg");
        }
    }

    SECTION("Invalid input") {
        // hallo %#0-' '9scon
        SECTION("Invalid format string") {
            ctx = ctx.make_new({true});
            CHECK_THROWS_WITH(
                minilua::string::format(ctx),
                Contains("bad argument #1") && Contains("string expected, got boolean"));

            ctx = ctx.make_new({"%123", 123});
            CHECK_THROWS_WITH(
                minilua::string::format(ctx),
                Contains("invalid format") && Contains("width or precision too long"));

            ctx = ctx.make_new({"%w", 123});
            CHECK_THROWS_WITH(
                minilua::string::format(ctx), Contains("invalid option '%w' to 'format'"));

            ctx = ctx.make_new({"hallo %#0-' '9scon", 123});
            CHECK_THROWS_WITH(
                minilua::string::format(ctx),
                Contains("invalid option") && Contains("to 'format'"));

            SECTION("invalid options for %%") {
                ctx = ctx.make_new({"%#%", 12});
                CHECK_THROWS_WITH(
                    minilua::string::format(ctx),
                    Contains("invalid option") && Contains("to 'format'"));

                ctx = ctx.make_new({"% %", 12});
                CHECK_THROWS_WITH(
                    minilua::string::format(ctx),
                    Contains("invalid option") && Contains("to 'format'"));
                ctx = ctx.make_new({"%-%", 12});
                CHECK_THROWS_WITH(
                    minilua::string::format(ctx),
                    Contains("invalid option") && Contains("to 'format'"));

                ctx = ctx.make_new({"%+%", 12});
                CHECK_THROWS_WITH(
                    minilua::string::format(ctx),
                    Contains("invalid option") && Contains("to 'format'"));

                ctx = ctx.make_new({"%0%", 12});
                CHECK_THROWS_WITH(
                    minilua::string::format(ctx),
                    Contains("invalid option") && Contains("to 'format'"));

                ctx = ctx.make_new({"%23%", 12});
                CHECK_THROWS_WITH(
                    minilua::string::format(ctx),
                    Contains("invalid option") && Contains("to 'format'"));

                ctx = ctx.make_new({"%.91%", 12});
                CHECK_THROWS_WITH(
                    minilua::string::format(ctx),
                    Contains("invalid option") && Contains("to 'format'"));
            }
        }

        SECTION("argument is of invalid type") {
            ctx = ctx.make_new({"%i", 123.456});
            CHECK_THROWS_WITH(
                minilua::string::format(ctx),
                Contains("bad argument #2") && Contains("number has no integer representation"));

            ctx = ctx.make_new({"%i", minilua::Table()});
            CHECK_THROWS_WITH(
                minilua::string::format(ctx),
                Contains("bad argument #2") && Contains("number expected, got table"));

            ctx = ctx.make_new({"%s%i", "Hallo", minilua::Table()});
            CHECK_THROWS_WITH(
                minilua::string::format(ctx),
                Contains("bad argument #3") && Contains("number expected, got table"));
        }
    }
}

TEST_CASE("string.len") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);
    auto test_function = [&ctx](const auto& str, int expected) {
        ctx = ctx.make_new({str});
        auto result = minilua::string::len(ctx);
        int len = std::get<minilua::Number>(result).try_as_int();

        CHECK(len == expected);
    };

    SECTION("String") {
        std::vector<std::string> cases = {"hello", "", "123456"};

        for (const auto& s : cases) {
            test_function(s, s.length());
        }
    }

    SECTION("Number") {
        std::vector<int> cases = {123, -10, 0};

        for (const auto& s : cases) {
            test_function(s, std::to_string(s).length());
        }

        test_function(-23.98, 6);
    }

    SECTION("Invalid Input") {
        ctx = ctx.make_new({true});
        CHECK_THROWS_WITH(
            minilua::string::len(ctx),
            Contains("bad argument #1") && Contains("string expected, got bool"));

        ctx = ctx.make_new({minilua::Table()});
        CHECK_THROWS_WITH(
            minilua::string::len(ctx),
            Contains("bad argument #1") && Contains("string expected, got table"));

        ctx = ctx.make_new({minilua::Nil()});
        CHECK_THROWS_WITH(
            minilua::string::len(ctx),
            Contains("bad argument #1") && Contains("string expected, got nil"));
    }
}

TEST_CASE("string.lower") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);
    auto test_function = [&ctx](const auto& str, const std::string& expected) {
        ctx = ctx.make_new({str});
        auto result = minilua::string::lower(ctx);

        CHECK(result == minilua::Value(expected));
    };

    SECTION("String") {
        test_function("hallo", "hallo");

        test_function("HALLO", "hallo");

        test_function("WeLt!", "welt!");

        test_function("", "");

        test_function("🙂", "🙂");

        test_function("!§$%&/()=?*'_:;", "!§$%&/()=?*'_:;");
    }

    SECTION("Number") {
        test_function(12345, "12345");

        test_function(-5, "-5");

        test_function(-3.56, "-3.56");
    }

    SECTION("Invalid Input") {
        ctx = ctx.make_new({true});
        CHECK_THROWS_WITH(
            minilua::string::lower(ctx),
            Contains("bad argument #1") && Contains("string expected, got boolean"));

        ctx = ctx.make_new({minilua::Nil()});
        CHECK_THROWS_WITH(
            minilua::string::lower(ctx),
            Contains("bad argument #1") && Contains("string expected, got nil"));
    }
}

TEST_CASE("string.rep") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("String, Number, Nil") {
        auto test_function = [&ctx](const auto& s, const auto& n, const std::string& expected) {
            ctx = ctx.make_new({s, n});
            auto result = minilua::string::rep(ctx);

            CHECK(result == expected);
        };

        test_function("Hi", 3, "HiHiHi");

        test_function("Hi", -3, "");

        test_function("", 5, "");

        test_function("Baum", 0, "");

        test_function("Hi", 3.0, "HiHiHi");

        test_function("Hi", 3., "HiHiHi");
    }

    SECTION("String, String, Nil") {
        auto test_function = [&ctx](const auto& s, const auto& n, const std::string& expected) {
            ctx = ctx.make_new({s, n});
            auto result = minilua::string::rep(ctx);

            CHECK(result == expected);
        };

        test_function("Hi", "3", "HiHiHi");

        test_function("Hi", "-3", "");

        test_function("", "5", "");

        test_function("Baum", "0", "");

        test_function("Hi", "3.0", "HiHiHi");

        test_function("Hi", "3.", "HiHiHi");
    }

    SECTION("Number, Number, Nil") {
        auto test_function = [&ctx](const auto& s, const auto& n, const std::string& expected) {
            ctx = ctx.make_new({s, n});
            auto result = minilua::string::rep(ctx);

            CHECK(result == expected);
        };

        test_function(12, 3, "121212");

        test_function(45, -3, "");

        test_function(0, 5, "00000");

        test_function(123456789, 0, "");

        test_function(123, 3.0, "123123123");

        test_function(456, 3., "456456456");
    }

    SECTION("Number, String, Nil") {
        auto test_function = [&ctx](const auto& s, const auto& n, const std::string& expected) {
            ctx = ctx.make_new({s, n});
            auto result = minilua::string::rep(ctx);

            CHECK(result == expected);
        };

        test_function(12, "3", "121212");

        test_function(45, "-3", "");

        test_function(0, "5", "00000");

        test_function(05, "5", "55555");

        test_function(123456789, "0", "");

        test_function(123, "3.0", "123123123");

        test_function(456, "3.", "456456456");
    }

    SECTION("String, Number, String") {
        auto test_function =
            [&ctx](const auto& s, const auto& n, const auto& sep, const std::string& expected) {
                ctx = ctx.make_new({s, n, sep});
                auto result = minilua::string::rep(ctx);

                CHECK(result == expected);
            };

        test_function("Hi", 3, " ", "Hi Hi Hi");

        test_function("Hi", -3, "Tim", "");

        test_function("", 5, "n", "nnnn");

        test_function("Baum", 0, "clear", "");

        test_function("Hi", 3.0, " ", "Hi Hi Hi");

        test_function("Hi", 3., " ", "Hi Hi Hi");

        test_function("Zeile", 2, "\n", "Zeile\nZeile");

        test_function("Zeile", 1, "\n", "Zeile");
    }

    SECTION("String, Number, Number") {
        auto test_function =
            [&ctx](const auto& s, const auto& n, const auto& sep, const std::string& expected) {
                ctx = ctx.make_new({s, n, sep});
                auto result = minilua::string::rep(ctx);

                CHECK(result == expected);
            };

        test_function("Hi", 3, 1, "Hi1Hi1Hi");

        test_function("Hi", -3, 42, "");

        test_function("", 5, 6, "6666");

        test_function("Baum", 0, 100, "");

        test_function("Hi", 3.0, 6, "Hi6Hi6Hi");

        test_function("Hi", 3., 6, "Hi6Hi6Hi");

        test_function("Zeile", 2, 42, "Zeile42Zeile");

        test_function("Zeile", 1, 55, "Zeile");
    }

    SECTION("String, String, String") {
        auto test_function =
            [&ctx](const auto& s, const auto& n, const auto& sep, const std::string& expected) {
                ctx = ctx.make_new({s, n, sep});
                auto result = minilua::string::rep(ctx);

                CHECK(result == expected);
            };

        test_function("Hi", "3", "1", "Hi1Hi1Hi");

        test_function("Hi", "-3", "42", "");

        test_function("", "5", "6", "6666");

        test_function("Baum", "0", "100", "");

        test_function("Hi", "3.0", "6", "Hi6Hi6Hi");

        test_function("Hi", "3.", "6", "Hi6Hi6Hi");

        test_function("Zeile", "2", "42", "Zeile42Zeile");

        test_function("Zeile", "1", "55", "Zeile");
    }

    SECTION("String, String, Number") {
        auto test_function =
            [&ctx](const auto& s, const auto& n, const auto& sep, const std::string& expected) {
                ctx = ctx.make_new({s, n, sep});
                auto result = minilua::string::rep(ctx);

                CHECK(result == expected);
            };

        test_function("Hi", "3", 1, "Hi1Hi1Hi");

        test_function("Hi", "-3", 42, "");

        test_function("", "5", 6, "6666");

        test_function("Baum", "0", 100, "");

        test_function("Hi", "3.0", 6, "Hi6Hi6Hi");

        test_function("Hi", "3.", 6, "Hi6Hi6Hi");

        test_function("Zeile", "2", 42, "Zeile42Zeile");

        test_function("Zeile", "1", 55, "Zeile");
    }

    SECTION("Number, Number, String") {
        auto test_function =
            [&ctx](const auto& s, const auto& n, const auto& sep, const std::string& expected) {
                ctx = ctx.make_new({s, n, sep});
                auto result = minilua::string::rep(ctx);

                CHECK(result == expected);
            };

        test_function(12, 4, ".", "12.12.12.12");

        test_function(45, -3, "zweiundviewzig", "");

        test_function(0, 5, "n", "0n0n0n0n0");

        test_function(123456789, 0, "baum", "");

        test_function(123, 3.0, " ", "123 123 123");

        std::stringstream ss;
        ss << "456"
           << "\t"
           << "456"
           << "\t"
           << "456";
        test_function(456, 3., "\t", ss.str());
    }

    SECTION("Number, Number, Number") {
        auto test_function =
            [&ctx](const auto& s, const auto& n, const auto& sep, const std::string& expected) {
                ctx = ctx.make_new({s, n, sep});
                auto result = minilua::string::rep(ctx);

                CHECK(result == expected);
            };

        test_function(12, 4, 3, "12312312312");

        test_function(45, -3, 45, "");

        test_function(0, 5, -3, "0-30-30-30-30");

        test_function(123456789, 0, 45, "");

        test_function(123, 3.0, 4, "12341234123");

        test_function(454, 3., 5, "45454545454");
    }

    SECTION("Number, String, String") {
        auto test_function =
            [&ctx](const auto& s, const auto& n, const auto& sep, const std::string& expected) {
                ctx = ctx.make_new({s, n, sep});
                auto result = minilua::string::rep(ctx);

                CHECK(result == expected);
            };

        test_function(12, "4", ".", "12.12.12.12");

        test_function(45, "-3", "zweiundviewzig", "");

        test_function(0, "5", "n", "0n0n0n0n0");

        test_function(123456789, "0", "baum", "");

        test_function(123, "3.0", " ", "123 123 123");

        std::stringstream ss;
        ss << "456"
           << "\t"
           << "456"
           << "\t"
           << "456";
        test_function(456, "3.", "\t", ss.str());
    }

    SECTION("Number, String, Number") {
        auto test_function =
            [&ctx](const auto& s, const auto& n, const auto& sep, const std::string& expected) {
                ctx = ctx.make_new({s, n, sep});
                auto result = minilua::string::rep(ctx);

                CHECK(result == expected);
            };

        test_function(12, "4", 3, "12312312312");

        test_function(45, "-3", 45, "");

        test_function(0, "5", -3, "0-30-30-30-30");

        test_function(123456789, "0", 45, "");

        test_function(123, "3.0", 4, "12341234123");

        test_function(454, "3.", 5, "45454545454");
    }

    SECTION("Invalid Input") {
        auto test_function = [&ctx](
                                 const auto& s, const auto& n, const auto& sep,
                                 const std::string& expected_error_message_part_1,
                                 const std::string& expected_error_message_part_2) {
            ctx = ctx.make_new({s, n, sep});

            CHECK_THROWS_WITH(
                minilua::string::rep(ctx),
                Contains(expected_error_message_part_1) && Contains(expected_error_message_part_2));
        };

        SECTION("s is no string") {
            test_function(
                true, "baum", minilua::Nil(), "bad argument #1", "string expected, got boolean");

            test_function(
                minilua::Table(), 42, "sep", "bad argument #1", "string expected, got table");
        }

        SECTION("n is no number") {
            test_function(
                "hallo", "welt", "space", "bad argument #2", "number expected, got string");

            test_function("Hallo ", false, true, "bad argument #2", "number expected, got boolean");
        }

        SECTION("n is no integer") {
            test_function(
                "hallo", 42.42, "space", "bad argument #2", "number has no integer representation");

            test_function(
                "hallo", -42.24, "space", "bad argument #2",
                "number has no integer representation");
        }

        SECTION("sep is no string") {
            test_function(
                "hallo", 42, minilua::Table(), "bad argument #3", "string expected, got table");

            test_function("hallo", -42, true, "bad argument #3", "string expected, got boolean");
        }
    }
}

TEST_CASE("string.reverse") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    auto test_function = [&ctx](const auto& s, const std::string& expected) {
        ctx = ctx.make_new({s});
        auto result = minilua::string::reverse(ctx);

        CHECK(result == minilua::Value(expected));
    };

    SECTION("String") {
        std::vector<std::string> args = {"ollaH", "Hallo", "🙂🙃"};

        for (const auto& arg : args) {
            std::string expected = arg;
            std::reverse(expected.begin(), expected.end());
            test_function(arg, expected);
        }
    }

    SECTION("Number") {
        std::vector<int> args = {12345, 54321, 1, -15};

        for (const auto& a : args) {
            std::string arg = std::to_string(a);
            std::string expected = arg;
            std::reverse(expected.begin(), expected.end());
            test_function(arg, expected);
        }

        test_function(32.45, "54.23");
    }

    SECTION("Invalid Input") {
        ctx = ctx.make_new({true});
        CHECK_THROWS_WITH(
            minilua::string::reverse(ctx),
            Contains("bad argument #1") && Contains("string expected, got boolean"));

        ctx = ctx.make_new({minilua::Nil()});
        CHECK_THROWS_WITH(
            minilua::string::reverse(ctx),
            Contains("bad argument #1") && Contains("string expected, got nil"));

        ctx = ctx.make_new({minilua::Table()});
        CHECK_THROWS_WITH(
            minilua::string::reverse(ctx),
            Contains("bad argument #1") && Contains("string expected, got table"));
    }

    SECTION("reverse value") {
        SECTION("Valid force") {
            ctx = ctx.make_new({minilua::Value("Hallo").with_origin(minilua::LiteralOrigin())});
            auto res = minilua::string::reverse(ctx);

            REQUIRE(res == minilua::Value("ollaH"));

            auto result = res.force("nomiS");

            REQUIRE(result.has_value());

            std::string expected = minilua::Value("Simon").to_literal();

            CHECK(
                result.value().collect_first_alternative()[0] ==
                minilua::SourceChange(minilua::Range(), expected));

            ctx = ctx.make_new({minilua::Value(12345).with_origin(minilua::LiteralOrigin())});
            res = minilua::string::reverse(ctx);

            REQUIRE(res == minilua::Value("54321"));

            result = res.force("PI");

            REQUIRE(result.has_value());

            expected = minilua::Value("IP").to_literal();

            CHECK(
                result.value().collect_first_alternative()[0] ==
                minilua::SourceChange(minilua::Range(), expected));
        }

        SECTION("Invalid force") {
            ctx = ctx.make_new({minilua::Value("Hallo").with_origin(minilua::LiteralOrigin())});
            auto res = minilua::string::reverse(ctx);

            REQUIRE(res == minilua::Value("ollaH"));

            auto result = res.force(56);

            CHECK_FALSE(result.has_value());

            result = res.force(minilua::Table());

            CHECK_FALSE(result.has_value());

            result = res.force(minilua::Nil());

            CHECK_FALSE(result.has_value());
        }
    }
}

TEST_CASE("string.sub") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("String, Number, Nil") {
        auto test_function = [&ctx](const auto& s, const auto& i, const std::string& expected) {
            ctx = ctx.make_new({minilua::Value(s), minilua::Value(i)});
            auto result = minilua::string::sub(ctx);

            REQUIRE(result.type() == minilua::String::TYPE);
            CHECK(result == expected);
        };

        test_function("HalloWelt!", 6, "Welt!");

        test_function("HalloWelt!", 6.0, "Welt!");

        test_function("Hallo", 6, "");

        test_function("Hallo", -3, "llo");

        test_function("Hallo", -7, "Hallo");

        test_function("Hallo", 0, "Hallo");

        test_function("Lachender 😃", 5, "ender 😃");
    }

    SECTION("String, String, Nil") {
        auto test_function = [&ctx](const auto& s, const auto& i, const std::string& expected) {
            ctx = ctx.make_new({minilua::Value(s), minilua::Value(i)});
            auto result = minilua::string::sub(ctx);

            REQUIRE(result.type() == minilua::String::TYPE);
            CHECK(result == expected);
        };

        test_function("HalloWelt!", "6", "Welt!");

        test_function("HalloWelt!", "6.0", "Welt!");

        test_function("Hallo", "6", "");

        test_function("Hallo", "-3", "llo");

        test_function("Hallo", "-7", "Hallo");

        test_function("Hallo", "0", "Hallo");

        test_function("Lachender 😃", "5", "ender 😃");
    }

    SECTION("Number, Number, Nil") {
        auto test_function = [&ctx](const auto& s, const auto& i, const std::string& expected) {
            ctx = ctx.make_new({minilua::Value(s), minilua::Value(i)});
            auto result = minilua::string::sub(ctx);

            REQUIRE(result.type() == minilua::String::TYPE);
            CHECK(result == expected);
        };

        test_function("HalloWelt!", 6, "Welt!");

        test_function("HalloWelt!", 6.0, "Welt!");

        test_function("Hallo", 6, "");

        test_function("Hallo", -3, "llo");

        test_function("Hallo", -7, "Hallo");

        test_function("Hallo", 0, "Hallo");

        test_function("Lachender 😃", 5, "ender 😃");
    }

    SECTION("Number, String, Nil") {
        auto test_function = [&ctx](const auto& s, const auto& i, const std::string& expected) {
            ctx = ctx.make_new({minilua::Value(s), minilua::Value(i)});
            auto result = minilua::string::sub(ctx);

            REQUIRE(result.type() == minilua::String::TYPE);
            CHECK(result == expected);
        };

        test_function(123456, "3", "3456");

        test_function(123456, "3.0", "3456");

        test_function(123456, "8", "");

        test_function(123456, "-3", "456");

        test_function(123456, "-8", "123456");

        test_function(123456, "0", "123456");

        test_function(123.456, "5", "456");
    }

    SECTION("String, Number, Number") {
        auto test_function =
            [&ctx](const auto& s, const auto& i, const auto& j, const std::string& expected) {
                ctx = ctx.make_new({minilua::Value(s), minilua::Value(i), minilua::Value(j)});
                auto result = minilua::string::sub(ctx);

                REQUIRE(result.type() == minilua::String::TYPE);
                CHECK(result == expected);
            };

        test_function("HalloWelt!", 6, 8, "Wel");

        test_function("HalloWelt!", 6.0, 8, "Wel");

        test_function("HalloWelt!", 6, 8.0, "Wel");

        test_function("HalloWelt!", 6.0, 8.0, "Wel");

        test_function("HalloWelt!", 6, 8., "Wel");

        test_function("HalloWelt!", 6., 8, "Wel");

        test_function("Hallo", 6, 8, "");

        test_function("Hallo", 6, 3, "");

        test_function("Hallo", 2, 6, "allo");

        test_function("Hallo", -7, -6, "");

        test_function("Hallo", 2, -3, "al");

        test_function("Hallo", -4, -3, "al");

        test_function("Hallo", -4, 4, "all");

        test_function("Hallo", 4, 4, "l");

        test_function("Hallo", 0, 5, "Hallo");

        test_function("Lachender 😃", 10, 14, " 😃");
    }

    SECTION("String, String, Number") {
        auto test_function =
            [&ctx](const auto& s, const auto& i, const auto& j, const std::string& expected) {
                ctx = ctx.make_new({minilua::Value(s), minilua::Value(i), minilua::Value(j)});
                auto result = minilua::string::sub(ctx);

                REQUIRE(result.type() == minilua::String::TYPE);
                CHECK(result == expected);
            };

        test_function("HalloWelt!", "6", 8, "Wel");

        test_function("HalloWelt!", "6.0", 8, "Wel");

        test_function("HalloWelt!", "6", 8.0, "Wel");

        test_function("HalloWelt!", "6.0", 8.0, "Wel");

        test_function("HalloWelt!", "6", 8., "Wel");

        test_function("HalloWelt!", "6.", 8, "Wel");

        test_function("Hallo", "6", 8, "");

        test_function("Hallo", "6", 3, "");

        test_function("Hallo", "2", 6, "allo");

        test_function("Hallo", "-7", -6, "");

        test_function("Hallo", "2", -3, "al");

        test_function("Hallo", "-4", -3, "al");

        test_function("Hallo", "-4", 4, "all");

        test_function("Hallo", "4", 4, "l");

        test_function("Hallo", "0", 5, "Hallo");

        test_function("Lachender 😃", "10", 14, " 😃");
    }

    SECTION("String, Number, String") {
        auto test_function =
            [&ctx](const auto& s, const auto& i, const auto& j, const std::string& expected) {
                ctx = ctx.make_new({minilua::Value(s), minilua::Value(i), minilua::Value(j)});
                auto result = minilua::string::sub(ctx);

                REQUIRE(result.type() == minilua::String::TYPE);
                CHECK(result == expected);
            };

        test_function("HalloWelt!", 6, "8", "Wel");

        test_function("HalloWelt!", 6.0, "8", "Wel");

        test_function("HalloWelt!", 6, "8.0", "Wel");

        test_function("HalloWelt!", 6.0, "8.0", "Wel");

        test_function("HalloWelt!", 6, "8.", "Wel");

        test_function("HalloWelt!", 6., "8", "Wel");

        test_function("Hallo", 6, "8", "");

        test_function("Hallo", 6, "3", "");

        test_function("Hallo", 2, "6", "allo");

        test_function("Hallo", -7, "-6", "");

        test_function("Hallo", 2, "-3", "al");

        test_function("Hallo", -4, "-3", "al");

        test_function("Hallo", -4, "4", "all");

        test_function("Hallo", 4, "4", "l");

        test_function("Hallo", 0, "5", "Hallo");

        test_function("Lachender 😃", 10, "14", " 😃");
    }

    SECTION("String, String, String") {
        auto test_function =
            [&ctx](const auto& s, const auto& i, const auto& j, const std::string& expected) {
                ctx = ctx.make_new({minilua::Value(s), minilua::Value(i), minilua::Value(j)});
                auto result = minilua::string::sub(ctx);

                REQUIRE(result.type() == minilua::String::TYPE);
                CHECK(result == expected);
            };

        test_function("HalloWelt!", "6", "8", "Wel");

        test_function("HalloWelt!", "6.0", "8", "Wel");

        test_function("HalloWelt!", "6", "8.0", "Wel");

        test_function("HalloWelt!", "6.0", "8.0", "Wel");

        test_function("HalloWelt!", "6", "8.", "Wel");

        test_function("HalloWelt!", "6.", "8", "Wel");

        test_function("Hallo", "7", "8", "");

        test_function("Hallo", "6", "3", "");

        test_function("Hallo", "2", "6", "allo");

        test_function("Hallo", "-7", "-6", "");

        test_function("Hallo", "2", "-3", "al");

        test_function("Hallo", "-4", "-3", "al");

        test_function("Hallo", "-4", "4", "all");

        test_function("Hallo", "4", "4", "l");

        test_function("Hallo", "0", "5", "Hallo");

        test_function("Lachender 😃", "10", "14", " 😃");
    }

    SECTION("Number, Number, Number") {
        auto test_function =
            [&ctx](const auto& s, const auto& i, const auto& j, const std::string& expected) {
                ctx = ctx.make_new({minilua::Value(s), minilua::Value(i), minilua::Value(j)});
                auto result = minilua::string::sub(ctx);

                REQUIRE(result.type() == minilua::String::TYPE);
                CHECK(result == expected);
            };

        test_function(123456789, 6, 8, "678");

        test_function(123456789, 6.0, 8, "678");

        test_function(123456789, 6, 8.0, "678");

        test_function(123456789, 6.0, 8.0, "678");

        test_function(123456789, 6, 8., "678");

        test_function(123456789, 6., 8, "678");

        test_function(1234, 6, 8, "");

        test_function(1234, 6, 3, "");

        test_function(123456789, 2, 6, "23456");

        test_function(12345, -7, -6, "");

        test_function(123456789, -4, 4, "");

        test_function(123456789, 2, -3, "234567");

        test_function(123456789, -4, -3, "67");

        test_function(123456789, -4, 7, "67");

        test_function(123456789, 4, 4, "4");

        test_function(123456789, 0, 5, "12345");

        test_function(-1234, 2, 3, "12");

        test_function(23.56, 1, 3, "23.");
    }

    SECTION("Number, String, Number") {
        auto test_function =
            [&ctx](const auto& s, const auto& i, const auto& j, const std::string& expected) {
                ctx = ctx.make_new({minilua::Value(s), minilua::Value(i), minilua::Value(j)});
                auto result = minilua::string::sub(ctx);

                REQUIRE(result.type() == minilua::String::TYPE);
                CHECK(result == expected);
            };

        test_function(123456789, "6", 8, "678");

        test_function(123456789, "6.0", 8, "678");

        test_function(123456789, "6", 8.0, "678");

        test_function(123456789, "6.0", 8.0, "678");

        test_function(123456789, "6", 8., "678");

        test_function(123456789, "6.", 8, "678");

        test_function(1234, "6", 8, "");

        test_function(1234, "6", 3, "");

        test_function(123456789, "2", 6, "23456");

        test_function(12345, "-7", -6, "");

        test_function(123456789, "-4", 4, "");

        test_function(123456789, "2", -3, "234567");

        test_function(123456789, "-4", -3, "67");

        test_function(123456789, "-4", 7, "67");

        test_function(123456789, "4", 4, "4");

        test_function(123456789, "0", 5, "12345");

        test_function(-1234, "2", 3, "12");

        test_function(23.56, "1", 3, "23.");
    }

    SECTION("Number, Number, String") {
        auto test_function =
            [&ctx](const auto& s, const auto& i, const auto& j, const std::string& expected) {
                ctx = ctx.make_new({minilua::Value(s), minilua::Value(i), minilua::Value(j)});
                auto result = minilua::string::sub(ctx);

                REQUIRE(result.type() == minilua::String::TYPE);
                CHECK(result == expected);
            };

        test_function(123456789, 6, "8", "678");

        test_function(123456789, 6.0, "8", "678");

        test_function(123456789, 6, "8.0", "678");

        test_function(123456789, 6.0, "8.0", "678");

        test_function(123456789, 6, "8.", "678");

        test_function(123456789, 6., "8", "678");

        test_function(1234, 6, "8", "");

        test_function(1234, 6, "3", "");

        test_function(123456789, 2, "6", "23456");

        test_function(12345, -7, "-6", "");

        test_function(123456789, -4, "4", "");

        test_function(123456789, 2, "-3", "234567");

        test_function(123456789, -4, "-3", "67");

        test_function(123456789, -4, "7", "67");

        test_function(123456789, 4, "4", "4");

        test_function(123456789, 0, "5", "12345");

        test_function(-1234, 2, "3", "12");

        test_function(23.56, 1, "3", "23.");
    }

    SECTION("Number, String, String") {
        auto test_function =
            [&ctx](const auto& s, const auto& i, const auto& j, const std::string& expected) {
                ctx = ctx.make_new({minilua::Value(s), minilua::Value(i), minilua::Value(j)});
                auto result = minilua::string::sub(ctx);

                REQUIRE(result.type() == minilua::String::TYPE);
                CHECK(result == expected);
            };

        test_function(123456789, "6", "8", "678");

        test_function(123456789, "6.0", "8", "678");

        test_function(123456789, "6", "8.0", "678");

        test_function(123456789, "6.0", "8.0", "678");

        test_function(123456789, "6", "8.", "678");

        test_function(123456789, "6.", "8", "678");

        test_function(1234, "6", "8", "");

        test_function(1234, "6", "3", "");

        test_function(123456789, "2", "6", "23456");

        test_function(12345, "-7", "-6", "");

        test_function(123456789, "-4", "4", "");

        test_function(123456789, "2", "-3", "234567");

        test_function(123456789, "-4", "-3", "67");

        test_function(123456789, "-4", "7", "67");

        test_function(123456789, "4", "4", "4");

        test_function(123456789, "0", "5", "12345");

        test_function(-1234, "2", "3", "12");

        test_function(23.56, "1", "3", "23.");
    }

    SECTION("Invalid input") {
        SECTION("s is not string") {
            ctx = ctx.make_new({true, 1, 2});

            CHECK_THROWS_WITH(
                minilua::string::sub(ctx),
                Contains("bad argument #1") && Contains("string expected, got boolean"));

            ctx = ctx.make_new({minilua::Table(), 1, 2});

            CHECK_THROWS_WITH(
                minilua::string::sub(ctx),
                Contains("bad argument #1") && Contains("string expected, got table"));
        }

        SECTION("i is not a number") {
            ctx = ctx.make_new({"hallo", "welt", 2});

            CHECK_THROWS_WITH(
                minilua::string::sub(ctx),
                Contains("bad argument #2") && Contains("number expected, got string"));

            ctx = ctx.make_new({"hallo", minilua::Nil(), 2});

            CHECK_THROWS_WITH(
                minilua::string::sub(ctx),
                Contains("bad argument #2") && Contains("number expected, got nil"));
        }

        SECTION("i is not in integer format") {
            ctx = ctx.make_new({"hallo", 1.5, 2});

            CHECK_THROWS_WITH(
                minilua::string::sub(ctx),
                Contains("bad argument #2") && Contains("number has no integer representation"));

            ctx = ctx.make_new({"hallo", -1.5, -2});

            CHECK_THROWS_WITH(
                minilua::string::sub(ctx),
                Contains("bad argument #2") && Contains("number has no integer representation"));
        }

        SECTION("j is not a number") {
            ctx = ctx.make_new({"hallo", 2, false});

            CHECK_THROWS_WITH(
                minilua::string::sub(ctx),
                Contains("bad argument #3") && Contains("number expected, got boolean"));

            ctx = ctx.make_new({"hallo", 2, minilua::Table()});

            CHECK_THROWS_WITH(
                minilua::string::sub(ctx),
                Contains("bad argument #3") && Contains("number expected, got table"));

            ctx = ctx.make_new({"hallo", 2, "minilua::Table"});

            CHECK_THROWS_WITH(
                minilua::string::sub(ctx),
                Contains("bad argument #3") && Contains("number expected, got string"));
        }

        SECTION("j is not in integer format") {
            ctx = ctx.make_new({"hallo", 2, 3.67});

            CHECK_THROWS_WITH(
                minilua::string::sub(ctx),
                Contains("bad argument #3") && Contains("number has no integer representation"));

            ctx = ctx.make_new({"hallo", 3, -4.665});

            CHECK_THROWS_WITH(
                minilua::string::sub(ctx),
                Contains("bad argument #3") && Contains("number has no integer representation"));
        }
    }

    SECTION("REVERSE") {

        ctx = ctx.make_new(
            {minilua::Value("Maus").with_origin(minilua::LiteralOrigin()),
             minilua::Value(2).with_origin(minilua::LiteralOrigin())});

        SECTION("Valid force") {
            auto test_function = [&ctx](
                                     const std::string& original_s, const auto& i, const auto& j,
                                     const std::string& expected_result,
                                     const std::string& force_to,
                                     const std::string& expected_new_s) {
                ctx = ctx.make_new(
                    {minilua::Value(original_s).with_origin(minilua::LiteralOrigin()),
                     minilua::Value(i).with_origin(minilua::LiteralOrigin()),
                     minilua::Value(j).with_origin(minilua::LiteralOrigin())});
                auto res = minilua::string::sub(ctx);

                REQUIRE(res == expected_result);

                auto result = res.force(force_to);

                REQUIRE(result.has_value());
                std::string expected = minilua::Value(expected_new_s).to_literal();
                CHECK(
                    result.value().collect_first_alternative()[0] ==
                    minilua::SourceChange(minilua::Range(), expected));
            };

            test_function("Maus", 2, minilua::Nil(), "aus", "ail", "Mail");

            test_function("Megamaus", -6, -3, "gama", "Baum", "MeBaumus");
        }

        SECTION("Invalid force") {
            auto test_function = [&ctx](
                                     const std::string& original_s, const auto& i, const auto& j,
                                     const std::string& expected_result, const auto& force_to) {
                ctx = ctx.make_new(
                    {minilua::Value(original_s).with_origin(minilua::LiteralOrigin()),
                     minilua::Value(i).with_origin(minilua::LiteralOrigin()),
                     minilua::Value(j).with_origin(minilua::LiteralOrigin())});
                auto res = minilua::string::sub(ctx);

                REQUIRE(res == expected_result);

                CHECK_FALSE(res.force(force_to).has_value());
            };

            test_function("Maus", 2, 5, "aus", "Baum");

            test_function("Maus", 5, 7, "", 42);
        }
    }
}

TEST_CASE("string.upper") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);
    auto test_function = [&ctx](const auto& str, const std::string& expected) {
        ctx = ctx.make_new({str});
        auto result = minilua::string::upper(ctx);

        CHECK(result == minilua::Value(expected));
    };

    SECTION("String") {
        test_function("HALLO", "HALLO");

        test_function("hallo", "HALLO");

        test_function("WeLt!", "WELT!");

        test_function("", "");

        test_function("🙂", "🙂");

        test_function("!§$%&/()=?*'_:;", "!§$%&/()=?*'_:;");
    }

    SECTION("Number") {
        test_function(12345, "12345");

        test_function(-5, "-5");

        test_function(-3.56, "-3.56");
    }

    SECTION("Invalid Input") {
        ctx = ctx.make_new({true});
        CHECK_THROWS_WITH(
            minilua::string::upper(ctx),
            Contains("bad argument #1") && Contains("string expected, got boolean"));

        ctx = ctx.make_new({minilua::Nil()});
        CHECK_THROWS_WITH(
            minilua::string::upper(ctx),
            Contains("bad argument #1") && Contains("string expected, got nil"));
    }
}