#include "MiniLua/source_change.hpp"
#include <MiniLua/environment.hpp>
#include <MiniLua/values.hpp>
#include <catch2/catch.hpp>
#include <cmath>
#include <memory>
#include <utility>

auto make_call_context(minilua::Vallist args, std::optional<minilua::Range> location = std::nullopt)
    -> minilua::CallContext {
    static minilua::Environment env;
    static minilua::CallContext ctx(&env);
    return ctx.make_new(std::move(args), location);
}

TEST_CASE("reversing Origin from addition") {
    minilua::Value lhs = minilua::Value(25) // NOLINT
                             .with_origin(minilua::LiteralOrigin{.location = minilua::Range()});
    minilua::Value rhs = minilua::Value(13) // NOLINT
                             .with_origin(minilua::LiteralOrigin{.location = minilua::Range()});

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

auto sqrt_impl(const minilua::CallContext& ctx) -> minilua::Value {
    return minilua::UnaryNumericFunctionHelper{
        [](minilua::Number arg) { return std::sqrt(arg.as_float()); },
        [](minilua::Number num) { return num.as_float() * num.as_float(); }}(ctx);
}

TEST_CASE("define correct origin for unary math functions and force value") {
    minilua::Value val = minilua::Value(25) // NOLINT
                             .with_origin(minilua::LiteralOrigin{.location = minilua::Range()});

    const auto ctx = make_call_context({val});
    auto res = sqrt_impl(ctx);
    REQUIRE(res == 5);

    REQUIRE(res.has_origin());
    INFO(res.origin());
    auto source_change_tree = res.force(3).value(); // NOLINT
    auto source_changes = source_change_tree.collect_first_alternative();
    CHECK(source_changes[0].replacement == "9.0");
}

auto pow_impl(const minilua::CallContext& ctx) -> minilua::Value {
    return minilua::BinaryNumericFunctionHelper{
        [](minilua::Number lhs, minilua::Number rhs) {
            return std::pow(lhs.as_float(), rhs.as_float());
        },
        [](minilua::Number new_value, minilua::Number old_rhs) {
            return std::pow(new_value.as_float(), 1 / old_rhs.as_float());
        },
        [](minilua::Number new_value, minilua::Number old_lhs) {
            return std::log(new_value.as_float()) / std::log(old_lhs.as_float());
        }}(ctx);
}

TEST_CASE("define correct origin for binary math functions and force value") {
    minilua::Value base = minilua::Value(8) // NOLINT
                              .with_origin(minilua::LiteralOrigin{.location = minilua::Range()});
    minilua::Value exp = minilua::Value(3) // NOLINT
                             .with_origin(minilua::LiteralOrigin{.location = minilua::Range()});

    const auto ctx = make_call_context({base, exp});
    auto res = pow_impl(ctx);
    REQUIRE(res == 512); // NOLINT

    REQUIRE(res.has_origin());
    auto source_change_tree = res.force(64).value(); // NOLINT
    source_change_tree.visit(minilua::overloaded{
        [](const minilua::SourceChangeAlternative& change) {
            // CHECK(change.origin == "add");
            CHECK(change.changes.size() == 2);
            change.changes[0].visit(minilua::overloaded{
                [](const minilua::SourceChange& inner_change) {
                    CHECK(inner_change.replacement == "4"); // -> 4^3 = 64
                },
                [](const auto&) { FAIL("unexpected inner element"); }});
            change.changes[1].visit(minilua::overloaded{
                [](const minilua::SourceChange& inner_change) {
                    CHECK(inner_change.replacement == "2.0"); // -> 8^2 = 64
                },
                [](const auto&) { FAIL("unexpected inner element"); }});
        },
        [](const auto&) { FAIL("unexpected element"); }});
}

TEST_CASE("reversing Origin from not") {
    minilua::Value value = minilua::Value(true) // NOLINT
                               .with_origin(minilua::LiteralOrigin{.location = minilua::Range()});

    minilua::Value res = !value;
    REQUIRE(res == false);

    // force 38 to become 27 => -11
    auto opt_source_changes = res.force(true); // NOLINT
    REQUIRE(opt_source_changes.has_value());
    auto source_changes = opt_source_changes.value();

    INFO(source_changes);

    source_changes.visit(minilua::overloaded{
        [](const minilua::SourceChange& change) {
            // CHECK(change.origin == "neg");
            CHECK(change.replacement == "false"); // !false = true
        },
        [](const auto&) { FAIL("unexpected element"); }});
}

TEST_CASE("Simplify Origin") {
    // note simplifying origin is not recursive

    SECTION("Empty Origin tree") { REQUIRE(minilua::Origin().simplify() == minilua::NoOrigin()); }

    SECTION("Single item tree") {
        REQUIRE(minilua::Origin(minilua::ExternalOrigin()).simplify() == minilua::ExternalOrigin());

        const auto item = minilua::LiteralOrigin{{{1, 2, 3}, {4, 5, 6}}};
        REQUIRE(minilua::Origin(item).simplify() == item);
    }

    SECTION("Empty nested tree") {
        REQUIRE(minilua::Origin(minilua::UnaryOrigin{}).simplify() == minilua::NoOrigin());
        REQUIRE(minilua::Origin(minilua::BinaryOrigin{}).simplify() == minilua::NoOrigin());
        REQUIRE(minilua::Origin(minilua::MultipleArgsOrigin{}).simplify() == minilua::NoOrigin());
    }

    SECTION("Incomplete branch") {
        REQUIRE(
            minilua::Origin(minilua::BinaryOrigin{
                                .lhs = std::make_shared<minilua::Value>(
                                    minilua::Value().with_origin(minilua::ExternalOrigin())),
                                .rhs = std::make_shared<minilua::Value>(minilua::Nil()),
                            })
                .simplify() == minilua::NoOrigin());

        REQUIRE(
            minilua::Origin(
                minilua::MultipleArgsOrigin{
                    .values = std::make_shared<minilua::Vallist>(minilua::Vallist(
                        {minilua::Value().with_origin(minilua::ExternalOrigin()), minilua::Nil()})),
                })
                .simplify() == minilua::NoOrigin());
    }
}
