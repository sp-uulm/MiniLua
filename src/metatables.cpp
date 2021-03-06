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

} // namespace minilua::mt
