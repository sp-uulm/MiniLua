#include <catch2/catch.hpp>
#include <type_traits>

#include "internal_env.hpp"

TEST_CASE("Internal Env") {
    // most is already tested through the public api (Environment)

    SECTION("is copyable") {
        static_assert(std::is_copy_assignable_v<minilua::Env>);
        static_assert(std::is_copy_constructible_v<minilua::Env>);

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
            std::shared_ptr<minilua::Value> original = env.local().at("local_var");

            env.declare_local("local_var");

            CHECK(env.local().at("local_var") != original);
        }
    }
}
