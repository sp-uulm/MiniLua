#include "MiniLua/values.hpp"
#include <MiniLua/package.hpp>
#include <memory>
#include <regex>
#include <string>

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
Table searchers = std::make_unique<Table>(new Table(
    {{1,
      [](const CallContext& ctx) -> Value {
          auto name = ctx.arguments().get(0);
          return preload.get(name);
      }},
     {2, [](CallContext ctx) -> Value {
          ctx = ctx.make_new({ctx.arguments().get(0), path});
          auto paths = searchpath(ctx);
          if (paths.get(0).is_nil()) {
              // TODO: print paths.get(1)
          }
          return paths.get(0);
      }}}));

auto static split_string(std::string text, std::string sep) -> std::vector<std::string> {
    std::vector<std::string> parts;

    int pos = 0;
    while ((pos = text.find(sep)) != std::string::npos) {
        // this check is needed for back to back seperators so they don't get inserted
        if (pos != 0) {
            parts.push_back(text.substr(0, pos));
        }
        text.erase(0, pos + sep.length());
    }
    // insert the remaining part of text
    parts.push_back(text);

    return parts;
}

auto searchpath(const CallContext& ctx) -> Vallist {
    auto name = ctx.arguments().get(0);
    auto path = ctx.arguments().get(1);
    auto sep = ctx.arguments().get(2);
    auto rep = ctx.arguments().get(3);

    // default values for optional parameters
    if (sep.is_nil()) {
        sep = ".";
    }
    if (rep.is_nil()) {
        // default value is the system directory seperator
        rep = std::get<String>(config).value[0];
    }
    // type handling
    if (name.is_number()) {
        name = name.to_string();
    } else if (!name.is_string()) {
        throw std::runtime_error(
            "bad argument #1 to 'searchpath' (string expected, got " + name.type() + ")");
    }
    if (path.is_number()) {
        path = path.to_string();
    } else if (!path.is_string()) {
        throw std::runtime_error(
            "bad argument #2 to 'searchpath' (string expected, got " + path.type() + ")");
    }
    if (!sep.is_string() and !sep.is_number()) {
        throw std::runtime_error(
            "bad argument #3 to 'searchpath' (string expected, got " + sep.type() + ")");
    }
    if (!rep.is_number() and !rep.is_string()) {
        throw std::runtime_error(
            "bad argument #4 to 'searchpath' (string expected, got " + rep.type() + ")");
    }
    // logical section
    auto parts = split_string(std::get<String>(path).value, ";");
    name = std::regex_replace(
        std::get<String>(name).value, std::regex(std::get<String>(sep.to_string()).value),
        std::get<String>(rep.to_string()).value);

    std::string looked_up_files;
    for (auto s : parts) {
        s = std::regex_replace(s, std::regex("?"), std::get<String>(name).value);

        // TODO: check if file s exists
        // If exists then return s
        // else add "no file 's'\n" to looked_up_files
    }
    return Vallist({Nil(), looked_up_files});
}
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
    if (modname.is_number()) {
        // lua treads numbers as stings in this case
        modname = modname.to_string();
    } else if (!modname.is_string()) {
        throw std::runtime_error(
            "bad argument #1 to 'require' (string expected, got " + modname.type() + ")");
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
            // true is assigned because it's defined this way in the lua documentation.
            // It's probably to prevent repeatedly trying to load the same module if it fails once.
            package::loaded.set(modname, true);
        }
    }
    return package::loaded.get(modname);
}

} // end namespace minilua