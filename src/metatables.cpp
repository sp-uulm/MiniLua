#include <utility>

#include "MiniLua/metatables.hpp"

namespace minilua::mt {

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
            [](auto value) -> CallResult {
                throw std::runtime_error("can't index into " + std::string(value.TYPE));
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
    BinaryValueMethod method) -> CallResult {
    auto arg1 = ctx.arguments().get(0);
    auto arg2 = ctx.arguments().get(1);

    return std::visit(
        overloaded{
            [&ctx, &metamethod](const Table& arg1, const Table& arg2) -> CallResult {
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

                throw std::runtime_error("attempt to perform arithmetic on a table value");
            },
            [&ctx, &metamethod](const Table& arg1, const auto& /*arg2*/) -> CallResult {
                auto meta_left = arg1.get_metamethod(metamethod);
                if (meta_left.is_function()) {
                    return meta_left.call(ctx);
                }

                throw std::runtime_error("attempt to perform arithmetic on a table value");
            },
            [&ctx, &metamethod](const auto& /*arg1*/, const Table& arg2) -> CallResult {
                auto meta_right = arg2.get_metamethod(metamethod);
                if (meta_right.is_function()) {
                    return meta_right.call(ctx);
                }

                throw std::runtime_error("attempt to perform arithmetic on a table value");
            },
            [&location, &method](const auto& arg1, const auto& arg2) -> CallResult {
                Value value = (Value(arg1).*method)(arg2, location);
                return CallResult({value});
            },
        },
        arg1.raw(), arg2.raw());
}

auto add(const CallContext& ctx, std::optional<Range> location) -> CallResult {
    return _binary_metamethod(ctx, std::move(location), "__add", &Value::add);
}

} // namespace minilua::mt
