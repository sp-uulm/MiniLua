#include <catch2/catch.hpp>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <list>
#include <random>
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

        j = -3;
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

        i = 3;
        j = -8;
        testFunction(s, i, j, 0, {minilua::Nil()});
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

            for (int i = 0; i < source_changes.size(); ++i) {
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
        CHECK_THROWS_WITH(minilua::string::len(ctx), Contains("bad argument #1") && Contains("string expected, got bool"));

        ctx = ctx.make_new({minilua::Table()});
        CHECK_THROWS_WITH(minilua::string::len(ctx), Contains("bad argument #1") && Contains("string expected, got table"));

        ctx = ctx.make_new({minilua::Nil()});
        CHECK_THROWS_WITH(minilua::string::len(ctx), Contains("bad argument #1") && Contains("string expected, got nil"));
    }
}