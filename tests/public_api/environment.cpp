#include <MiniLua/MiniLua.hpp>
#include <catch2/catch.hpp>

#include <sstream>

TEST_CASE("Environment is constructable") {
    SECTION("from unordered_map") {
        std::unordered_map<std::string, int> map;
        map.insert_or_assign("hi", 25); // NOLINT
        std::unordered_map<std::string, int> map2{std::move(map)};
        REQUIRE_THROWS_AS(map.at("hi"), std::out_of_range);
        REQUIRE(map.empty());
        REQUIRE(map2.at("hi") == 25);
    }

    SECTION("default construction is empty") {
        minilua::Environment env;
        REQUIRE(env.size() == 0);
    }
}

TEST_CASE("Environment is copyable") {
    static_assert(std::is_copy_constructible<minilua::Environment>());
    static_assert(std::is_copy_assignable<minilua::Environment>());

    minilua::Environment env;
    env.add("val1", 24); // NOLINT

    SECTION("via copy constructor") {
        minilua::Environment env_copy{static_cast<const minilua::Environment&>(env)}; // NOLINT
        REQUIRE(env == env_copy);
    }

    SECTION("via copy assignment") {
        minilua::Environment env_copy2; // NOLINT
        env_copy2 = static_cast<const minilua::Environment&>(env);
        REQUIRE(env == env_copy2);
    }
}

TEST_CASE("Environment can be moved") {
    static_assert(std::is_move_constructible<minilua::Environment>());
    static_assert(std::is_move_assignable<minilua::Environment>());

    SECTION("via move constructor") {
        minilua::Environment env;
        env.add("val1", 24); // NOLINT

        INFO(env);

        minilua::Environment env2{std::move(env)}; // NOLINT

        REQUIRE(!env.has("val1"));
        REQUIRE(env.get("val1") == minilua::Nil());
        REQUIRE(env2.has("val1"));
        REQUIRE(env2.get("val1") == 24);
        REQUIRE(env != env2);
    }

    SECTION("via move constructor") {
        minilua::Environment env;
        env.add("val1", 24); // NOLINT

        minilua::Environment env2;
        env2 = std::move(env);
        REQUIRE(env.get("val1") == minilua::Nil());
        REQUIRE(env2.get("val1") == 24);
        REQUIRE(env != env2);
    }
}

TEST_CASE("Environments can be swapped") {
    static_assert(std::is_swappable<minilua::Environment>());
    minilua::Environment env;
    env.add("val1", 24); // NOLINT
    minilua::Environment env2;

    swap(env, env2);
    REQUIRE(env.get("val1") == minilua::Nil());
    REQUIRE(env2.get("val1") == 24);
    REQUIRE(env != env2);
}

TEST_CASE("Environment contains the inserted value") {
    SECTION("from single insertions") {
        minilua::Environment env;

        env.add("val1", 24); // NOLINT
        REQUIRE(env.size() == 1);
        REQUIRE(env.get("val1") == 24);

        std::string key = "val2";
        env.add(key, 35); // NOLINT
        REQUIRE(env.size() == 2);
        REQUIRE(env.get("val2") == 35);
    }

    SECTION("from mass insertion") {
        minilua::Environment env;

        SECTION("via initializer list of pairs") {
            env.add_all({
                {"val1", 24}, // NOLINT
                {"val2", 35}, // NOLINT
            });
            REQUIRE(env.size() == 2);
            REQUIRE(env.get("val1") == 24);
            REQUIRE(env.get("val2") == 35);
        }

        SECTION("via unordered_map") {
            std::unordered_map<std::string, minilua::Value> map{
                {"val3", 66}, // NOLINT
                {"val4", 17}, // NOLINT
            };
            env.add_all(map);
            REQUIRE(env.size() == 2);
            REQUIRE(env.get("val3") == 66);
            REQUIRE(env.get("val4") == 17);
        }

        SECTION("via vector of pairs") {
            env.add_all(std::vector<std::pair<std::string, minilua::Value>>{
                std::make_pair("val5", 226), // NOLINT
                std::make_pair("val6", 16),  // NOLINT
            });
            REQUIRE(env.size() == 2);
            REQUIRE(env.get("val5") == 226);
            REQUIRE(env.get("val6") == 16);
        }
    }
}

TEST_CASE("setting I/O in Environment") {
    minilua::Environment env;

    std::stringstream in;
    env.set_stdin(&in);
    REQUIRE(&in == env.get_stdin());

    std::stringstream out;
    env.set_stdout(&out);
    REQUIRE(&out == env.get_stdout());

    std::stringstream err;
    env.set_stderr(&err);
    REQUIRE(&err == env.get_stderr());

    SECTION("nullptr as I/O is not allowed") {
        minilua::Environment env;
        REQUIRE_THROWS(env.set_stdin(nullptr));
        REQUIRE_THROWS(env.set_stdout(nullptr));
        REQUIRE_THROWS(env.set_stderr(nullptr));
    }
}
