#include <catch2/catch.hpp>
#include <type_traits>

#include "internal_env.hpp"

TEST_CASE("Internal Env is copyable") {
    static_assert(std::is_copy_assignable_v<minilua::Env>);
    static_assert(std::is_copy_constructible_v<minilua::Env>);
}

TEST_CASE("Internal Env get/set local variables") {
    minilua::Env env;
    env.set_local("local_var", 2);
    REQUIRE(env.get_local("local_var") == 2);

    minilua::Env env_copy{env};
    CHECK(env_copy.get_local("local_var") == 2);

    SECTION("changing the copied local env does not change the original local env") {
        env_copy.set_local("local_var2", "hi");
        REQUIRE(env_copy.get_local("local_var2") == "hi");
        CHECK(env.get_local("local_var2") == std::nullopt);
    }

    SECTION("redefining local variables") {
        auto original = env.local().at("local_var");

        env.declare_local("local_var");
        CHECK(env.local().at("local_var") != original);

        env.set_local("local_var", 21); // NOLINT
        CHECK(*env.local().at("local_var") != *original);
    }
}

TEST_CASE("Internal Env get/set global variables") {
    minilua::Env env;
    env.set_global("var", 2);
    REQUIRE(env.get_global("var") == 2);
}

TEST_CASE("Internal Env get/set variables") {
    minilua::Env env;
    env.set_var("var1", 22); // NOLINT
    REQUIRE(env.get_global("var1") == 22);
    CHECK(env.get_var("var1") == 22);

    env.declare_local("var2");
    env.set_var("var2", 17); // NOLINT
    REQUIRE(env.get_local("var2") == 17);
    CHECK(env.get_var("var2") == 17);
}

TEST_CASE("to_string_with_base") {
    CHECK(minilua::to_string_with_base(15, 10) == "15");
    CHECK(minilua::to_string_with_base(15, 16) == "F");
    CHECK(minilua::to_string_with_base(15, 8) == "17");
    CHECK(minilua::to_string_with_base(72, 3) == "2200");
    CHECK(minilua::to_string_with_base(72, 2) == "1001000");
    CHECK(minilua::to_string_with_base(72, 7) == "132");
    CHECK(minilua::to_string_with_base(1237817389, 35) == "NJUD64");
    CHECK(minilua::to_string_with_base(-88, 12) == "-74");
}
