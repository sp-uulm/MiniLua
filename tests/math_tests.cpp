#include <catch2/catch.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <list>
#include <random>
#include <string>
#include <vector>

#include "MiniLua/environment.hpp"
#include "MiniLua/math.hpp"
#include "MiniLua/source_change.hpp"
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
                minilua::math::abs(ctx), "bad argument #1 to 'abs' (number expected, got string)");
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
            minilua::Number n = std::get<minilua::Number>(minilua::math::acos(ctx));
            CHECK(std::isnan(n.as_float()));

            x = 2;
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            n = std::get<minilua::Number>(minilua::math::acos(ctx));
            CHECK(std::isnan(n.as_float()));
        }

        SECTION("x is in [-1, 1]") {
            double x = -0.5;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            minilua::Number n = std::get<minilua::Number>(minilua::math::acos(ctx));
            CHECK(n.as_float() == Approx(2.0944));

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
            minilua::Number n = std::get<minilua::Number>(minilua::math::acos(ctx));
            CHECK(std::isnan(n.as_float()));

            x = "2";
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            n = std::get<minilua::Number>(minilua::math::acos(ctx));
            CHECK(std::isnan(n.as_float()));
        }

        SECTION("x is in [-1, 1]") {
            std::string x = "-0.5";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            minilua::Number n = std::get<minilua::Number>(minilua::math::acos(ctx));
            CHECK(n.as_float() == Approx(2.0943951023932));

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
            minilua::math::acos(ctx), "bad argument #1 to 'acos' (number expected, got string)");
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
            minilua::Number n = std::get<minilua::Number>(minilua::math::asin(ctx));
            CHECK(std::isnan(n.as_float()));

            x = 2;
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            n = std::get<minilua::Number>(minilua::math::acos(ctx));
            CHECK(std::isnan(n.as_float()));
        }

        SECTION("x is in [-1, 1]") {
            double x = -0.5;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            minilua::Number n = std::get<minilua::Number>(minilua::math::asin(ctx));
            CHECK(n.as_float() == Approx(-0.5235987755983));

            x = 0;
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::asin(ctx) == minilua::Value(0));

            x = 1;
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            n = std::get<minilua::Number>(minilua::math::asin(ctx));
            CHECK(n.as_float() == Approx(1.5707963267949));
        }
    }

    SECTION("String") {
        SECTION("x is not in [-1, 1]") {
            std::string x = "-2";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            minilua::Number n = std::get<minilua::Number>(minilua::math::asin(ctx));
            CHECK(std::isnan(n.as_float()));

            x = "2";
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            n = std::get<minilua::Number>(minilua::math::asin(ctx));
            CHECK(std::isnan(n.as_float()));
        }

        SECTION("x is in [-1, 1]") {
            std::string x = "-0.5";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            minilua::Number n = std::get<minilua::Number>(minilua::math::asin(ctx));
            CHECK(n.as_float() == Approx(-0.5235987755983));

            x = "0";
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::asin(ctx) == minilua::Value(0));

            x = "1";
            list = minilua::Vallist({minilua::Value(x)});
            ctx = ctx.make_new(list);
            n = std::get<minilua::Number>(minilua::math::asin(ctx));
            CHECK(n.as_float() == Approx(1.5707963267949));
        }
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::asin(ctx), "bad argument #1 to 'asin' (number expected, got string)");
    }
}

TEST_CASE("math.atan(x [, y]") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Number, Number") {
        int x = 1;
        int y = 2;
        minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
        ctx = ctx.make_new(list);
        auto n = std::get<minilua::Number>(minilua::math::atan(ctx));
        CHECK(n.as_float() == Approx(0.46364760900081));
    }

    SECTION("Number, Nil") {
        int x = 1;
        minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Nil()});
        ctx = ctx.make_new(list);
        auto n = std::get<minilua::Number>(minilua::math::atan(ctx));
        CHECK(n.as_float() == Approx(0.78539816339745));
    }

    SECTION("Number, String") {
        SECTION("Valid String") {
            int x = 1;
            std::string y = "2";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            auto n = std::get<minilua::Number>(minilua::math::atan(ctx));
            CHECK(n.as_float() == Approx(0.46364760900081));
        }

        SECTION("Invalid String") {
            int x = 1;
            std::string y = "Minilua";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::atan(ctx),
                "bad argument #2 to 'atan' (number expected, got string)");
        }
    }

    SECTION("Number, Bool") {
        int x = 1;
        bool y = false;
        minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::atan(ctx), "bad argument #2 to 'atan' (number expected, got boolean)");
    }

    SECTION("String, Number") {
        SECTION("Valid String") {
            std::string x = "1";
            int y = 2;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            auto n = std::get<minilua::Number>(minilua::math::atan(ctx));
            CHECK(n.as_float() == Approx(0.46364760900081));
        }

        SECTION("Invalid String") {
            std::string x = "Baum";
            int y = 2;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::atan(ctx),
                "bad argument #1 to 'atan' (number expected, got string)");
        }
    }

    SECTION("String, Nil") {
        SECTION("Valid String") {
            std::string s = "1";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s), minilua::Nil()});
            ctx = ctx.make_new(list);
            auto n = std::get<minilua::Number>(minilua::math::atan(ctx));
            CHECK(n.as_float() == Approx(0.78539816339745));
        }

        SECTION("Invalid String") {
            std::string s = "Baum";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s), minilua::Nil()});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::atan(ctx),
                "bad argument #1 to 'atan' (number expected, got string)");
        }
    }

    SECTION("String, String") {
        SECTION("Valid String, Valid String") {
            std::string s = "1";
            std::string i = "2";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s), minilua::Value(i)});
            ctx = ctx.make_new(list);
            auto n = std::get<minilua::Number>(minilua::math::atan(ctx));
            CHECK(n.as_float() == Approx(0.46364760900081));
        }

        SECTION("Valid String, Invalid String") {
            std::string s = "1";
            std::string i = "Minilua";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s), minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::atan(ctx),
                "bad argument #2 to 'atan' (number expected, got string)");
        }

        SECTION("Invalid String, Valid String") {
            std::string i = "Minilua";
            std::string s = "1";
            minilua::Vallist list = minilua::Vallist({minilua::Value(i), minilua::Value(s)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::atan(ctx),
                "bad argument #1 to 'atan' (number expected, got string)");
        }

        SECTION("Invalid String, Invalid String") {
            std::string i = "Minilua";
            std::string s = "Baum";
            minilua::Vallist list = minilua::Vallist({minilua::Value(i), minilua::Value(s)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::atan(ctx),
                "bad argument #1 to 'atan' (number expected, got string)");
        }
    }

    SECTION("String, Bool") {
        SECTION("Valid String") {
            std::string x = "1";
            bool y = true;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::atan(ctx),
                "bad argument #2 to 'atan' (number expected, got boolean)");
        }

        SECTION("Invalid String") {
            std::string x = "Baum";
            bool y = true;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::atan(ctx),
                "bad argument #1 to 'atan' (number expected, got string)");
        }
    }

    SECTION("y = 0") {
        SECTION("y is Number") {
            int x = 1;
            int y = 0;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            auto n = std::get<minilua::Number>(minilua::math::atan(ctx));
            CHECK(n.as_float() == Approx(1.5707963267949));
        }

        SECTION("y is String") {
            int x = 1;
            std::string y = "0";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            auto n = std::get<minilua::Number>(minilua::math::atan(ctx));
            CHECK(n.as_float() == Approx(1.5707963267949));
        }
    }

    SECTION("boolean, boolean") {
        bool b = true;
        bool a = false;
        minilua::Vallist list({a, b});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::atan(ctx), "bad argument #1 to 'atan' (number expected, got boolean)");
    }

    SECTION("invalid input") {
        SECTION("invalid input") {
            std::string s = "Minilua";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s), minilua::Nil()});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::atan(ctx),
                "bad argument #1 to 'atan' (number expected, got string)");
        }
    }
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
            minilua::math::ceil(ctx), "bad argument #1 to 'ceil' (number expected, got string)");
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
        CHECK(n.as_float() == Approx(0.54030230586814));

        i = -1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::cos(ctx));
        CHECK(n.as_float() == Approx(0.54030230586814));

        double d = minilua::math::PI;
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
        CHECK(n.as_float() == Approx(0.54030230586814));

        i = "-1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::cos(ctx));
        CHECK(n.as_float() == Approx(0.54030230586814));

        std::string d = std::to_string(minilua::math::PI);
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::cos(ctx));
        CHECK(n.as_float() == Approx(-1));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::cos(ctx), "bad argument #1 to 'cos' (number expected, got string)");
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
        CHECK(n.as_float() == Approx(57.295779513082));

        i = -1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::deg(ctx));
        CHECK(n.as_float() == Approx(-57.295779513082));

        double d = minilua::math::PI;
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::deg(ctx));
        CHECK(n.as_float() == Approx(180));
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
        CHECK(n.as_float() == Approx(57.295779513082));

        i = "-1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::deg(ctx));
        CHECK(n.as_float() == Approx(-57.295779513082));

        std::string d = std::to_string(minilua::math::PI);
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::deg(ctx));
        CHECK(n.as_float() == Approx(180));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::deg(ctx), "bad argument #1 to 'deg' (number expected, got string)");
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
        CHECK(n.as_float() == Approx(2.718281828459));

        i = 2;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.as_float() == Approx(7.3890560989307));

        i = -1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.as_float() == Approx(0.36787944117144));

        double d = 0.5;
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.as_float() == Approx(1.6487212707001));

        int x = 20;
        d = std::log(x);
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.as_float() == Approx(x));
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
        CHECK(n.as_float() == Approx(2.718281828459));

        i = "2";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.as_float() == Approx(7.3890560989307));

        i = "-1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.as_float() == Approx(0.36787944117144));

        i = "0.5";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.as_float() == Approx(1.6487212707001));

        int x = 20;
        i = std::to_string(std::log(x));
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::exp(ctx));
        CHECK(n.as_float() == Approx(x));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::exp(ctx), "bad argument #1 to 'exp' (number expected, got string)");
    }
}

TEST_CASE("math.floor(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        SECTION("Integer") {
            int i = 42;
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::floor(ctx) == minilua::Value(i));

            i = 0;
            list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::floor(ctx) == minilua::Value(i));

            i = -982;
            list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::floor(ctx) == minilua::Value(i));
        }

        SECTION("Double") {
            double i = 42.5;
            minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::floor(ctx) == minilua::Value(42));

            i = -1.9;
            list = minilua::Vallist({minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::floor(ctx) == minilua::Value(-2));
        }
    }

    SECTION("String") {
        std::string i = "42";
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::floor(ctx) == minilua::Value(42));

        i = "0";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::floor(ctx) == minilua::Value(0));

        i = "-982";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::floor(ctx) == minilua::Value(-982));

        i = "42.5";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::floor(ctx) == minilua::Value(42));

        i = "-1.9";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::floor(ctx) == minilua::Value(-2));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::floor(ctx), "bad argument #1 to 'floor' (number expected, got string)");
    }
}

TEST_CASE("math.fmod(x, y)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Integer, Integer") {
        double i = 42.5;
        double j = 4.2;
        minilua::Vallist list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::fmod(ctx));
        CHECK(n.as_float() == Approx(0.5));

        i = -2.5;
        j = 4.2;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(-2.5));

        i = -2.5;
        j = -4.2;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(-2.5));

        i = 2.5;
        j = -4.2;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(2.5));

        i = 2.5;
        j = 0;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::fmod(ctx));
        CHECK(std::isnan(n.as_float()));

        i = 0;
        j = 2.5;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(0));

        i = 0;
        j = 0;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(minilua::math::fmod(ctx), "bad argument #2 to 'fmod' (zero)");
    }

    SECTION("Integer, String") {
        double i = 42.5;
        std::string j = "4.2";
        minilua::Vallist list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::fmod(ctx));
        CHECK(n.as_float() == Approx(0.5));

        i = -2.5;
        j = "4.2";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(-2.5));

        i = -2.5;
        j = "-4.2";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(-2.5));

        i = 2.5;
        j = "-4.2";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(2.5));

        i = 2.5;
        j = "0";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::fmod(ctx));
        CHECK(std::isnan(n.as_float()));

        i = 0;
        j = "2.5";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(0));

        i = 0;
        j = "0";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(minilua::math::fmod(ctx), "bad argument #2 to 'fmod' (zero)");

        i = 0;
        j = "Baum";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::fmod(ctx), "bad argument #2 to 'fmod' (number expected, got string)");
    }

    SECTION("String, Integer") {
        std::string i = "42.5";
        double j = 4.2;
        minilua::Vallist list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::fmod(ctx));
        CHECK(n.as_float() == Approx(0.5));

        i = "-2.5";
        j = 4.2;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(-2.5));

        i = "-2.5";
        j = -4.2;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(-2.5));

        i = "2.5";
        j = -4.2;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(2.5));

        i = "2.5";
        j = 0;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::fmod(ctx));
        CHECK(std::isnan(n.as_float()));

        i = "0";
        j = 2.5;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(0));

        i = "0";
        j = 0;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(minilua::math::fmod(ctx), "bad argument #2 to 'fmod' (zero)");

        i = "lua";
        j = 0;
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::fmod(ctx), "bad argument #1 to 'fmod' (number expected, got string)");
    }

    SECTION("String, String") {
        std::string i = "42.5";
        std::string j = "4.2";
        minilua::Vallist list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::fmod(ctx));
        CHECK(n.as_float() == Approx(0.5));

        i = "-2.5";
        j = "4.2";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(-2.5));

        i = "-2.5";
        j = "-4.2";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(-2.5));

        i = "2.5";
        j = "-4.2";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(2.5));

        i = "2.5";
        j = "0";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::fmod(ctx));
        CHECK(std::isnan(n.as_float()));

        i = "0";
        j = "2.5";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::fmod(ctx) == minilua::Value(0));

        i = "0";
        j = "0";
        list = minilua::Vallist({minilua::Value(i), minilua::Value(j)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(minilua::math::fmod(ctx), "bad argument #2 to 'fmod' (zero)");
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        bool b = true;
        minilua::Vallist list = minilua::Vallist({minilua::Value(s), minilua::Value(b)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::floor(ctx), "bad argument #1 to 'floor' (number expected, got string)");
    }
}

TEST_CASE("math.log(x [, base]") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Number, Number") {
        int x = 3;
        int y = 2;
        minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
        ctx = ctx.make_new(list);
        auto n = std::get<minilua::Number>(minilua::math::log(ctx));
        CHECK(n.as_float() == Approx(1.5849625007212));
    }

    SECTION("Number, Nil") {
        int x = 3;
        minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Nil()});
        ctx = ctx.make_new(list);
        auto n = std::get<minilua::Number>(minilua::math::log(ctx));
        CHECK(n.as_float() == Approx(1.0986122886681));
    }

    SECTION("Number, String") {
        SECTION("Valid String") {
            int x = 3;
            std::string y = "2";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            auto n = std::get<minilua::Number>(minilua::math::log(ctx));
            CHECK(n.as_float() == Approx(1.5849625007212));
        }

        SECTION("Invalid String") {
            int x = 1;
            std::string y = "Minilua";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::log(ctx), "bad argument #2 to 'log' (number expected, got string)");
        }
    }

    SECTION("Number, Bool") {
        int x = 1;
        bool y = false;
        minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::log(ctx), "bad argument #2 to 'log' (number expected, got boolean)");
    }

    SECTION("String, Number") {
        SECTION("Valid String") {
            std::string x = "3";
            int y = 2;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            auto n = std::get<minilua::Number>(minilua::math::log(ctx));
            CHECK(n.as_float() == Approx(1.5849625007212));
        }

        SECTION("Invalid String") {
            std::string x = "Baum";
            int y = 2;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::log(ctx), "bad argument #1 to 'log' (number expected, got string)");
        }
    }

    SECTION("String, Nil") {
        SECTION("Valid String") {
            std::string s = "3";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s), minilua::Nil()});
            ctx = ctx.make_new(list);
            auto n = std::get<minilua::Number>(minilua::math::log(ctx));
            CHECK(n.as_float() == Approx(1.0986122886681));
        }

        SECTION("Invalid String") {
            std::string s = "Baum";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s), minilua::Nil()});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::log(ctx), "bad argument #1 to 'log' (number expected, got string)");
        }
    }

    SECTION("String, String") {
        SECTION("Valid String, Valid String") {
            std::string s = "3";
            std::string i = "2";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s), minilua::Value(i)});
            ctx = ctx.make_new(list);
            auto n = std::get<minilua::Number>(minilua::math::log(ctx));
            CHECK(n.as_float() == Approx(1.5849625007212));
        }

        SECTION("Valid String, Invalid String") {
            std::string s = "1";
            std::string i = "Minilua";
            minilua::Vallist list = minilua::Vallist({minilua::Value(s), minilua::Value(i)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::log(ctx), "bad argument #2 to 'log' (number expected, got string)");
        }

        SECTION("Invalid String, Valid String") {
            std::string i = "Minilua";
            std::string s = "1";
            minilua::Vallist list = minilua::Vallist({minilua::Value(i), minilua::Value(s)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::log(ctx), "bad argument #1 to 'log' (number expected, got string)");
        }

        SECTION("Invalid String, Invalid String") {
            std::string i = "Minilua";
            std::string s = "Baum";
            minilua::Vallist list = minilua::Vallist({minilua::Value(i), minilua::Value(s)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::log(ctx), "bad argument #1 to 'log' (number expected, got string)");
        }
    }

    SECTION("String, Bool") {
        SECTION("Valid String") {
            std::string x = "1";
            bool y = true;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::log(ctx), "bad argument #2 to 'log' (number expected, got boolean)");
        }

        SECTION("Invalid String") {
            std::string x = "Baum";
            bool y = true;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::log(ctx), "bad argument #1 to 'log' (number expected, got string)");
        }
    }

    SECTION("base = 0") {
        SECTION("base is Number") {
            int x = 3;
            int y = 0;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::log(ctx) == minilua::Value(-0));
        }

        SECTION("base is String") {
            int x = 3;
            std::string y = "0";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Value(y)});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::log(ctx) == minilua::Value(-0));
        }
    }

    SECTION("x = 0") {
        SECTION("x is Number") {
            int x = 0;
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Nil()});
            ctx = ctx.make_new(list);
            minilua::Number n = std::get<minilua::Number>(minilua::math::log(ctx));
            CHECK(std::isinf(n.as_float()));

            int base = 2;
            list = minilua::Vallist({minilua::Value(x), minilua::Value(base)});
            ctx = ctx.make_new(list);
            n = std::get<minilua::Number>(minilua::math::log(ctx));
            CHECK(std::isinf(n.as_float()));
        }

        SECTION("x is String") {
            std::string x = "0";
            minilua::Vallist list = minilua::Vallist({minilua::Value(x), minilua::Nil()});
            ctx = ctx.make_new(list);
            minilua::Number n = std::get<minilua::Number>(minilua::math::log(ctx));
            CHECK(std::isinf(n.as_float()));
        }
    }

    SECTION("log(1,1)") {
        int x = 1;
        int base = x;

        minilua::Vallist list({minilua::Value(x), minilua::Value(base)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::log(ctx));
        CHECK(std::isnan(n.as_float()));
    }

    SECTION("log(0,0)") {
        int x = 0;
        int base = x;

        minilua::Vallist list({minilua::Value(x), minilua::Value(base)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::log(ctx));
        CHECK(std::isnan(n.as_float()));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s), minilua::Nil()});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::log(ctx), "bad argument #1 to 'log' (number expected, got string)");
    }
}

TEST_CASE("math.max(x, ...)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);
    SECTION("Numbers") {
        std::vector<minilua::Value> v;
        for (int i = 9; i >= 0; i--) {
            v.emplace_back(i);
        }
        // Highest value is at the first position
        minilua::Vallist list = minilua::Vallist(v);
        ctx = ctx.make_new(list);
        CHECK(minilua::math::max(ctx) == 9);

        // Highest value is at the last position
        v.emplace_back(42);
        list = minilua::Vallist(v);
        ctx = ctx.make_new(list);
        CHECK(minilua::math::max(ctx) == 42);

        // Highest value is in the middle
        for (int i = 12; i <= 20; i++) {
            v.emplace_back(i);
        }
        list = minilua::Vallist(v);
        ctx = ctx.make_new(list);
        CHECK(minilua::math::max(ctx) == 42);
    }

    SECTION("Strings") {
        std::vector<minilua::Value> v;
        std::list<std::string> values = {"ziehen", "Baum", "MiniLua", "lua", "welt"};
        for (const auto& a : values) {
            v.emplace_back(a);
        }
        // Highest value is at the first position
        minilua::Vallist list = minilua::Vallist(v);
        ctx = ctx.make_new(list);
        CHECK(minilua::math::max(ctx) == minilua::Value("ziehen"));

        // Highest value is at the last position
        v.emplace_back("zug");
        list = minilua::Vallist(v);
        ctx = ctx.make_new(list);
        CHECK(minilua::math::max(ctx) == minilua::Value("zug"));

        // Highest value is in the middle
        values = {"Corona", "Sudoku", "c++", "Ulm", "Universität"};
        for (const auto& a : values) {
            v.emplace_back(a);
        }
        list = minilua::Vallist(v);
        ctx = ctx.make_new(list);
        CHECK(minilua::math::max(ctx) == minilua::Value("zug"));
    }

    SECTION("No arguemts") {
        CHECK_THROWS_WITH(minilua::math::max(ctx), "bad argument #1 to 'max' (value expected)");
    }
}

TEST_CASE("math.min(x, ...)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        std::vector<minilua::Value> v;
        for (int i = 0; i <= 9; i++) {
            v.emplace_back(i);
        }
        // Lowest value is at the first position
        minilua::Vallist list = minilua::Vallist(v);
        ctx = ctx.make_new(list);
        CHECK(minilua::math::min(ctx) == 0);

        // Lowest value is at the last position
        v.emplace_back(-1);
        list = minilua::Vallist(v);
        ctx = ctx.make_new(list);
        CHECK(minilua::math::min(ctx) == -1);

        // Lowest value is in the middle
        for (int i = 12; i <= 20; i++) {
            v.emplace_back(i);
        }
        list = minilua::Vallist(v);
        ctx = ctx.make_new(list);
        CHECK(minilua::math::min(ctx) == -1);
    }

    SECTION("Strings") {
        std::vector<minilua::Value> v;
        std::list<std::string> values = {"Baum", "ziehen", "MiniLua", "lua", "welt"};
        for (const auto& a : values) {
            v.emplace_back(a);
        }
        // Lowest value is at the first position
        minilua::Vallist list = minilua::Vallist(v);
        ctx = ctx.make_new(list);
        CHECK(minilua::math::min(ctx) == minilua::Value("Baum"));

        // Lowest value is at the last position
        v.emplace_back("Analysis2a");
        list = minilua::Vallist(v);
        ctx = ctx.make_new(list);
        CHECK(minilua::math::min(ctx) == minilua::Value("Analysis2a"));

        // Lowest value is in the middle
        values = {"Corona", "Sudoku", "c++", "Ulm", "Universität"};
        for (const auto& a : values) {
            v.emplace_back(a);
        }
        list = minilua::Vallist(v);
        ctx = ctx.make_new(list);
        CHECK(minilua::math::min(ctx) == minilua::Value("Analysis2a"));
    }

    SECTION("No arguemts") {
        CHECK_THROWS_WITH(minilua::math::min(ctx), "bad argument #1 to 'min' (value expected)");
    }
}

TEST_CASE("math.modf(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        SECTION("whole numbers") {
            int x = 42;
            minilua::Vallist list({x});
            ctx = ctx.make_new(list);
            minilua::Vallist result({42, 0.0});

            CHECK(minilua::math::modf(ctx) == result);
        }

        SECTION("real numbers") {
            double x = 42.5;
            minilua::Vallist list({x});
            ctx = ctx.make_new(list);
            minilua::Vallist result({42, 0.5});

            CHECK(minilua::math::modf(ctx) == result);

            x = 2.125;
            list = minilua::Vallist({x});
            ctx = ctx.make_new(list);
            result = minilua::Vallist({2, 0.125});

            CHECK(minilua::math::modf(ctx) == result);
        }
    }

    SECTION("Strings") {
        SECTION("whole numbers") {
            std::string x = "42";
            minilua::Vallist list({x});
            ctx = ctx.make_new(list);
            minilua::Vallist result({42, 0.0});

            CHECK(minilua::math::modf(ctx) == result);
        }

        SECTION("real numbers") {
            std::string x = "42.5";
            minilua::Vallist list({x});
            ctx = ctx.make_new(list);
            minilua::Vallist result({42, 0.5});

            CHECK(minilua::math::modf(ctx) == result);

            x = "2.125";
            list = minilua::Vallist({x});
            ctx = ctx.make_new(list);
            result = minilua::Vallist({2, 0.125});

            CHECK(minilua::math::modf(ctx) == result);
        }
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::modf(ctx), "bad argument #1 to 'modf' (number expected, got string)");
    }
}

TEST_CASE("math.rad(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        int i = 0;
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::rad(ctx) == minilua::Value(0));

        i = 1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::rad(ctx));
        CHECK(n.as_float() == Approx(0.017453292519943));

        i = -1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::rad(ctx));
        CHECK(n.as_float() == Approx(-0.017453292519943));

        double d = 180;
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::rad(ctx));
        CHECK(n.as_float() == Approx(minilua::math::PI));

        d = 2.5;
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::rad(ctx));
        CHECK(n.as_float() == Approx(0.043633231299858));
    }

    SECTION("Strings") {
        std::string i = "0";
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::rad(ctx) == minilua::Value(0));

        i = "1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::rad(ctx));
        CHECK(n.as_float() == Approx(0.017453292519943));

        i = "-1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::rad(ctx));
        CHECK(n.as_float() == Approx(-0.017453292519943));

        std::string d = "180";
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::rad(ctx));
        CHECK(n.as_float() == Approx(minilua::math::PI));

        d = "2.5";
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::rad(ctx));
        CHECK(n.as_float() == Approx(0.043633231299858));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::rad(ctx), "bad argument #1 to 'rad' (number expected, got string)");
    }
}

TEST_CASE("math.randomseed(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        int i = 42;
        minilua::Vallist list({i});
        ctx = ctx.make_new(list);
        minilua::math::randomseed(ctx);
        CHECK(minilua::math::get_random_seed() == std::default_random_engine((unsigned int)i));
    }

    SECTION("Strings") {
        std::string i = "42";
        minilua::Vallist list({i});
        ctx = ctx.make_new(list);
        minilua::math::randomseed(ctx);
        CHECK(minilua::math::get_random_seed() == std::default_random_engine((unsigned int)42));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::randomseed(ctx),
            "bad argument #1 to 'randomseed' (number expected, got string)");
    }
}

TEST_CASE("math.random([x, [y]]") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);
    int seed = 42;

    // Setup to always produce the same results because of always using the same seed
    minilua::Vallist list({seed});
    minilua::CallContext ctx_seed = ctx.make_new(list);

    SECTION("nil, nil") {
        minilua::math::randomseed(ctx_seed);
        list = minilua::Vallist({minilua::Nil(), minilua::Nil()});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::random(ctx));
        CHECK(n.as_float() == Approx(0.524587));
    }

    SECTION("Number, nil") {
        minilua::math::randomseed(ctx_seed);
        int i = 1967;
        list = minilua::Vallist({i, minilua::Nil()});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::random(ctx) == minilua::Value(1));

        minilua::math::randomseed(ctx_seed);
        i = 0;
        list = minilua::Vallist({i, minilua::Nil()});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::random(ctx) == minilua::Value(1126542223));
    }

    SECTION("String, Nil") {
        SECTION("valid string") {
            minilua::math::randomseed(ctx_seed);
            std::string i = "1967";
            list = minilua::Vallist({i, minilua::Nil()});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::random(ctx) == minilua::Value(1));

            minilua::math::randomseed(ctx_seed);
            i = "0";
            list = minilua::Vallist({i, minilua::Nil()});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::random(ctx) == minilua::Value(1126542223));
        }

        SECTION("Invalid string") {
            std::string s = "Minilua";
            list = minilua::Vallist({minilua::Value(s), minilua::Nil()});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx),
                "bad argument #1 to 'random' (number expected, got string)");
        }
    }

    SECTION("Boolean, Nil") {
        minilua::math::randomseed(ctx_seed);
        bool i = true;
        list = minilua::Vallist({i, minilua::Nil()});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::random(ctx),
            "bad argument #1 to 'random' (number expected, got boolean)");
    }

    SECTION("Number, Number") {
        minilua::math::randomseed(ctx_seed);
        int i = 1967;
        int j = 2021;
        list = minilua::Vallist({i, j});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::random(ctx) == minilua::Value(1967));

        minilua::math::randomseed(ctx_seed);
        i = 4;
        j = 4;
        list = minilua::Vallist({i, j});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::random(ctx) == minilua::Value(4));

        minilua::math::randomseed(ctx_seed);
        i = 123;
        j = 4;
        list = minilua::Vallist({i, j});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::random(ctx), "bad argument #1 to 'random' (interval is empty)");
    }

    SECTION("String, Number") {
        SECTION("valid string") {
            minilua::math::randomseed(ctx_seed);
            std::string i = "1967";
            int j = 2021;
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::random(ctx) == minilua::Value(1967));

            minilua::math::randomseed(ctx_seed);
            i = "4";
            j = 4;
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::random(ctx) == minilua::Value(4));

            minilua::math::randomseed(ctx_seed);
            i = "123";
            j = 4;
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx), "bad argument #1 to 'random' (interval is empty)");
        }

        SECTION("invalid string") {
            std::string s = "Minilua";
            int j = 4;
            list = minilua::Vallist({minilua::Value(s), j});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx),
                "bad argument #1 to 'random' (number expected, got string)");
        }
    }

    SECTION("Boolean, Number") {
        bool s = false;
        int j = 4;
        list = minilua::Vallist({minilua::Value(s), j});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::random(ctx),
            "bad argument #1 to 'random' (number expected, got boolean)");
    }

    SECTION("Number, String") {
        SECTION("valid string") {
            minilua::math::randomseed(ctx_seed);
            int i = 1967;
            std::string j = "2021";
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::random(ctx) == minilua::Value(1967));

            minilua::math::randomseed(ctx_seed);
            i = 4;
            j = "4";
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::random(ctx) == minilua::Value(4));

            minilua::math::randomseed(ctx_seed);
            i = 123;
            j = "4";
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx), "bad argument #1 to 'random' (interval is empty)");
        }

        SECTION("invalid string") {
            std::string s = "Minilua";
            int j = 4;
            list = minilua::Vallist({j, s});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx),
                "bad argument #2 to 'random' (number expected, got string)");
        }
    }

    SECTION("Boolean, String") {
        SECTION("valid string") {
            minilua::math::randomseed(ctx_seed);
            bool i = true;
            std::string j = "2021";
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx),
                "bad argument #1 to 'random' (number expected, got boolean)");
        }

        SECTION("invalid string") {
            bool j = false;
            std::string s = "Minilua";
            list = minilua::Vallist({j, s});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx),
                "bad argument #1 to 'random' (number expected, got boolean)");
        }
    }

    SECTION("String, String") {
        SECTION("Valid, valid") {
            minilua::math::randomseed(ctx_seed);
            std::string i = "1967";
            std::string j = "2021";
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::random(ctx) == minilua::Value(1967));

            minilua::math::randomseed(ctx_seed);
            i = "4";
            j = "4";
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::random(ctx) == minilua::Value(4));

            minilua::math::randomseed(ctx_seed);
            i = "123";
            j = "4";
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx), "bad argument #1 to 'random' (interval is empty)");
        }

        SECTION("Valid, Invalid") {
            minilua::math::randomseed(ctx_seed);
            std::string i = "123";
            std::string j = "Minilua";
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx),
                "bad argument #2 to 'random' (number expected, got string)");
        }

        SECTION("Invalid, Valid") {
            minilua::math::randomseed(ctx_seed);
            std::string i = "MiniLua";
            std::string j = "4";
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx),
                "bad argument #1 to 'random' (number expected, got string)");
        }

        SECTION("Invalid, Invalid") {
            minilua::math::randomseed(ctx_seed);
            std::string i = "MiniLua";
            std::string j = "baum";
            list = minilua::Vallist({i, j});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx),
                "bad argument #1 to 'random' (number expected, got string)");
        }
    }

    SECTION("Number, Boolean") {
        int j = 4;
        bool s = false;
        list = minilua::Vallist({j, s});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::random(ctx),
            "bad argument #2 to 'random' (number expected, got boolean)");
    }

    SECTION("String, Boolean") {
        SECTION("Valid string") {
            std::string j = "4";
            bool s = false;
            list = minilua::Vallist({j, s});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx),
                "bad argument #2 to 'random' (number expected, got boolean)");
        }

        SECTION("Invalid string") {
            std::string j = "Minilua";
            bool s = false;
            list = minilua::Vallist({j, s});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::random(ctx),
                "bad argument #1 to 'random' (number expected, got string)");
        }
    }

    SECTION("Boolean, Boolean") {
        bool s = false;
        bool j = true;
        list = minilua::Vallist({minilua::Value(s), j});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::random(ctx),
            "bad argument #1 to 'random' (number expected, got boolean)");
    }
}

TEST_CASE("math.sin(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        int i = 0;
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::sin(ctx) == minilua::Value(0));

        i = 1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::sin(ctx));
        CHECK(n.as_float() == Approx(0.8414709848079));

        i = -1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::sin(ctx));
        CHECK(n.as_float() == Approx(-0.8414709848079));

        double d = 180;
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::sin(ctx));
        CHECK(n.as_float() == Approx(-0.80115263573383));

        d = 1.579;
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::sin(ctx));
        CHECK(n.as_float() == Approx(0.99996635006169));
    }

    SECTION("Strings") {
        std::string i = "0";
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::sin(ctx) == minilua::Value(0));

        i = "1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::sin(ctx));
        CHECK(n.as_float() == Approx(0.8414709848079));

        i = "-1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::sin(ctx));
        CHECK(n.as_float() == Approx(-0.8414709848079));

        std::string d = "180";
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::sin(ctx));
        CHECK(n.as_float() == Approx(-0.80115263573383));

        d = "1.579";
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::sin(ctx));
        CHECK(n.as_float() == Approx(0.99996635006169));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::sin(ctx), "bad argument #1 to 'sin' (number expected, got string)");
    }
}

TEST_CASE("math.sqrt(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        int i = 0;
        minilua::Vallist list({i});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::sqrt(ctx) == minilua::Value(0));

        i = 1;
        list = minilua::Vallist({i});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::sqrt(ctx) == minilua::Value(1));

        i = 4;
        list = minilua::Vallist({i});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::sqrt(ctx) == minilua::Value(2));

        double d = 2.5;
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::sqrt(ctx));
        CHECK(n.as_float() == Approx(1.5811388300842));

        i = -1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::sqrt(ctx));
        CHECK(isnan(n.as_float()));
    }

    SECTION("Strings") {
        std::string i = "0";
        minilua::Vallist list({i});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::sqrt(ctx) == minilua::Value(0));

        i = "1";
        list = minilua::Vallist({i});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::sqrt(ctx) == minilua::Value(1));

        i = "4";
        list = minilua::Vallist({i});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::sqrt(ctx) == minilua::Value(2));

        std::string d = "2.5";
        list = minilua::Vallist({d});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::sqrt(ctx));
        CHECK(n.as_float() == Approx(1.5811388300842));

        i = "-1";
        list = minilua::Vallist({i});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::sqrt(ctx));
        CHECK(isnan(n.as_float()));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::sqrt(ctx), "bad argument #1 to 'sqrt' (number expected, got string)");
    }
}

TEST_CASE("math.tan(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        int i = 0;
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::tan(ctx) == minilua::Value(0));

        i = 1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::tan(ctx));
        CHECK(n.as_float() == Approx(1.5574077246549));

        i = -1;
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::tan(ctx));
        CHECK(n.as_float() == Approx(-1.5574077246549));

        double d = 180;
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::tan(ctx));
        CHECK(n.as_float() == Approx(1.3386902103512));

        d = 1.579;
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::tan(ctx));
        CHECK(n.as_float() == Approx(-121.89388112867));
    }

    SECTION("Strings") {
        std::string i = "0";
        minilua::Vallist list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::tan(ctx) == minilua::Value(0));

        i = "1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        minilua::Number n = std::get<minilua::Number>(minilua::math::tan(ctx));
        CHECK(n.as_float() == Approx(1.5574077246549));

        i = "-1";
        list = minilua::Vallist({minilua::Value(i)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::tan(ctx));
        CHECK(n.as_float() == Approx(-1.5574077246549));

        std::string d = "180";
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::tan(ctx));
        CHECK(n.as_float() == Approx(1.3386902103512));

        d = "1.579";
        list = minilua::Vallist({minilua::Value(d)});
        ctx = ctx.make_new(list);
        n = std::get<minilua::Number>(minilua::math::tan(ctx));
        CHECK(n.as_float() == Approx(-121.89388112867));
    }

    SECTION("invalid input") {
        std::string s = "Minilua";
        minilua::Vallist list = minilua::Vallist({minilua::Value(s)});
        ctx = ctx.make_new(list);
        CHECK_THROWS_WITH(
            minilua::math::tan(ctx), "bad argument #1 to 'tan' (number expected, got string)");
    }
}

TEST_CASE("math.tointeger(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("Numbers") {
        SECTION("Integers") {
            int i = 0;
            minilua::Vallist list({i});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::to_integer(ctx) == minilua::Value(i));

            i = 0xA;
            list = minilua::Vallist({i});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::to_integer(ctx) == minilua::Value(i));
        }

        SECTION("Floats") {
            double i = 1.0;
            minilua::Vallist list = minilua::Vallist({i});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::to_integer(ctx) == minilua::Value(1));

            i = 10e1;
            list = minilua::Vallist({i});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::to_integer(ctx) == minilua::Value(100));

            i = 2.5;
            list = minilua::Vallist({i});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::to_integer(ctx) == minilua::Nil());

            i = 10e-3;
            list = minilua::Vallist({i});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::to_integer(ctx) == minilua::Nil());
        }
    }

    SECTION("Strings") {
        std::string s = "2";
        minilua::Vallist list = minilua::Vallist({s});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::to_integer(ctx) == minilua::Value(2));

        s = "2.5";
        list = minilua::Vallist({s});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::to_integer(ctx) == minilua::Nil());

        s = "0xA";
        list = minilua::Vallist({s});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::to_integer(ctx) == minilua::Value(10));

        s = "Minilua";
        list = minilua::Vallist({s});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::to_integer(ctx) == minilua::Nil());
    }

    SECTION("Bool") {
        bool b = true;
        minilua::Vallist list = minilua::Vallist({b});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::to_integer(ctx) == minilua::Nil());

        b = false;
        list = minilua::Vallist({b});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::to_integer(ctx) == minilua::Nil());
    }
}

TEST_CASE("math.type(x)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    minilua::Value integer("integer");
    minilua::Value floatt("float");
    SECTION("Numbers") {
        SECTION("Integers") {
            int i = 0;
            minilua::Vallist list({i});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::type(ctx) == integer);

            i = 0xA;
            list = minilua::Vallist({i});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::type(ctx) == integer);
        }

        SECTION("Floats") {
            double i = 1.5;
            minilua::Vallist list = minilua::Vallist({i});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::type(ctx) == floatt);

            /*i = 10e1;
            list = minilua::Vallist({i});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::type(ctx) == floatt);*/

            i = 10e-3;
            list = minilua::Vallist({i});
            ctx = ctx.make_new(list);
            CHECK(minilua::math::type(ctx) == floatt);
        }
    }

    SECTION("Strings") {
        std::string s = "2";
        minilua::Vallist list = minilua::Vallist({s});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::type(ctx) == minilua::Nil());

        s = "2.5";
        list = minilua::Vallist({s});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::type(ctx) == minilua::Nil());

        s = "0xA";
        list = minilua::Vallist({s});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::type(ctx) == minilua::Nil());

        s = "Minilua";
        list = minilua::Vallist({s});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::type(ctx) == minilua::Nil());
    }

    SECTION("Bool") {
        bool b = true;
        minilua::Vallist list = minilua::Vallist({b});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::type(ctx) == minilua::Nil());

        b = false;
        list = minilua::Vallist({b});
        ctx = ctx.make_new(list);
        CHECK(minilua::math::type(ctx) == minilua::Nil());
    }
}

TEST_CASE("math.ult(m, n)") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("well formated input") {
        SECTION("Number, Number") {
            int m = -1;
            int n = -2;
            minilua::Vallist list({m, n});
            ctx = ctx.make_new(list);
            minilua::Bool b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK_FALSE(b);

            m = -1;
            n = 2;
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK(!b);

            m = 1;
            n = -2;
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK(b);

            m = 1;
            n = 2;
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK(b);
        }

        SECTION("Number, String") {
            int m = -1;
            std::string n = "-2";
            minilua::Vallist list({m, n});
            ctx = ctx.make_new(list);
            minilua::Bool b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK_FALSE(b);

            m = -1;
            n = "2";
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK_FALSE(b);

            m = 1;
            n = "-2";
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK(b);

            m = 1;
            n = "2";
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK(b);
        }

        SECTION("String, Number") {
            std::string m = "-1";
            int n = -2;
            minilua::Vallist list({m, n});
            ctx = ctx.make_new(list);
            minilua::Bool b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK_FALSE(b);

            m = "-1";
            n = 2;
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK_FALSE(b);

            m = "1";
            n = -2;
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK(b);

            m = "1";
            n = 2;
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK(b);
        }

        SECTION("String, String") {
            std::string m = "-1";
            std::string n = "-2";
            minilua::Vallist list({m, n});
            ctx = ctx.make_new(list);
            minilua::Bool b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK_FALSE(b);

            m = "-1";
            n = "2";
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK_FALSE(b);

            m = "1";
            n = "-2";
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK(b);

            m = "1";
            n = "2";
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            b = std::get<minilua::Bool>(minilua::math::ult(ctx));
            CHECK(b);
        }
    }

    SECTION("invalid formated input (float numbers)") {
        SECTION("Number, Number") {
            double m = -1.3278462978346;
            double n = -2.9837165;
            minilua::Vallist list({m, n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx),
                "bad argument #1 to 'ult' (number has no integer representation)");

            m = -1;
            n = 2.5;
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx),
                "bad argument #2 to 'ult' (number has no integer representation)");

            m = 1.42;
            n = -2;
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx),
                "bad argument #1 to 'ult' (number has no integer representation)");
        }

        SECTION("Number, String") {
            double m = -1.3278462978346;
            std::string n = "-2.9837165";
            minilua::Vallist list({m, n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx),
                "bad argument #1 to 'ult' (number has no integer representation)");

            m = -1;
            n = "2.5";
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx),
                "bad argument #2 to 'ult' (number has no integer representation)");

            m = 1.42;
            n = "-2";
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx),
                "bad argument #1 to 'ult' (number has no integer representation)");
        }

        SECTION("String, Number") {
            std::string m = "-1.3278462978346";
            double n = -2.9837165;
            minilua::Vallist list({m, n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx),
                "bad argument #1 to 'ult' (number has no integer representation)");

            m = "-1";
            n = 2.5;
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx),
                "bad argument #2 to 'ult' (number has no integer representation)");

            m = "1.42";
            n = -2;
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx),
                "bad argument #1 to 'ult' (number has no integer representation)");
        }

        SECTION("String, String") {
            std::string m = "-1.3278462978346";
            std::string n = "-2.9837165";
            minilua::Vallist list({m, n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx),
                "bad argument #1 to 'ult' (number has no integer representation)");

            m = "-1";
            n = "2.5";
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx),
                "bad argument #2 to 'ult' (number has no integer representation)");

            m = "1.42";
            n = "-2";
            list = minilua::Vallist({m, n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx),
                "bad argument #1 to 'ult' (number has no integer representation)");
        }
    }

    SECTION("Invalid input") {
        SECTION("m is invalid") {
            std::string m = "Minilua";
            std::string n = "baum";
            minilua::Vallist list = minilua::Vallist({minilua::Value(m), n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx), "bad argument #1 to 'ult' (number expected, got string)");

            n = "1";
            list = minilua::Vallist({minilua::Value(m), n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx), "bad argument #1 to 'ult' (number expected, got string)");
        }

        SECTION("n is invalid") {
            int m = 42;
            std::string n = "baum";
            minilua::Vallist list = minilua::Vallist({minilua::Value(m), n});
            ctx = ctx.make_new(list);
            CHECK_THROWS_WITH(
                minilua::math::ult(ctx), "bad argument #2 to 'ult' (number expected, got string)");
        }
    }
}

TEST_CASE("reverse abs") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("force result of abs to a positive number") {
        int i = 42;
        minilua::Value value =
            minilua::Value(i).with_origin(minilua::LiteralOrigin{minilua::Range()});
        minilua::Vallist list = minilua::Vallist(value);
        ctx = ctx.make_new(list);
        auto res = minilua::math::abs(ctx);
        REQUIRE(res == 42);

        auto result = res.force(minilua::Value{25});
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "25"));

        std::string s = "42";
        value = minilua::Value(i).with_origin(minilua::LiteralOrigin{minilua::Range()});
        list = minilua::Vallist({value});
        ctx = ctx.make_new(list);
        res = minilua::math::abs(ctx);
        REQUIRE(res == 42);

        result = res.force(minilua::Value{25});
        REQUIRE(result.has_value());

        INFO(result->origin());
        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "25"));

        result = res.force(minilua::Value{"10"});
        CHECK_FALSE(result.has_value());
    }

    SECTION("force result of abs to a negative number or invalid value") {
        int i = 42;
        minilua::Value value =
            minilua::Value(i).with_origin(minilua::LiteralOrigin{minilua::Range()});
        minilua::Vallist list = minilua::Vallist(value);
        ctx = ctx.make_new(list);
        auto res = minilua::math::abs(ctx);
        REQUIRE(res == 42);

        // abs only returns positive numbers, so the result can't be forced to a negative number
        auto result = res.force(minilua::Value{-25});
        CHECK_FALSE(result.has_value());

        // abs only returns positive Numbers, so the result can't be forced to another datatype
        // other than Number
        result = res.force(minilua::Value{true});
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("reverse acos") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("correct force") {
        double x = -0.5;
        minilua::Value value = minilua::Value(x).with_origin(minilua::LiteralOrigin());
        minilua::Vallist list = minilua::Vallist(value);
        ctx = ctx.make_new(list);

        auto res = minilua::math::acos(ctx);
        minilua::Number n = std::get<minilua::Number>(res);
        REQUIRE(n.as_float() == Approx(2.0944));

        auto result = res.force(minilua::Value(0));
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "1"));

        std::string s = "-0.5";
        value = minilua::Value(x).with_origin(minilua::LiteralOrigin());
        list = minilua::Vallist({value});
        ctx = ctx.make_new(list);

        res = minilua::math::acos(ctx);
        n = std::get<minilua::Number>(res);
        REQUIRE(n.as_float() == Approx(2.0944));

        result = res.force(minilua::Value(0));
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "1"));

        SECTION("force value to nan") {
            double x = -0.5;
            minilua::Value value = minilua::Value(x).with_origin(minilua::LiteralOrigin());
            minilua::Vallist list = minilua::Vallist(value);
            ctx = ctx.make_new(list);

            auto res = minilua::math::acos(ctx);
            minilua::Number n = std::get<minilua::Number>(res);
            REQUIRE(n.as_float() == Approx(2.0944));

            // force value to nan, directly insert nan doesn't work
            auto result = res.force(minilua::Value(std::asin(2)));
            REQUIRE(result.has_value());

            CHECK(
                result.value().collect_first_alternative()[0] ==
                minilua::SourceChange(minilua::Range(), "nan"));
        }
    }

    SECTION("invalid force") {
        double x = -0.5;
        minilua::Value value = minilua::Value(x).with_origin(minilua::LiteralOrigin());
        minilua::Vallist list = minilua::Vallist(value);
        ctx = ctx.make_new(list);

        auto res = minilua::math::acos(ctx);
        minilua::Number n = std::get<minilua::Number>(res);
        REQUIRE(n.as_float() == Approx(2.0944));

        // acos only returns Numbers, so the result can't be forced to a string, even if its
        // formated like a Number
        auto result = res.force(minilua::Value("2"));
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("reverse asin") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("correct force") {
        double x = -0.5;
        minilua::Value value = minilua::Value(x).with_origin(minilua::LiteralOrigin());
        minilua::Vallist list = minilua::Vallist(value);
        ctx = ctx.make_new(list);

        auto res = minilua::math::asin(ctx);
        minilua::Number n = std::get<minilua::Number>(res);
        REQUIRE(n.as_float() == Approx(-0.5235987755983));

        auto result = res.force(minilua::Value(0));
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "0"));

        std::string s = "-0.5";
        value = minilua::Value(s).with_origin(minilua::LiteralOrigin());
        list = minilua::Vallist(value);
        ctx = ctx.make_new(list);

        res = minilua::math::asin(ctx);
        n = std::get<minilua::Number>(res);
        REQUIRE(n.as_float() == Approx(-0.5235987755983));

        result = res.force(minilua::Value(0));
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "0"));

        x = 2;
        value = minilua::Value(x).with_origin(minilua::LiteralOrigin());
        list = minilua::Vallist(value);
        ctx = ctx.make_new(list);

        res = minilua::math::asin(ctx);
        n = std::get<minilua::Number>(res);
        REQUIRE(std::isnan(n.as_float()));

        result = res.force(minilua::Value(0));
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "0"));

        SECTION("force value to nan") {
            double x = -0.5;
            minilua::Value value = minilua::Value(x).with_origin(minilua::LiteralOrigin());
            minilua::Vallist list = minilua::Vallist(value);
            ctx = ctx.make_new(list);

            auto res = minilua::math::asin(ctx);
            minilua::Number n = std::get<minilua::Number>(res);
            REQUIRE(n.as_float() == Approx(-0.5235987755983));

            // force value to nan, directly insert nan doesn't work
            auto result = res.force(minilua::Value(std::asin(2)));
            REQUIRE(result.has_value());

            CHECK(
                result.value().collect_first_alternative()[0] ==
                minilua::SourceChange(minilua::Range(), "nan"));
        }
    }

    SECTION("incorrect force") {
        double x = -0.5;
        minilua::Value value = minilua::Value(x).with_origin(minilua::LiteralOrigin());
        minilua::Vallist list = minilua::Vallist(value);
        ctx = ctx.make_new(list);

        auto res = minilua::math::asin(ctx);
        minilua::Number n = std::get<minilua::Number>(res);
        REQUIRE(n.as_float() == Approx(-0.5235987755983));

        // asin only returns Numbers, so the result can't be forced to a string, even if its
        // formated like a Number
        auto result = res.force("2");
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("reverse atan") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        SECTION("One parameter") {
            int x = 1;
            minilua::Value value = minilua::Value(x).with_origin(minilua::LiteralOrigin());
            minilua::Vallist list = minilua::Vallist({value, minilua::Nil()});
            ctx = ctx.make_new(list);
            minilua::Value res = minilua::math::atan(ctx);
            auto n = std::get<minilua::Number>(res);
            REQUIRE(n.as_float() == Approx(0.78539816339745));

            auto result = res.force(minilua::Value(0));
            REQUIRE(result.has_value());

            CHECK(
                result.value().collect_first_alternative()[0] ==
                minilua::SourceChange(minilua::Range(), "0"));
        }

        SECTION("Two parameters") {
            int x = 1;
            int y = 2;
            minilua::Value value1 = minilua::Value(x).with_origin(minilua::LiteralOrigin());
            minilua::Value value2 = minilua::Value(y).with_origin(minilua::LiteralOrigin());
            minilua::Vallist list = minilua::Vallist({value1, value2});
            ctx = ctx.make_new(list);
            minilua::Value res = minilua::math::atan(ctx);
            auto n = std::get<minilua::Number>(res);
            REQUIRE(n.as_float() == Approx(0.46364760900081));

            auto result = res.force(minilua::Value(0.64350110879328));
            REQUIRE(result.has_value());

            auto tree = result.value();
            tree.visit(minilua::overloaded{
                [](const minilua::SourceChangeAlternative& change) {
                    REQUIRE(change.changes.size() == 2);
                    CHECK(change.changes[0] == minilua::SourceChange(minilua::Range(), "-0.6"));
                    CHECK(change.changes[1] == minilua::SourceChange(minilua::Range(), "-0.8"));
                },
                [](const auto& /*unused*/) { FAIL("unexpected source change"); }});
        }
    }

    SECTION("invalid force") {
        SECTION("one parameter") {
            int x = 1;
            minilua::Value value = minilua::Value(x).with_origin(minilua::LiteralOrigin());
            minilua::Vallist list = minilua::Vallist({value, minilua::Nil()});
            ctx = ctx.make_new(list);
            minilua::Value res = minilua::math::atan(ctx);
            auto n = std::get<minilua::Number>(res);
            REQUIRE(n.as_float() == Approx(0.78539816339745));

            // atan only returns Numbers, so the result can't be forced to a string, even if its
            // formated like a Number
            auto result = res.force(minilua::Value("0"));
            CHECK_FALSE(result.has_value());
        }

        SECTION("two paramters") {
            int x = 1;
            int y = 2;
            minilua::Value value1 = minilua::Value(x).with_origin(minilua::LiteralOrigin());
            minilua::Value value2 = minilua::Value(y).with_origin(minilua::LiteralOrigin());
            minilua::Vallist list = minilua::Vallist({value1, value2});
            ctx = ctx.make_new(list);
            minilua::Value res = minilua::math::atan(ctx);
            auto n = std::get<minilua::Number>(res);
            REQUIRE(n.as_float() == Approx(0.46364760900081));

            // atan only returns Numbers, so the result can't be forced to a string, even if its
            // formated like a Number
            auto result = res.force(minilua::Value("0.64350110879328"));
            CHECK_FALSE(result.has_value());
        }
    }
}

TEST_CASE("reverse ceil") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        double i = 42.5;
        minilua::Value value = minilua::Value(i).with_origin(minilua::LiteralOrigin());
        minilua::Vallist list = minilua::Vallist(value);
        ctx = ctx.make_new(list);
        auto res = minilua::math::ceil(ctx);
        REQUIRE(res == minilua::Value(43));

        auto result = res.force(10);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "10"));
    }

    SECTION("invalid force") {
        double i = 42.5;
        minilua::Value value = minilua::Value(i).with_origin(minilua::LiteralOrigin());
        minilua::Vallist list = minilua::Vallist(value);
        ctx = ctx.make_new(list);
        auto res = minilua::math::ceil(ctx);
        REQUIRE(res == minilua::Value(43));

        // ceil only returns Numbers, so the result can't be forced to a string, even if its
        // formated like a Number
        auto result = res.force("10");
        CHECK_FALSE(result.has_value());

        // ceil only returns Numbers formated like an Integer, so the result can't be forced to a
        // float-value
        result = res.force(1.5);
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("reverse cos") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        int i = 0;
        minilua::Vallist list =
            minilua::Vallist({minilua::Value(i).with_origin(minilua::LiteralOrigin())});
        ctx = ctx.make_new(list);
        auto res = minilua::math::cos(ctx);
        REQUIRE(res == minilua::Value(1));

        auto result = res.force(0);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "1.5708"));

        result = res.force(3);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "nan"));
    }

    SECTION("invalid force") {
        int i = 0;
        minilua::Vallist list =
            minilua::Vallist({minilua::Value(i).with_origin(minilua::LiteralOrigin())});
        ctx = ctx.make_new(list);
        auto res = minilua::math::cos(ctx);
        REQUIRE(res == minilua::Value(1));

        auto result = res.force("0");
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("reverse deg") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        int i = 0;
        minilua::Vallist list =
            minilua::Vallist({minilua::Value(i).with_origin(minilua::LiteralOrigin())});
        ctx = ctx.make_new(list);
        auto res = minilua::math::deg(ctx);
        REQUIRE(res == minilua::Value(0));

        auto result = res.force(57.295779513082);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "1"));
    }

    SECTION("Invalid force") {
        int i = 0;
        minilua::Vallist list =
            minilua::Vallist({minilua::Value(i).with_origin(minilua::LiteralOrigin())});
        ctx = ctx.make_new(list);
        auto res = minilua::math::deg(ctx);
        REQUIRE(res == minilua::Value(0));

        auto result = res.force("25");
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("reverse exp") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        int i = 0;
        minilua::Vallist list =
            minilua::Vallist({minilua::Value(i).with_origin(minilua::LiteralOrigin())});
        ctx = ctx.make_new(list);
        auto res = minilua::math::exp(ctx);
        REQUIRE(res == minilua::Value(1));

        auto result = res.force(2.718281828459);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "1"));

        result = res.force(-2.718281828459);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "1"));
    }

    SECTION("invalid force") {
        int i = 0;
        minilua::Vallist list =
            minilua::Vallist({minilua::Value(i).with_origin(minilua::LiteralOrigin())});
        ctx = ctx.make_new(list);
        auto res = minilua::math::exp(ctx);
        REQUIRE(res == minilua::Value(1));

        auto result = res.force("1");
        CHECK_FALSE(result.has_value());

        result = res.force(0);
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("reverse floor") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        double i = 42.5;
        minilua::Vallist list =
            minilua::Vallist({minilua::Value(i).with_origin(minilua::LiteralOrigin())});
        ctx = ctx.make_new(list);
        auto res = minilua::math::floor(ctx);
        REQUIRE(res == minilua::Value(42));

        auto result = res.force(15);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "15"));
    }

    SECTION("invalid force") {
        double i = 42.5;
        minilua::Vallist list =
            minilua::Vallist({minilua::Value(i).with_origin(minilua::LiteralOrigin())});
        ctx = ctx.make_new(list);
        auto res = minilua::math::floor(ctx);
        REQUIRE(res == minilua::Value(42));

        auto result = res.force(15.5);
        REQUIRE_FALSE(result.has_value());

        result = res.force("15");
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("reverse fmod") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        SECTION("divisor is number") {
            double i = -2.5;
            double j = 4.2;
            minilua::Vallist list = minilua::Vallist(
                {minilua::Value(i).with_origin(minilua::LiteralOrigin()),
                 minilua::Value(j).with_origin(minilua::LiteralOrigin())});
            ctx = ctx.make_new(list);
            auto res = minilua::math::fmod(ctx);
            REQUIRE(res == minilua::Value(-2.5));

            auto result = res.force(1);
            REQUIRE(result.has_value());

            CHECK(
                result.value().collect_first_alternative()[0] ==
                minilua::SourceChange(minilua::Range(), "5.2"));
        }

        SECTION("divisor is string") {
            double i = -2.5;
            std::string j = "4.2";
            minilua::Vallist list = minilua::Vallist(
                {minilua::Value(i).with_origin(minilua::LiteralOrigin()),
                 minilua::Value(j).with_origin(minilua::LiteralOrigin())});
            ctx = ctx.make_new(list);
            auto res = minilua::math::fmod(ctx);
            REQUIRE(res == minilua::Value(-2.5));

            auto result = res.force(1);
            REQUIRE(result.has_value());

            CHECK(
                result.value().collect_first_alternative()[0] ==
                minilua::SourceChange(minilua::Range(), "5.2"));
        }
    }

    SECTION("invalid force") {
        double i = -2.5;
        double j = 4.2;
        minilua::Vallist list = minilua::Vallist(
            {minilua::Value(i).with_origin(minilua::LiteralOrigin()),
             minilua::Value(j).with_origin(minilua::LiteralOrigin())});
        ctx = ctx.make_new(list);
        auto res = minilua::math::fmod(ctx);
        REQUIRE(res == minilua::Value(-2.5));

        // fmod only returns Numbers, so the result can't be forced to a string, even if its
        // formated like a Number
        auto result = res.force("1");
        CHECK_FALSE(result.has_value());

        // fmod only returns Numbers that are smaller than the divisor.
        result = res.force(5);
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("reverse log") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        int x = 3;
        minilua::Vallist list = minilua::Vallist(
            {minilua::Value(x).with_origin(minilua::LiteralOrigin()), minilua::Nil()});
        ctx = ctx.make_new(list);
        auto res = minilua::math::log(ctx);
        auto n = std::get<minilua::Number>(res);
        REQUIRE(n.as_float() == Approx(1.0986122886681));

        auto result = res.force(0);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "1"));
    }

    SECTION("invalid force") {
        SECTION("try to force an invalid value") {
            int x = 3;
            minilua::Vallist list = minilua::Vallist(
                {minilua::Value(x).with_origin(minilua::LiteralOrigin()), minilua::Nil()});
            ctx = ctx.make_new(list);
            auto res = minilua::math::log(ctx);
            auto n = std::get<minilua::Number>(res);
            REQUIRE(n.as_float() == Approx(1.0986122886681));

            // log only returns numbers, so the result cant be forced to a string, even if its
            // formated like a number
            auto result = res.force("1");
            CHECK_FALSE(result.has_value());
        }

        SECTION("valid force, but log was called with 2 arguments") {
            int x = 3;
            int y = 2;
            minilua::Vallist list = minilua::Vallist(
                {minilua::Value(x).with_origin(minilua::LiteralOrigin()),
                 minilua::Value(y).with_origin(minilua::LiteralOrigin())});
            ctx = ctx.make_new(list);
            auto res = minilua::math::log(ctx);
            auto n = std::get<minilua::Number>(res);
            REQUIRE(n.as_float() == Approx(1.5849625007212));

            auto result = res.force(0);
            CHECK_FALSE(result.has_value());
        }
    }
}

TEST_CASE("reverse max") {
    // TODO: first implement reverse of max
}

TEST_CASE("reverse min") {
    // TODO: first implement reverse of min
}

TEST_CASE("reverse modf") {
    // TODO: first implement reverse for return-values of modf
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {}

    SECTION("invalid force") {}
}

TEST_CASE("reverse rad") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        int i = 1;
        minilua::Vallist list =
            minilua::Vallist(minilua::Value(i).with_origin(minilua::LiteralOrigin()));
        ctx = ctx.make_new(list);
        auto res = minilua::math::rad(ctx);
        minilua::Number n = std::get<minilua::Number>(res);
        REQUIRE(n.as_float() == Approx(0.017453292519943));

        auto result = res.force(0.043633231299858);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "2.5"));
    }

    SECTION("invalid force") {
        int i = 1;
        minilua::Vallist list =
            minilua::Vallist(minilua::Value(i).with_origin(minilua::LiteralOrigin()));
        ctx = ctx.make_new(list);
        auto res = minilua::math::rad(ctx);
        minilua::Number n = std::get<minilua::Number>(res);
        REQUIRE(n.as_float() == Approx(0.017453292519943));

        // rad only returns Numbers, so the result can't be forced to a string, even if its formated
        // like a Number
        auto result = res.force("24");
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("reverse sin") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        int i = 0;
        minilua::Vallist list =
            minilua::Vallist(minilua::Value(i).with_origin(minilua::LiteralOrigin()));
        ctx = ctx.make_new(list);
        auto res = minilua::math::sin(ctx);
        REQUIRE(res == minilua::Value(0));

        auto result = res.force(1);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "1.5708"));

        result = res.force(3);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "nan"));
    }

    SECTION("invalid force") {
        int i = 0;
        minilua::Vallist list =
            minilua::Vallist(minilua::Value(i).with_origin(minilua::LiteralOrigin()));
        ctx = ctx.make_new(list);
        auto res = minilua::math::sin(ctx);
        REQUIRE(res == minilua::Value(0));

        // sin only returns Numbers, so the result can't be forced to a string, even if it's
        // formated like a Number
        auto result = res.force("1");
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("reverse sqrt") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        int i = 1;
        minilua::Value v = minilua::Value(i).with_origin(minilua::LiteralOrigin());
        minilua::Vallist list(v);
        ctx = ctx.make_new(list);
        auto res = minilua::math::sqrt(ctx);
        REQUIRE(res == minilua::Value(1));

        auto result = res.force(2);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "4"));
    }

    SECTION("invalid force") {
        int i = 1;
        minilua::Value v = minilua::Value(i).with_origin(minilua::LiteralOrigin());
        minilua::Vallist list(v);
        ctx = ctx.make_new(list);
        auto res = minilua::math::sqrt(ctx);
        REQUIRE(res == minilua::Value(1));

        // sqrt only returns Numbers, so the result can't be forced to a string, even if it's
        // formated like a Number
        auto result = res.force("2");
        CHECK_FALSE(result.has_value());

        // sqrt only returns positive Numbers, so the result can't be forced to an negative Number
        result = res.force(-1);
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("reverse tan") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        int i = 0;
        minilua::Vallist list =
            minilua::Vallist(minilua::Value(i).with_origin(minilua::LiteralOrigin()));
        ctx = ctx.make_new(list);
        auto res = minilua::math::tan(ctx);
        REQUIRE(res == minilua::Value(0));

        auto result = res.force(1.3386902103512);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "0.929219"));
    }

    SECTION("invalid force") {
        int i = 0;
        minilua::Vallist list =
            minilua::Vallist(minilua::Value(i).with_origin(minilua::LiteralOrigin()));
        ctx = ctx.make_new(list);
        auto res = minilua::math::tan(ctx);
        REQUIRE(res == minilua::Value(0));

        // tan only returns Numbers, so the result can't be forced to a string, even if it's
        // formated like a Number
        auto result = res.force("42");
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("reverse to_integer") {
    minilua::Environment env;
    minilua::CallContext ctx(&env);

    SECTION("valid force") {
        int i = 0;
        minilua::Value v = minilua::Value(i).with_origin(minilua::LiteralOrigin());
        minilua::Vallist list({v});
        ctx = ctx.make_new(list);
        auto res = minilua::math::to_integer(ctx);
        REQUIRE(res == minilua::Value(i));

        auto result = res.force(10);
        REQUIRE(result.has_value());

        CHECK(
            result.value().collect_first_alternative()[0] ==
            minilua::SourceChange(minilua::Range(), "10"));
    }

    SECTION("invalid force") {
        int i = 0;
        minilua::Value v = minilua::Value(i).with_origin(minilua::LiteralOrigin());
        minilua::Vallist list({v});
        ctx = ctx.make_new(list);
        auto res = minilua::math::to_integer(ctx);
        REQUIRE(res == minilua::Value(i));

        // to_integer only returns Numbers, so the result can't be forced to a string, even if it's
        // formated like a Number
        auto result = res.force("10");
        CHECK_FALSE(result.has_value());

        // to_integer only returns Numbers formated like Integers, so the result can't be forced to
        // a float
        result = res.force(4.2);
        CHECK_FALSE(result.has_value());
    }
}
