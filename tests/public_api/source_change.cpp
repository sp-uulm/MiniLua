#include <MiniLua/source_change.hpp>
#include <catch2/catch.hpp>

TEST_CASE("minilua::Location") {
    minilua::Location loc1{
        .line = 5, // NOLINT
        .column = 0,
        .byte = 25 // NOLINT
    };
    CHECK(loc1 == minilua::Location{.line = 5, .column = 0, .byte = 25});
}

TEST_CASE("minilua::Range") {
    minilua::Location loc1{
        .line = 5, // NOLINT
        .column = 0,
        .byte = 25 // NOLINT
    };
    minilua::Location loc2{
        .line = 5,   // NOLINT
        .column = 7, // NOLINT
        .byte = 32   // NOLINT
    };
    minilua::Range range{
        .start = loc1,
        .end = loc2,
    };
    CHECK(
        range == minilua::Range{
                     .start =
                         {
                             .line = 5,
                             .column = 0,
                             .byte = 25,
                         },
                     .end = {
                         .line = 5,
                         .column = 7,
                         .byte = 32,
                     }});
}

TEST_CASE("minilua::SourceChange") {
    auto change =
        minilua::SourceChange(minilua::Range{{0, 0, 0}, {0, 5, 5}}, "replacement"); // NOLINT
    change.hint = "hint";
    change.origin = "origin";
    REQUIRE(change == change);
    minilua::SourceChangeTree source_change{change};

    REQUIRE(source_change.origin() == "origin");
    REQUIRE(source_change.hint() == "hint");
    REQUIRE(source_change == source_change);

    auto change2 =
        minilua::SourceChange(minilua::Range{{0, 0, 0}, {0, 5, 5}}, "replacement"); // NOLINT
    change2.hint = "hint";
    change2.origin = "origin";
    minilua::SourceChangeTree source_change2{change2};
    REQUIRE(source_change == source_change2);

    auto combined_change = minilua::SourceChangeCombination({source_change, source_change2});
    minilua::SourceChangeTree source_change3{combined_change};
    source_change3.origin() = "new_origin";
    source_change3.hint() = "new_hint";

    // kind, range, replacement, origin, hint
    std::vector<std::tuple<std::string, minilua::Range, std::string, std::string, std::string>>
        visited_elements;

    source_change3.visit_all([&visited_elements](const minilua::SourceChange& change) {
        visited_elements.emplace_back(
            "SourceChange", change.range, change.replacement, change.origin, change.hint);
    });

    REQUIRE_THAT(
        visited_elements,
        Catch::Matchers::UnorderedEquals(
            std::vector<
                std::tuple<std::string, minilua::Range, std::string, std::string, std::string>>{
                {"SourceChange", minilua::Range{{0, 0, 0}, {0, 5, 5}}, "replacement", "origin",
                 "hint"},
                {"SourceChange", minilua::Range{{0, 0, 0}, {0, 5, 5}}, "replacement", "origin",
                 "hint"}}));
}
