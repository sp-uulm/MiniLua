#include <catch2/catch.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <list>
#include <random>
#include <string>
#include <vector>

#include "MiniLua/environment.hpp"
#include "MiniLua/string.hpp"
#include "MiniLua/source_change.hpp"
#include "MiniLua/values.hpp"

using Catch::Matchers::Contains;

TEST_CASE("string.byte"){
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("String, Nil, Nil") {
        auto testFunction = [&ctx](std::string str, int expectedResult){
            ctx = ctx.make_new({str});
            auto resultList = minilua::string::byte(ctx);
            CHECK(resultList.get(0) == minilua::Value(expectedResult) && resultList.size() == 1);
        };
        std::string s = "a";
        testFunction(s, 97);

        s = "Hallo";
        testFunction(s, 72);

        s = "\n";
        testFunction(s, 10);
    }

    SECTION("Number, Nil, Nil") {
        auto testFunction = [&ctx](auto str, int expectedResult){
            ctx = ctx.make_new({str});
            auto resultList = minilua::string::byte(ctx);
            CHECK(resultList.get(0) == minilua::Value(expectedResult) && resultList.size() == 1);
        };
        int s = 1;
        testFunction(s, 49);

        s = 10;
        testFunction(s, 49);

        double new_s = 1.5;
        testFunction(s, 49);
    }

    SECTION("String, Number, Nil") {
        auto testFunction = [&ctx](auto str, auto i,  int expectedResult){
            ctx = ctx.make_new({str, i});
            auto resultList = minilua::string::byte(ctx);
            CHECK(resultList.get(0) == minilua::Value(expectedResult) && resultList.size() == 1);
        };

        std::string s = "Hallo";
        int i = 0;
    }
}