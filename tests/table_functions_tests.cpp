#include <catch2/catch.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <list>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "MiniLua/environment.hpp"
#include "MiniLua/source_change.hpp"
#include "MiniLua/table_functions.hpp"
#include "MiniLua/values.hpp"

using Catch::Matchers::Contains;

minilua::Environment env;
minilua::CallContext ctx(&env);

TEST_CASE("table.concat(list [, sep [, i [, j]]])") {
    SECTION("all elements are between 1 and #table") {
        std::unordered_map<minilua::Value, minilua::Value> map = {
            {1, "Hallo"}, {2, "Welt"}, {3, "!"}, {4, "Minilua"}, {5, "Universität"}};
        minilua::Table table(map);

        SECTION("no optional parameters") {
            ctx = ctx.make_new({table});

            CHECK(minilua::table::concat(ctx) == "HalloWelt!MiniluaUniversität");
        }

        SECTION("with separator") {
            minilua::Value sep = " ";
            ctx = ctx.make_new({table, sep});

            CHECK(minilua::table::concat(ctx) == "Hallo Welt ! Minilua Universität");
        }

        SECTION("with separator and start-value") {
            minilua::Value sep = " ";
            minilua::Value i = 3;
            ctx = ctx.make_new({table, sep, i});

            CHECK(minilua::table::concat(ctx) == "! Minilua Universität");
        }

        SECTION("with all optional parameters") {
            minilua::Value sep = " ";
            minilua::Value i = 3;
            minilua::Value j = 4;
            ctx = ctx.make_new({table, sep, i, j});

            CHECK(minilua::table::concat(ctx) == "! Minilua");
        }
    }

    SECTION("some elements are outside of 1 and #table") {
        std::unordered_map<minilua::Value, minilua::Value> map = {
            {1, "Hallo"},
            {2, "Welt"},
            {3, "!"},
            {4, "Minilua"},
            {5, "Universität"},
            {7, "Essen"},
            {"Programmieren", "Lua"}};
        minilua::Table table(map);

        SECTION("no optional parameters") {
            ctx = ctx.make_new({table});

            CHECK(minilua::table::concat(ctx) == "HalloWelt!MiniluaUniversität");
        }

        SECTION("with separator") {
            minilua::Value sep = " ";
            ctx = ctx.make_new({table, sep});

            CHECK(minilua::table::concat(ctx) == "Hallo Welt ! Minilua Universität");

            sep = 1;
            ctx = ctx.make_new({table, sep});

            CHECK(minilua::table::concat(ctx) == "Hallo1Welt1!1Minilua1Universität");
        }

        SECTION("with separator and start-value") {
            minilua::Value sep = " ";
            minilua::Value i = 3;
            ctx = ctx.make_new({table, sep, i});

            CHECK(minilua::table::concat(ctx) == "! Minilua Universität");

            i = "3";
            ctx = ctx.make_new({table, sep, i});

            CHECK(minilua::table::concat(ctx) == "! Minilua Universität");
        }

        SECTION("with all optional parameters") {
            minilua::Value sep = " ";
            minilua::Value i = 3;
            minilua::Value j = 4;
            ctx = ctx.make_new({table, sep, i, j});

            CHECK(minilua::table::concat(ctx) == "! Minilua");

            i = "3";
            j = 4;
            ctx = ctx.make_new({table, sep, i, j});

            CHECK(minilua::table::concat(ctx) == "! Minilua");

            i = 3;
            j = "4";
            ctx = ctx.make_new({table, sep, i, j});

            CHECK(minilua::table::concat(ctx) == "! Minilua");

            i = "3";
            j = "4";
            ctx = ctx.make_new({table, sep, i, j});

            CHECK(minilua::table::concat(ctx) == "! Minilua");
        }
    }

    SECTION("Incorrect inputs") {
        SECTION("Valid table, invalid inputs") {
            std::unordered_map<minilua::Value, minilua::Value> map = {
                {1, "Hallo"}, {2, "Welt"}, {3, "!"}, {4, "Minilua"}, {5, "Universität"}};
            minilua::Table table(map);

            ctx = ctx.make_new({2});
            CHECK_THROWS_WITH(
                minilua::table::concat(ctx),
                Contains("bad argument #1 to 'concat'") && Contains("table expected"));

            ctx = ctx.make_new({table, true});
            CHECK_THROWS_WITH(
                minilua::table::concat(ctx),
                Contains("bad argument #2 to 'concat'") && Contains("string expected"));

            ctx = ctx.make_new({table, " ", "welt"});
            CHECK_THROWS_WITH(
                minilua::table::concat(ctx),
                Contains("bad argument #3 to 'concat'") && Contains("number expected"));
            ctx = ctx.make_new({table, " ", true});
            CHECK_THROWS_WITH(
                minilua::table::concat(ctx),
                Contains("bad argument #3 to 'concat'") && Contains("number expected"));

            ctx = ctx.make_new({table, " ", 3, "welt"});
            CHECK_THROWS_WITH(
                minilua::table::concat(ctx),
                Contains("bad argument #4 to 'concat'") && Contains("number expected"));
            ctx = ctx.make_new({table, " ", 3, true});
            CHECK_THROWS_WITH(
                minilua::table::concat(ctx),
                Contains("bad argument #4 to 'concat'") && Contains("number expected"));
        }

        SECTION("Invalid table, valid input") {
            std::unordered_map<minilua::Value, minilua::Value> map = {
                {1, "Hallo"}, {2, "Welt"}, {3, true}, {4, false}, {5, "Universität"}};
            minilua::Table table(map);

            ctx = ctx.make_new({table, " ", 2});
            CHECK_THROWS_WITH(
                minilua::table::concat(ctx),
                Contains("Invalid value") && Contains("in table for 'concat'!"));
        }
    }
}

TEST_CASE("table.insert(list [,pos], value)") {
    SECTION("All elements of the table are between 1 and #table") {
        std::unordered_map<minilua::Value, minilua::Value> map = {
            {1, "Hallo"}, {2, "Welt"}, {3, "!"}, {4, "Minilua"}, {5, "Universität"}};
        minilua::Table table(map);

        SECTION("No optional parameters") {
            ctx = ctx.make_new({table, minilua::Nil(), 42});

            minilua::table::insert(ctx);
            CHECK(table.has(6));
            CHECK(table.get(6) == 42);
        }

        SECTION("Insert between the elements of table") {
            ctx = ctx.make_new({table, 3, "code"});

            minilua::table::insert(ctx);
            CHECK(table.has(6));
            CHECK(table.get(3) == "code");
        }

        SECTION("Insert between the elements of table with number-formated string") {
            ctx = ctx.make_new({table, "3", "code"});

            minilua::table::insert(ctx);
            CHECK(table.has(6));
            CHECK(table.get(3) == "code");
        }
    }

    SECTION("invalid input") {
        std::unordered_map<minilua::Value, minilua::Value> map = {
            {1, "Hallo"}, {2, "Welt"}, {3, "!"}, {4, "Minilua"}, {5, "Universität"}};
        minilua::Table table(map);

        SECTION("Insert outside of the border") {
            ctx = ctx.make_new({table, 100, 43});

            CHECK_THROWS_WITH(
                minilua::table::insert(ctx), Contains("#2") && Contains("position out of bounds"));

            ctx = ctx.make_new({table, 0, 43});

            CHECK_THROWS_WITH(
                minilua::table::insert(ctx), Contains("#2") && Contains("position out of bounds"));

            ctx = ctx.make_new({table, -100, 43});

            CHECK_THROWS_WITH(
                minilua::table::insert(ctx), Contains("#2") && Contains("position out of bounds"));
        }

        SECTION("Insert at a non-number-position") {
            ctx = ctx.make_new({table, "lua", 43});

            CHECK_THROWS_WITH(
                minilua::table::insert(ctx),
                Contains("bad argument #2") && Contains("number expected"));
        }

        SECTION("Call function with no value to insert") {
            ctx = ctx.make_new({table});

            CHECK_THROWS_WITH(minilua::table::insert(ctx), Contains("wrong number of arguments"));
        }

        SECTION("first argument isn't a table") {
            ctx = ctx.make_new({42, minilua::Nil(), 234});

            CHECK_THROWS_WITH(
                minilua::table::insert(ctx), Contains("argument #1") && Contains("table expected"));
        }
    }
}