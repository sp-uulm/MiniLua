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
    auto change = minilua::SCSingle(minilua::Range{{0, 0, 0}, {0, 5, 5}}, "replacement"); // NOLINT
    change.hint = "hint";
    change.origin = "origin";
    INFO(change);
    REQUIRE(change == change);
    minilua::SourceChange source_change{change};

    INFO(source_change);
    REQUIRE(source_change.origin() == "origin");
    REQUIRE(source_change.hint() == "hint");
    REQUIRE(source_change == source_change);

    auto change2 = minilua::SCSingle(minilua::Range{{0, 0, 0}, {0, 5, 5}}, "replacement"); // NOLINT
    change2.hint = "hint";
    change2.origin = "origin";
    INFO(change2);
    minilua::SourceChange source_change2{change2};
    REQUIRE(source_change == source_change2);

    auto combined_change = minilua::SCAnd({source_change, source_change2});
    INFO(combined_change);
    minilua::SourceChange source_change3{combined_change};
    INFO(source_change3);
}
