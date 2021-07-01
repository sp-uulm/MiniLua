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
}

TEST_CASE("table.insert(list [,pos], value)") {
    SECTION("All elements of the table are between 1 and #table") {
        std::unordered_map<minilua::Value, minilua::Value> map = {
            {1, "Hallo"}, {2, "Welt"}, {3, "!"}, {4, "Minilua"}, {5, "Universität"}};
        minilua::Table table(map);
    }
}