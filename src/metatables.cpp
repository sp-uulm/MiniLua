#include <string>
#include <utility>

#include "MiniLua/metatables.hpp"

namespace minilua::mt {

using namespace std::string_literals;

auto index(const CallContext& ctx) -> CallResult {
    auto arg1 = ctx.arguments().get(0);

    return std::visit(
        overloaded{
            [&ctx](const Table& table) -> CallResult {
                auto key = ctx.arguments().get(1);
                auto value = table.get(key);

                if (value.is_nil() && table.get_metatable()) {
                    auto metatable = table.get_metatable().value();
                    auto index_event = metatable.get("__index");

                    if (index_event.is_function()) {
                        return index_event.call(ctx.make_new({table, key})).one_value();
                    } else if (index_event.is_table()) {
                        // NOTE: This might trigger another metatable
                        return index(ctx.make_new({index_event, key}));
                    }
                }

                return CallResult({value});
            },
            [](const auto& value) -> CallResult {
                throw std::runtime_error("can't index into " + std::string(value.TYPE));
            },
        },
        arg1.raw());
}

auto newindex(const CallContext& ctx) -> CallResult {
    auto arg1 = ctx.arguments().get(0);

    return std::visit(
        overloaded{
            [&ctx](Table table) -> CallResult {
                auto key = ctx.arguments().get(1);
                auto new_value = ctx.arguments().get(2);

                auto old_value = table.get(key);

                if (old_value.is_nil() && table.get_metatable()) {
                    auto metatable = table.get_metatable().value();
                    auto newindex_event = metatable.get("__newindex");

                    if (newindex_event.is_function()) {
                        return newindex_event.call(ctx.make_new({table, key, new_value}))
                            .one_value();
                    } else if (newindex_event.is_table()) {
                        // NOTE: This might trigger another metatable
                        return newindex(ctx.make_new({newindex_event, key, new_value}));
                    }
                }

                // if it was not handled by the metatable we just set the value
                table.set(key, new_value);

                return CallResult();
            },
            [](const auto& value) -> CallResult {
                throw std::runtime_error("can't index into " + std::string(value.TYPE));
            },
        },
        arg1.raw());
}

auto call(const CallContext& ctx) -> CallResult {
    auto arg1 = ctx.arguments().get(0);

    return std::visit(
        overloaded{
            [&ctx](const Table& table) -> CallResult {
                auto metamethod = table.get_metamethod("__call");
                if (metamethod.is_function()) {
                    return metamethod.call(ctx);
                }

                throw std::runtime_error("attempted to call a table value");
            },
            [&ctx](const Function& value) -> CallResult {
                std::vector<Value> arguments(ctx.arguments().begin() + 1, ctx.arguments().end());
                auto new_ctx = ctx.make_new(arguments, ctx.call_location());
                return value.call(new_ctx);
            },
            [](const auto& value) -> CallResult {
                throw std::runtime_error(
                    "attempted to call a "s + std::string(value.TYPE) + " value");
            },
        },
        arg1.raw());
}

// NOTE: This abomination means:
// pointer to a method of class Value with the signature
// auto method(const Value&, std::optional<Range>) const -> Value;
using BinaryValueMethod = Value (Value::*)(const Value&, std::optional<Range>) const;

static auto _binary_metamethod(
    const CallContext& ctx, std::optional<Range> location, const std::string& metamethod,
    BinaryValueMethod method, const std::string& op_kind = "arithmetic") -> CallResult {
    auto arg1 = ctx.arguments().get(0);
    auto arg2 = ctx.arguments().get(1);

    return std::visit(
        overloaded{
            [&ctx, &metamethod, &op_kind](const Table& arg1, const Table& arg2) -> CallResult {
                {
                    auto meta_left = arg1.get_metamethod(metamethod);
                    if (meta_left.is_function()) {
                        return meta_left.call(ctx);
                    }
                }

                {
                    auto meta_right = arg2.get_metamethod(metamethod);
                    if (meta_right.is_function()) {
                        return meta_right.call(ctx);
                    }
                }

                throw std::runtime_error(
                    "attempt to perform "s + op_kind + " on a table value (both)");
            },
            [&ctx, &metamethod, &op_kind](const Table& arg1, const auto& /*arg2*/) -> CallResult {
                auto meta_left = arg1.get_metamethod(metamethod);
                if (meta_left.is_function()) {
                    return meta_left.call(ctx);
                }

                throw std::runtime_error(
                    "attempt to perform "s + op_kind + " on a table value (light)");
            },
            [&ctx, &metamethod, &op_kind](const auto& /*arg1*/, const Table& arg2) -> CallResult {
                auto meta_right = arg2.get_metamethod(metamethod);
                if (meta_right.is_function()) {
                    return meta_right.call(ctx);
                }

                throw std::runtime_error(
                    "attempt to perform "s + op_kind + " on a table value (right)");
            },
            [&location, &method, &arg1, &arg2](const auto&, const auto&) -> CallResult {
                Value value = (arg1.*method)(arg2, location);
                return CallResult({value});
            },
        },
        arg1.raw(), arg2.raw());
}

auto add(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(ctx, std::move(location), "__add", &Value::add);
}

auto sub(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(ctx, std::move(location), "__sub", &Value::sub);
}

auto mul(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(ctx, std::move(location), "__mul", &Value::mul);
}

auto div(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(ctx, std::move(location), "__div", &Value::div);
}

auto mod(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(ctx, std::move(location), "__mod", &Value::mod);
}

auto pow(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(ctx, std::move(location), "__pow", &Value::pow);
}

auto idiv(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(ctx, std::move(location), "__idiv", &Value::int_div);
}

auto band(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(
        ctx, std::move(location), "__band", &Value::bit_and, "bitwise operation");
}

auto bor(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(
        ctx, std::move(location), "__bor", &Value::bit_or, "bitwise operation");
}

auto bxor(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(
        ctx, std::move(location), "__bxor", &Value::bit_xor, "bitwise operation");
}

auto shl(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(
        ctx, std::move(location), "__shl", &Value::bit_shl, "bitwise operation");
}

auto shr(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(
        ctx, std::move(location), "__shr", &Value::bit_shr, "bitwise operation");
}

auto concat(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(ctx, std::move(location), "__concat", &Value::concat, "concatenate");
}

static auto _force_bool(const CallResult& result) -> CallResult {
    auto value = Vallist({result.values().get(0)});
    return CallResult(value, result.source_change());
}

auto eq(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    auto arg1 = ctx.arguments().get(0);
    auto arg2 = ctx.arguments().get(1);

    auto result = std::visit(
        overloaded{
            [&ctx](const Table& arg1, const Table& arg2) -> CallResult {
                // first check if they are trivially equal
                if (arg1 == arg2) {
                    return CallResult({true});
                }

                {
                    auto meta_left = arg1.get_metamethod("__eq");
                    if (meta_left.is_function()) {
                        return meta_left.call(ctx);
                    }
                }

                {
                    auto meta_right = arg2.get_metamethod("__eq");
                    if (meta_right.is_function()) {
                        return meta_right.call(ctx);
                    }
                }

                return CallResult({false});
            },
            [&location](const auto& arg1, const auto& arg2) -> CallResult {
                Value value = Value(arg1).equals(arg2, location);
                return CallResult({value});
            },
        },
        arg1.raw(), arg2.raw());

    return _force_bool(result);
}

auto lt(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _force_bool(_binary_metamethod(ctx, std::move(location), "__lt", &Value::less_than));
}

auto le(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    auto arg1 = ctx.arguments().get(0);
    auto arg2 = ctx.arguments().get(1);

    return _force_bool(std::visit(
        overloaded{
            [&ctx](const Table& arg1, const Table& arg2) -> CallResult {
                {
                    auto meta_left = arg1.get_metamethod("__le");
                    if (meta_left.is_function()) {
                        return meta_left.call(ctx);
                    }
                }

                {
                    auto meta_right = arg2.get_metamethod("__le");
                    if (meta_right.is_function()) {
                        return meta_right.call(ctx);
                    }
                }

                auto new_ctx = ctx.make_new({ctx.arguments().get(1), ctx.arguments().get(0)});

                {
                    auto meta_left = arg1.get_metamethod("__lt");
                    if (meta_left.is_function()) {
                        auto result = meta_left.call(new_ctx);
                        auto value = result.values().get(0).invert();
                        return CallResult(Vallist(value), result.source_change());
                    }
                }

                {
                    auto meta_right = arg2.get_metamethod("__lt");
                    if (meta_right.is_function()) {
                        auto result = meta_right.call(new_ctx);
                        auto value = result.values().get(0).invert();
                        return CallResult(Vallist(value), result.source_change());
                    }
                }

                throw std::runtime_error("attempt to perform arithmetic on a table value (both)");
            },
            [&ctx](const Table& arg1, const auto& /*arg2*/) -> CallResult {
                {
                    auto meta_left = arg1.get_metamethod("__le");
                    if (meta_left.is_function()) {
                        return meta_left.call(ctx);
                    }
                }

                {
                    auto new_ctx = ctx.make_new({ctx.arguments().get(1), ctx.arguments().get(0)});
                    auto meta_left = arg1.get_metamethod("__lt");
                    if (meta_left.is_function()) {
                        auto result = meta_left.call(new_ctx);
                        auto value = result.values().get(0).invert();
                        return CallResult(Vallist(value), result.source_change());
                    }
                }

                throw std::runtime_error("attempt to perform arithmetic on a table value (light)");
            },
            [&ctx](const auto& /*arg1*/, const Table& arg2) -> CallResult {
                {
                    auto meta_right = arg2.get_metamethod("__le");
                    if (meta_right.is_function()) {
                        return meta_right.call(ctx);
                    }
                }

                {
                    auto new_ctx = ctx.make_new({ctx.arguments().get(1), ctx.arguments().get(0)});
                    auto meta_right = arg2.get_metamethod("__lt");
                    if (meta_right.is_function()) {
                        auto result = meta_right.call(new_ctx);
                        auto value = result.values().get(0).invert();
                        return CallResult(Vallist(value), result.source_change());
                    }
                }

                throw std::runtime_error("attempt to perform arithmetic on a table value (right)");
            },
            [&location](const auto& arg1, const auto& arg2) -> CallResult {
                Value value = Value(arg1).less_than_or_equal(arg2, location);
                return CallResult({value});
            },
        },
        arg1.raw(), arg2.raw()));
}

// NOTE: This abomination means:
// pointer to a method of class Value with the signature
// auto method(std::optional<Range>) const -> Value;
using UnaryValueMethod = Value (Value::*)(std::optional<Range>) const;

static auto _unary_metamethod(
    const CallContext& ctx, std::optional<Range> location, const std::string& metamethod,
    UnaryValueMethod method, const std::string& op_kind = "arithmetic") -> CallResult {
    auto arg1 = ctx.arguments().get(0);

    return std::visit(
        overloaded{
            [&ctx, &metamethod, &op_kind](const Table& arg1) -> CallResult {
                auto meta_left = arg1.get_metamethod(metamethod);
                if (meta_left.is_function()) {
                    return meta_left.call(ctx);
                }

                throw std::runtime_error("attempt to perform "s + op_kind + " on a table value");
            },
            [&location, &method](const auto& arg1) -> CallResult {
                Value value = (Value(arg1).*method)(location);
                return CallResult({value});
            },
        },
        arg1.raw());
}

auto unm(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _unary_metamethod(ctx, std::move(location), "__unm", &Value::negate);
}

auto bnot(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _unary_metamethod(
        ctx, std::move(location), "__bnot", &Value::bit_not, "bitwise operation");
}

auto len(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    auto arg1 = ctx.arguments().get(0);

    return std::visit(
        overloaded{
            [&ctx](const Table& arg1) -> CallResult {
                auto meta_left = arg1.get_metamethod("__len");
                if (meta_left.is_function()) {
                    return meta_left.call(ctx);
                }

                return CallResult({arg1.border()});
            },
            [&location](const auto& arg1) -> CallResult {
                Value value = Value(arg1).len(location);
                return CallResult({value});
            },
        },
        arg1.raw());
}

void gc(const CallContext& ctx) {
    auto arg1 = ctx.arguments().get(0);

    return std::visit(
        overloaded{
            [&ctx](const Table& arg1) {
                auto meta = arg1.get_metamethod("__gc");
                if (meta.is_function()) {
                    auto _ = meta.call(ctx);
                }
            },
            [](const auto& /*unused*/) {}},
        arg1.raw());
}

} // namespace minilua::mt
