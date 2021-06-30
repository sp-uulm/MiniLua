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

TEST_CASE("table.concat(list [, sep [, i [, j]]])") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

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
    }
}