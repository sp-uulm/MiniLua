#include <catch2/catch.hpp>
#include <string>

#include "MiniLua/math.hpp"
#include "MiniLua/values.hpp"

TEST_CASE("math.abs(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Integer") {
        SECTION("positive") {
            int i = 42;
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::abs(ctx) == i);
        }

        SECTION("negative") {
            int i = -42;
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::abs(ctx) == -i);
        }
    }

    SECTION("Double") {
        SECTION("positive") {
            double i = 42.5;
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::abs(ctx) == i);
        }

        SECTION("negative") {
            double i = -42.5;
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::abs(ctx) == -i);
        }
    }

    SECTION("String") {
        SECTION("positive Integer as string") {
            std::string i = "42";
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::abs(ctx) == 42);
        }

        SECTION("negative Integer as string") {
            std::string i = "-42";
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::abs(ctx) == 42);
        }

        SECTION("positive Double as string") {
            std::string i = "42.5";
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::abs(ctx) == 42.5);
        }

        SECTION("negative Double as string") {
            std::string i = "-42.5";
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::abs(ctx) == 42.5);
        }

        SECTION("non-number string") {
            std::string i = "baum";
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::abs(ctx), "bad argument #1 to abs (number expected, got string)");
        }
    }
}

TEST_CASE("math.acos(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Integer") {
        SECTION("x is not in [-1, 1]") {
            int x = -2;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::acos(ctx) == minilua::Value("nan"));

            x = 2;
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::acos(ctx) == minilua::Value("nan"));
        }

        SECTION("x is in [-1, 1]") {
            double x = -0.5;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            minilua::Number n = std::get<minilua::Number>(minilua::math::acos(ctx));
            CHECK(n.value == Approx(2.0944));

            x = 1;
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::acos(ctx) == minilua::Value(0));
        }
    }

    SECTION("String") {
        SECTION("x is not in [-1, 1]") {
            std::string x = "-2";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::acos(ctx) == minilua::Value("nan"));

            x = "2";
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::acos(ctx) == minilua::Value("nan"));
        }

        SECTION("x is in [-1, 1]") {
            std::string x = "-0.5";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            minilua::Number n = std::get<minilua::Number>(minilua::math::acos(ctx));
            CHECK(n.value == Approx(2.0943951023932));

            x = "1";
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::acos(ctx) == minilua::Value(0));
        }
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::acos(ctx), "bad argument #1 to abs (number expected, got string)");
    }
}

TEST_CASE("math.asin(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Integer") {
        SECTION("x is not in [-1, 1]") {
            int x = -2;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::asin(ctx) == minilua::Value("nan"));

            x = 2;
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::asin(ctx) == minilua::Value("nan"));
        }

        SECTION("x is in [-1, 1]") {
            double x = -0.5;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            minilua::Number n = std::get<minilua::Number>(minilua::math::asin(ctx));
            CHECK(n.value == Approx(-0.5235987755983));

            x = 0;
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::asin(ctx) == minilua::Value(0));

            x = 1;
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            n = std::get<minilua::Number>(minilua::math::asin(ctx));
            CHECK(n.value == Approx(1.5707963267949));
        }
    }

    SECTION("String") {
        SECTION("x is not in [-1, 1]") {
            std::string x = "-2";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::asin(ctx) == minilua::Value("nan"));

            x = "2";
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::asin(ctx) == minilua::Value("nan"));
        }

        SECTION("x is in [-1, 1]") {
            std::string x = "-0.5";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            minilua::Number n = std::get<minilua::Number>(minilua::math::asin(ctx));
            CHECK(n.value == Approx(-0.5235987755983));

            x = "0";
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::asin(ctx) == minilua::Value(0));

            x = "1";
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            n = std::get<minilua::Number>(minilua::math::asin(ctx));
            CHECK(n.value == Approx(1.5707963267949));
        }
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::asin(ctx), "bad argument #1 to abs (number expected, got string)");
    }
}

TEST_CASE("math.ceil(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Integer") {}

    SECTION("String") {}

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::asin(ctx), "bad argument #1 to abs (number expected, got string)");
    }
}