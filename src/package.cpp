#include "MiniLua/values.hpp"
#include <MiniLua/package.hpp>

namespace minilua {
auto create_package_table(MemoryAllocator* allocator) -> Table {
    std::unordered_map<Value, Value> math_functions;
    Table package(allocator);

    return package;
}

namespace package {

#ifndef _WIN64
Value config = "\\\n"
               ";\n"
               "?\n"
               "!\n"
               "-";
#else
minilua::Value config = "/\n"
                        ";\n"
                        "?\n"
                        "!\n"
                        "-";
#endif

Value cpath = Nil();
Value path = Nil();
Table loaded;
Table preload;
Table searchers;

} // end namespace package

auto find_loader(const CallContext& ctx) -> Vallist {
    String modname = std::get<String>(ctx.arguments().get(0));
    std::string error_msg;
    for (const auto& p : package::searchers) {
        Value searcher = p.second;

        if (searcher.is_function()) {
            Function func = std::get<Function>(searcher);
            auto res = func.call(ctx).values();
            auto loader = res.get(0);

            if (loader.is_function()) {
                return res;
            } else if (loader.is_string()) {
                String tmp = std::get<String>(loader);
                error_msg += tmp.value + "\n";
            }
        }
    }
    throw std::runtime_error("module '" + modname.value + "' not found:\n" + error_msg);
}

auto require(const CallContext& ctx) -> Value {
    auto modname = ctx.arguments().get(0);
    if (!modname.is_string()) {
        throw std::runtime_error("");
    }
    if (package::loaded.has(modname)) {
        return package::loaded.get(modname);
    } else {
        // Search for loader in package.searchers
        auto tmp = find_loader(ctx);
        Function loader = std::get<Function>(tmp.get(0));
        const Value& extra_value = tmp.get(1);
        auto erg = loader.call(ctx.make_new({modname, extra_value})).values().get(0);
        if (!erg.is_nil()) {
            package::loaded.set(modname, erg);
        } else if (package::loaded.get(modname).is_nil()) {
            package::loaded.set(modname, true);
        }
    }
    return package::loaded.get(modname);
}

} // end namespace minilua