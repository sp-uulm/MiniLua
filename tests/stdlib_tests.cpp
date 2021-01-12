#include <catch2/catch.hpp>
#include <string>

#include "MiniLua/stdlib.hpp"
#include "MiniLua/values.hpp"

TEST_CASE("to_number") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Base = Nill") {
        SECTION("Number to Number") {
            int i = 42;
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::to_number(ctx) == minilua::Value(i));
        }

        SECTION("Hex-String") {
            std::string s = "0X083ad.1";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
            ctx = ctx.make_new(list);
            CHECK(minilua::to_number(ctx) == minilua::Value(33709.0625));
        }

        SECTION("Number-String with potent") {
            std::string s = "2.25324e4";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
            ctx = ctx.make_new(list);
            CHECK(minilua::to_number(ctx) == minilua::Value(22532.4));
        }

        SECTION("Number-String with negative potent") {
            std::string s = "11230.e-2";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
            ctx = ctx.make_new(list);
            CHECK(minilua::to_number(ctx) == minilua::Value(112.3));
        }

        SECTION("Number as String") {
            std::string s = "42";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
            ctx = ctx.make_new(list);
            CHECK(minilua::to_number(ctx) == minilua::Value(42));
        }
    }

    SECTION("Base != Nil") {}
}

TEST_CASE("assert_lua") {
    SECTION("Assert fails with default message") {
        minilua::Environment env;
        minilua::CallContext ctx(&env);
        minilua::Vallist list = minilua::Vallist({minilua::Value{false}, minilua::Nil()});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(minilua::assert_lua(ctx), "assertion failed");
    }

    SECTION("Assert fails with default message") {
        std::string s = "Hallo Welt!";
        minilua::Environment env;
        minilua::CallContext ctx(&env);
        minilua::Vallist list = minilua::Vallist({minilua::Value{false}, minilua::Value{s}});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(minilua::assert_lua(ctx), s);
    }

    SECTION("Assert passes") {
        SECTION("Assert passes with standard true") {
            minilua::Environment env;
            minilua::CallContext ctx(&env);
            minilua::Vallist list = minilua::Vallist({minilua::Value{true}, minilua::Value{42}});
            ctx = ctx.make_new(list);
            CHECK(minilua::assert_lua(ctx) == ctx.arguments());
        }

        SECTION("Assert passes with converted true") {
            minilua::Environment env;
            minilua::CallContext ctx(&env);
            minilua::Vallist list =
                minilua::Vallist({minilua::Value{42}, minilua::Value{"Hallo Welt!"}});
            ctx = ctx.make_new(list);
            CHECK(minilua::assert_lua(ctx) == ctx.arguments());
        }
    }
}

TEST_CASE("select") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("get last element (index = -1)") {
        int end = 100;
        minilua::Vallist list = minilua::Vallist(
            {minilua::Value{-1}, minilua::Value{"Hallo Welt!"}, minilua::Value{75},
             minilua::Value{end}});
        ctx = ctx.make_new(list);
        CHECK(minilua::select(ctx) == minilua::Vallist{end});
    }

    SECTION("Get last 3 elements (index = -3)") {
        minilua::Vallist list = minilua::Vallist(
            {minilua::Value{-3}, minilua::Value{"Hallo Welt!"}, minilua::Value{75},
             minilua::Value{100}, minilua::Value{5}, minilua::Value{6}, minilua::Value{7}});
        ctx = ctx.make_new(list);
        CHECK(
            minilua::select(ctx) ==
            minilua::Vallist{minilua::Value{5}, minilua::Value{6}, minilua::Value{7}});
    }

    SECTION("get all elements") {
        minilua::Vallist list = minilua::Vallist(
            {minilua::Value{1}, minilua::Value{"Hallo Welt!"}, minilua::Value{75},
             minilua::Value{100}});
        ctx = ctx.make_new(list);
        CHECK(
            minilua::select(ctx) ==
            minilua::Vallist(
                {minilua::Value{"Hallo Welt!"}, minilua::Value{75}, minilua::Value{100}}));
    }

    SECTION("get amount of arguments") {
        minilua::Vallist list = minilua::Vallist(
            {minilua::Value{"#"}, minilua::Value{"Hallo Welt!"}, minilua::Value{75},
             minilua::Value{100}});
        ctx = ctx.make_new(list);
        int size = list.size() - 1;

        CHECK(minilua::select(ctx) == minilua::Vallist{size});
    }

    SECTION("index extends size of list") {
        minilua::Vallist list = minilua::Vallist(
            {minilua::Value{100}, minilua::Value{"Hallo Welt!"}, minilua::Value{75},
             minilua::Value{100}});
        ctx = ctx.make_new(list);

        CHECK(minilua::select(ctx) == minilua::Vallist());
    }

    SECTION("fails") {
        SECTION("index out of range") {
            SECTION("index = 0") {
                minilua::Vallist list = minilua::Vallist(
                    {minilua::Value{0}, minilua::Value{"Hallo Welt!"}, minilua::Value{75},
                     minilua::Value{100}});
                ctx = ctx.make_new(list);

                CHECK_THROWS_WITH(
                    minilua::select(ctx), "bad argument #1 to 'select' (index out of range)");
            }

            SECTION("negative index that is bigger than listsize") {
                minilua::Vallist list = minilua::Vallist(
                    {minilua::Value{-100}, minilua::Value{"Hallo Welt!"}, minilua::Value{75},
                     minilua::Value{100}});
                ctx = ctx.make_new(list);

                CHECK_THROWS_WITH(
                    minilua::select(ctx), "bad argument #1 to 'select' (index out of range)");
            }
        }

        SECTION("invalid string") {
            minilua::Vallist list = minilua::Vallist(
                {minilua::Value{"Baum"}, minilua::Value{"Hallo Welt!"}, minilua::Value{75},
                 minilua::Value{100}});
            ctx = ctx.make_new(list);

            CHECK_THROWS_WITH(
                minilua::select(ctx), "bad argument #1 to 'select' (number expected, got string)");
        }

        SECTION("invalid index") {
            minilua::Value a{true};
            minilua::Vallist list = minilua::Vallist(
                {minilua::Value{true}, minilua::Value{"Hallo Welt!"}, minilua::Value{75},
                 minilua::Value{100}});
            ctx = ctx.make_new(list);

            CHECK_THROWS_WITH(
                minilua::select(ctx),
                "bad argument #1 to 'select' (number expected, got " + a.type() + ")");
        }
    }
}