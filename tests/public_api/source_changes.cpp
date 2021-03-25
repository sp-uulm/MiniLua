#include "MiniLua/source_change.hpp"
#include <catch2/catch.hpp>

#include <MiniLua/MiniLua.hpp>

TEST_CASE("Simplify SourceChangeTree") {
    SECTION("empty tree") {
        REQUIRE(minilua::simplify(std::nullopt) == std::nullopt);
        REQUIRE(minilua::simplify(minilua::SourceChangeAlternative()) == std::nullopt);
        REQUIRE(minilua::simplify(minilua::SourceChangeCombination()) == std::nullopt);
    }

    SECTION("nested empty tree") {
        REQUIRE(
            minilua::simplify(minilua::SourceChangeCombination(
                {minilua::SourceChangeAlternative(),
                 minilua::SourceChangeAlternative({minilua::SourceChangeCombination()})})) ==
            std::nullopt);

        REQUIRE(
            minilua::simplify(minilua::SourceChangeCombination({
                minilua::SourceChangeAlternative(),
                minilua::SourceChangeAlternative(),
                minilua::SourceChangeAlternative(),
                minilua::SourceChangeAlternative(),
            })) == std::nullopt);
    }

    SECTION("simple one item tree") {
        REQUIRE(
            minilua::simplify(minilua::SourceChange({{1, 2, 3}}, "123")) ==
            minilua::SourceChange({{1, 2, 3}}, "123"));
    }

    SECTION("nested single items") {
        const auto item = minilua::SourceChange({{1, 2, 3}, {4, 5, 6}}, "123");

        REQUIRE(minilua::simplify(minilua::SourceChangeAlternative({item})) == item);
        REQUIRE(minilua::simplify(minilua::SourceChangeCombination({item})) == item);
        REQUIRE(
            minilua::simplify(minilua::SourceChangeAlternative(
                {minilua::SourceChangeAlternative({item})})) == item);
        REQUIRE(
            minilua::simplify(minilua::SourceChangeCombination(
                {minilua::SourceChangeAlternative({item})})) == item);
    }

    SECTION("multiple nested items") {
        const auto item1 = minilua::SourceChange({{1, 2, 3}, {4, 5, 6}}, "123");
        const auto item2 = minilua::SourceChange({{7, 8, 9}, {8, 9, 9}}, "abc");

        REQUIRE(
            minilua::simplify(minilua::SourceChangeAlternative({item1, item2})) ==
            minilua::SourceChangeAlternative({item1, item2}));

        REQUIRE(
            minilua::simplify(minilua::SourceChangeAlternative({
                minilua::SourceChangeAlternative({item1}),
                minilua::SourceChangeAlternative({item2}),
            })) == minilua::SourceChangeAlternative({item1, item2}));

        REQUIRE(
            minilua::simplify(minilua::SourceChangeCombination({
                minilua::SourceChangeAlternative({item1}),
                minilua::SourceChangeAlternative({item2}),
            })) == minilua::SourceChangeCombination({item1, item2}));
    }
}
