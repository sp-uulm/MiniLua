#include <catch2/catch.hpp>
#include <cmath>
#include <string>

#include "MiniLua/environment.hpp"
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
            minilua::math::acos(ctx), "bad argument #1 to acos (number expected, got string)");
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
            minilua::math::asin(ctx), "bad argument #1 to asin (number expected, got string)");
    }
}

TEST_CASE("math.atan(x [, y]") {
    // not implemented yet
}

TEST_CASE("math.ceil(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        SECTION("Integer") {
            int i = 42;
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::ceil(ctx) == minilua::Value(i));

            i = 0;
            list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::ceil(ctx) == minilua::Value(i));

            i = -982;
            list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::ceil(ctx) == minilua::Value(i));
        }

        SECTION("Double") {
            double i = 42.5;
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::ceil(ctx) == minilua::Value(43));

            i = -1.9;
            list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::ceil(ctx) == minilua::Value(-1));
        }
    }

    SECTION("String") {
        std::string i = "42";
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::ceil(ctx) == minilua::Value(42));

        i = "0";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::ceil(ctx) == minilua::Value(0));

        i = "-982";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::ceil(ctx) == minilua::Value(-982));

        i = "42.5";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::ceil(ctx) == minilua::Value(43));

        i = "-1.9";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::ceil(ctx) == minilua::Value(-1));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::ceil(ctx), "bad argument #1 to ceil (number expected, got string)");
    }
}

TEST_CASE("math.cos(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        int i = 0;
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::cos(ctx) == minilua::Value(1));

        i = 1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::cos(ctx));
        CHECK(n.value == Approx(0.54030230586814));

        i = -1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::cos(ctx));
        CHECK(n.value == Approx(0.54030230586814));

        double d = boost::math::constants::pi<double>();
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::cos(ctx) == minilua::Value(-1));
    }

    SECTION("Strings") {
        std::string i = "0";
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::cos(ctx) == minilua::Value(1));

        i = "1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::cos(ctx));
        CHECK(n.value == Approx(0.54030230586814));

        i = "-1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::cos(ctx));
        CHECK(n.value == Approx(0.54030230586814));

        std::string d = std::to_string(boost::math::constants::pi<double>());
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::cos(ctx));
        CHECK(n.value == Approx(-1));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::cos(ctx), "bad argument #1 to cos (number expected, got string)");
    }
}

TEST_CASE("math.deg(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        int i = 0;
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::deg(ctx) == minilua::Value(0));

        i = 1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::deg(ctx));
        CHECK(n.value == Approx(57.295779513082));

        i = -1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::deg(ctx));
        CHECK(n.value == Approx(-57.295779513082));

        double d = boost::math::constants::pi<double>();
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::deg(ctx));
        CHECK(n.value == Approx(180));
    }

    SECTION("Strings") {
        std::string i = "0";
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::deg(ctx) == minilua::Value(0));

        i = "1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::deg(ctx));
        CHECK(n.value == Approx(57.295779513082));

        i = "-1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::deg(ctx));
        CHECK(n.value == Approx(-57.295779513082));

        std::string d = std::to_string(boost::math::constants::pi<double>());
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::deg(ctx));
        CHECK(n.value == Approx(180));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::deg(ctx), "bad argument #1 to deg (number expected, got string)");
    }
}

TEST_CASE("math.exp(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        int i = 0;
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::exp(ctx) == minilua::Value(1));

        i = 1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.value == Approx(2.718281828459));

        i = 2;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.value == Approx(7.3890560989307));

        i = -1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.value == Approx(0.36787944117144));

        double d = 0.5;
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.value == Approx(1.6487212707001));

        int x = 20;
        d = std::log(x);
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.value == Approx(x));
    }

    SECTION("Strings") {
        std::string i = "0";
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::exp(ctx) == minilua::Value(1));

        i = "1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.value == Approx(2.718281828459));

        i = "2";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.value == Approx(7.3890560989307));

        i = "-1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.value == Approx(0.36787944117144));

        i = "0.5";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.value == Approx(1.6487212707001));

        int x = 20;
        i = std::to_string(std::log(x));
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.value == Approx(x));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::exp(ctx), "bad argument #1 to exp (number expected, got string)");
    }
}