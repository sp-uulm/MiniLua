#include <MiniLua/values.hpp>
#include <catch2/catch.hpp>

TEST_CASE("reversing Origin from addition") {
    minilua::Value lhs = minilua::Value(25).with_origin(
        minilua::LiteralOrigin{.location = minilua::Range()}); // NOLINT
    minilua::Value rhs = minilua::Value(13).with_origin(
        minilua::LiteralOrigin{.location = minilua::Range()}); // NOLINT

    minilua::Value res = lhs + rhs;
    REQUIRE(res == 38);

    // force 38 to become 27 => -11
    auto opt_source_changes = res.force(27); // NOLINT
    REQUIRE(opt_source_changes.has_value());
    auto source_changes = opt_source_changes.value();

    INFO(source_changes);

    source_changes.visit(minilua::overloaded{
        [](const minilua::SourceChangeAlternative& change) {
            CHECK(change.origin == "add");
            CHECK(change.changes.size() == 2);
            change.changes[0].visit(minilua::overloaded{
                [](const minilua::SourceChange& inner_change) {
                    CHECK(inner_change.replacement == "14"); // 25 - 11
                },
                [](const auto&) { FAIL("unexpected inner element"); }});
            change.changes[1].visit(minilua::overloaded{
                [](const minilua::SourceChange& inner_change) {
                    CHECK(inner_change.replacement == "2"); // 13 - 11
                },
                [](const auto&) { FAIL("unexpected inner element"); }});
        },
        [](const auto&) { FAIL("unexpected element"); }});
}
