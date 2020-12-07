#include <MiniLua/utils.hpp>
#include <catch2/catch.hpp>

#include <sstream>

using Catch::Matchers::Matches;

TEST_CASE("owning_ptr is constructable via make_owning") {
    minilua::owning_ptr<std::string> x = minilua::make_owning<std::string>("hi"); // NOLINT
    REQUIRE(*x.get() == "hi");
    REQUIRE(*x == "hi");

    SECTION("without default constructor") {
        struct X {
            X() = delete;
        };

        minilua::owning_ptr<X> x = minilua::make_owning<X>(X{});
        REQUIRE(x.get() != nullptr);
    }
}

TEST_CASE("owning_ptr can't be constructed from a nullptr") {
    REQUIRE_THROWS(minilua::owning_ptr<int>(nullptr));
}

TEST_CASE("owning_ptr is copy constructable") {
    const minilua::owning_ptr<std::string> x = minilua::make_owning<std::string>("hi"); // NOLINT

    SECTION("via copy constructor") {
        const minilua::owning_ptr<std::string> y{x}; // NOLINT
        REQUIRE(x == y);
        REQUIRE(x.get() != y.get()); // pointer
        REQUIRE(*x == *y);           // reference
        REQUIRE(&*x != &*y);         // pointer
    }

    SECTION("via copy assignment") {
        minilua::owning_ptr<std::string> y;
        y = x;
        REQUIRE(x == y);
        REQUIRE(x.get() != y.get()); // pointer
        REQUIRE(*x == *y);           // reference
        REQUIRE(&*x != &*y);         // pointer
    }
}

TEST_CASE("owning_ptr is move constructable") {
    minilua::owning_ptr<std::string> x = minilua::make_owning<std::string>("hi"); // NOLINT

    SECTION("via move constructor") {
        const minilua::owning_ptr<std::string> y{std::move(x)}; // NOLINT
        REQUIRE(x != y);
        REQUIRE(*x != *y);
    }

    SECTION("via move assignment") {
        minilua::owning_ptr<std::string> x = minilua::make_owning<std::string>("hi"); // NOLINT
        minilua::owning_ptr<std::string> y;
        y = std::move(x);
        REQUIRE(x != y);
        REQUIRE(*x != *y);
    }
}

TEST_CASE("owning_ptr is printable") {
    SECTION("for printable types") {
        minilua::owning_ptr<std::string> x = minilua::make_owning<std::string>("hi");
        std::stringstream ss;
        ss << x;
        REQUIRE(ss.str() == "owning_ptr(hi)");
    }

    SECTION("for non printable types") {
        struct X {};
        minilua::owning_ptr<X> x;
        std::stringstream ss;
        ss << x;
        REQUIRE_THAT(ss.str(), Matches(R"#(owning_ptr\(0x[0-9a-f]+\))#"));
    }
}
