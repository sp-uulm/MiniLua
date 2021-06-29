#include "MiniLua/table_functions.hpp"
#include "MiniLua/utils.hpp"
#include "MiniLua/values.hpp"
#include <algorithm>
#include <cmath>
#include <future>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace minilua {
auto static try_value_is_int(Value s, const std::string& method_name, int arg_index) -> int {
    try {
        if (s.is_number()) {
            return std::get<Number>(s).try_as_int();
        }
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(
            "bad argument #" + std::to_string(arg_index) + " to '" + method_name +
            "' (number expected, got " + s.type() + ")");
    }
    auto tmp = Number(1);
    try {
        if (s.is_string()) {
            tmp = std::get<Number>(s.to_number());
        } else if (!s.is_number()) {
            throw std::runtime_error("");
        }
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(
            "bad argument #" + std::to_string(arg_index) + " to '" + method_name +
            "' (number expected, got " + s.type() + ")");
    }
    try {
        return tmp.try_as_int();
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(
            "bad argument #" + std::to_string(arg_index) + " to '" + method_name +
            "' (number has no integer representation)");
    }
}

namespace table {
auto concat(const CallContext& ctx) -> Value {
    // TODO: Add origin
    std::string result;
    auto list = ctx.arguments().get(0);
    auto sep = ctx.arguments().get(1);
    auto i = ctx.arguments().get(2);
    auto j = ctx.arguments().get(3);

    return std::visit(
        overloaded{
            [&result](const Table& list, Nil /*unused*/, Nil /*unused*/, Nil /*unused*/) -> Value {
                for (int m = 1; m <= list.border(); m++) {
                    Value v = list.get(m);
                    if (!v.is_number() && !v.is_string()) {
                        throw std::runtime_error(
                            "Invalid value (" + v.type() + ") in table for 'concat'!");
                    }
                    String vs = std::get<String>(v.to_string());
                    result += vs.value;
                }
                return Value(result);
            },
            [&result](
                const Table& list, const Value& sep, Nil /*unused*/, Nil /*unused*/) -> Value {
                if (!sep.is_number() && !sep.is_string()) {
                    throw std::runtime_error(
                        "bad argument #2 to 'concat' (string expected, got" + sep.type() + ")");
                }
                String s = std::get<String>(sep.to_string());
                for (int m = 1; m <= list.border();) {
                    Value v = list.get(m);
                    if (!v.is_number() && !v.is_string()) {
                        throw std::runtime_error(
                            "Invalid value (" + v.type() + ") in table for 'concat'!");
                    }
                    String vs = std::get<String>(v.to_string());
                    result += vs.value;
                    if (++m != list.border()) {
                        result += s.value;
                    }
                }
                return Value(result);
            },
            [&result](const Table& list, const Value& sep, Value i, Nil /*unused*/) -> Value {
                if (!sep.is_number() && !sep.is_string()) {
                    throw std::runtime_error(
                        "bad argument #2 to 'concat' (string expected, got" + sep.type() + ")");
                }
                String s = std::get<String>(sep.to_string());

                int m = try_value_is_int(std::move(i), "concat", 3);

                for (; m <= list.border();) {
                    Value v;
                    if (list.has(m)) {
                        v = list.get(m);
                    } else {
                        throw std::runtime_error(
                            "invalid value (nil) at index " + std::to_string(m) + " for 'concat'");
                    }
                    if (!v.is_number() && !v.is_string()) {
                        throw std::runtime_error(
                            "Invalid value (" + v.type() + ") in table for 'concat'!");
                    }
                    String vs = std::get<String>(v.to_string());
                    result += vs.value;
                    if (++m < list.size()) {
                        result += s.value;
                    }
                }

                return Value(result);
            },
            [&result](const Table& list, const Value& sep, Value i, Value j) -> Value {
                if (!sep.is_number() && !sep.is_string()) {
                    throw std::runtime_error(
                        "bad argument #2 to 'concat' (string expected, got" + sep.type() + ")");
                }
                String s = std::get<String>(sep.to_string());

                int m = try_value_is_int(std::move(i), "concat", 3);
                int j_int = try_value_is_int(std::move(j), "concat", 4);
                ;
                for (; m <= j_int;) {
                    Value v;
                    if (list.has(m)) {
                        v = list.get(m);
                    } else {
                        throw std::runtime_error(
                            "invalid value (nil) at index " + std::to_string(m) + " for 'concat'");
                    }
                    if (!v.is_number() && !v.is_string()) {
                        throw std::runtime_error(
                            "Invalid value (" + v.type() + ") in table for 'concat'!");
                    }
                    String vs = std::get<String>(v.to_string());
                    result += vs.value;
                    if (++m < j_int) {
                        result += s.value;
                    }
                }
                return Value(result);
            },
            [](auto list, auto /*unused*/, auto /*unused*/, auto /*unused*/) -> Value {
                throw std::runtime_error(
                    "bad argument #1 to 'concat' (table expected, got " + std::string(list.TYPE) +
                    ")");
            }},
        list.raw(), sep.raw(), i.raw(), j.raw());
}

auto move(const CallContext& ctx) -> Value {
    auto a1 = ctx.arguments().get(0);
    auto f = ctx.arguments().get(1);
    auto e = ctx.arguments().get(2);
    auto t = ctx.arguments().get(3);
    auto a2 = ctx.arguments().get(4);

    return std::visit(
        overloaded{
            [](Table a1, const Value& f, const Value& e, const Value& t, Nil /*unused*/) -> Value {
                int fi = try_value_is_int(f, "move", 2);
                int ei = try_value_is_int(e, "move", 3);
                int ti = try_value_is_int(t, "move", 4);

                for (; fi <= ei; fi++) {
                    a1.set(ti, a1.get(fi));
                    ti++;
                }
                return a1;
            },
            [](const Table& a1, const Value& f, const Value& e, const Value& t, Table a2) -> Value {
                int fi = try_value_is_int(f, "move", 2);
                int ei = try_value_is_int(e, "move", 3);
                int ti = try_value_is_int(t, "move", 4);

                for (; fi <= ei; fi++) {
                    a2.set(ti, a1.get(fi));
                    ti++;
                }
                return a2;
            },
            [](const Table& /*unused*/, const Value& /*unused*/, const Value& /*unused*/,
               const Value& /*unused*/, auto a2) -> Value {
                throw std::runtime_error(
                    "bad argument #5 to 'move' (tabe expected, got " + std::string(a2.TYPE) + ")");
            },
            [](auto a1, auto /*unused*/, auto /*unused*/, auto /*unused*/,
               auto /*unused*/) -> Value {
                throw std::runtime_error(
                    "bad argument #1 to 'move' (table expected, got " + std::string(a1.TYPE) + ")");
            }},
        a1.raw(), f.raw(), e.raw(), t.raw(), a2.raw());
}

void insert(const CallContext& ctx) {
    auto list = ctx.arguments().get(0);
    auto pos = ctx.arguments().get(1);
    auto value = ctx.arguments().get(2);

    std::visit(
        overloaded{
            [](Table& table, Nil /*unused*/, const Value& value) {
                Number pos = table.border() + 1;
                table.set(pos, value);
            },
            [](Table& table, const Value& pos, const Value& value) {
                int p = try_value_is_int(pos, "insert", 2);
                if (table.has(p)) {
                    // move every element one to the right so make space for the new element that is
                    // inserted if pos is already occupied
                    for (int i = table.border(); i >= p; i--) {
                        table.set(i + 1, table.get(i));
                    }
                }
                table.set(p, value);
            },
            [](auto /*unused*/, auto /*unused*/, auto /*unused*/) {}},
        list.raw(), pos.raw(), value.raw());
}

auto pack(const CallContext& ctx) -> Value {
    // TODO: add origin
    Table t = Table();
    int i = 1;

    for (const auto& a : ctx.arguments()) {
        t.set(i, a);
    }
    return t;
}

auto remove(const CallContext& ctx) -> Value {
    auto list = ctx.arguments().get(0);
    auto pos = ctx.arguments().get(1);

    return std::visit(
        overloaded{
            [](Table list, Nil /*unused*/) -> Value {
                auto tmp = list.get(list.border());
                list.set(list.border(), Nil());
                return tmp;
            },
            [](Table list, const Value& pos) -> Value {
                int posi = try_value_is_int(pos, "remove", 2);
                auto tmp = list.get(posi);
                for (int i = posi + 1; i <= list.border(); i++) {
                    list.set(posi, list.get(i));
                    posi++;
                }
                list.set(list.border(), Nil());
                return tmp;
            },
            [](auto list, auto /*unused*/) -> Value {
                throw std::runtime_error(
                    "bad argument #1 to 'remove' (table expected, got " + std::string(list.TYPE) +
                    ")");
            }},
        list.raw(), pos.raw());
}

void sort(const CallContext& ctx) {
    auto list = ctx.arguments().get(0);
    auto comp = ctx.arguments().get(1);

    std::visit(
        overloaded{
            [](Table list, Nil /*unused*/) {
                std::vector<Value> content;
                for (int i = 1; i <= list.border(); i++) {
                    content.push_back(list.get(i));
                }
                std::sort(content.begin(), content.end());
                for (int i = 1; i <= list.border(); i++) {
                    list.set(i, content.at(i));
                }
            },
            [&ctx](Table list, const Function& comp) {
                std::vector<Value> content;
                for (int i = 1; i <= list.border(); i++) {
                    content.push_back(list.get(i));
                }
                std::sort(
                    content.begin(), content.end(),
                    [&ctx, &comp](const Value& a, const Value& b) -> bool {
                        auto c = ctx.make_new(Vallist{a, b});
                        auto erg = comp.call(c).values().get(0);
                        if (erg.type() == "Boolean") {
                            return std::get<Bool>(erg).value;
                        } else {
                            throw std::runtime_error("invalid order function for sorting");
                        }
                    });
                for (int i = 1; i <= list.border(); i++) {
                    list.set(i, content.at(i));
                }
            },
            [](auto list, auto /*unused*/) {
                throw std::runtime_error(
                    "bad argument #1 to 'sort' (table expected, got " + std::string(list.TYPE) +
                    ")");
            }},
        list.raw(), comp.raw());
}

auto unpack(const CallContext& ctx) -> Vallist {
    // TODO: add origin
    std::vector<Value> vector;
    auto list = ctx.arguments().get(0);
    auto i = ctx.arguments().get(1);
    auto j = ctx.arguments().get(2);
    return std::visit(
        overloaded{
            [&vector](const Table& list, Nil /*unused*/, Nil /*unused*/) -> Vallist {
                for (int i = 1; i <= list.border(); i++) {
                    auto v = list.get(i);
                    vector.push_back(v);
                }
                return Vallist(vector);
            },
            [&vector](const Table& list, const Value& i, Nil /*unused*/) {
                int i_int = try_value_is_int(i, "unpack", 2);
                for (; i_int <= list.border(); i_int++) {
                    vector.push_back(list.get(i));
                }
                return Vallist(vector);
            },
            [&vector](const Table& list, const Value& i, const Value& j) {
                int i_int = try_value_is_int(i, "unpack", 2);
                int j_int = try_value_is_int(j, "unpack", 3);
                for (; i_int <= j_int; i_int++) {
                    vector.push_back(list.get(i));
                }
                return Vallist(vector);
            },
            [](auto list, auto /*unused*/, auto /*unused*/) -> Vallist {
                throw std::runtime_error(
                    "bad argument #1 for 'unpack' (table expected, got " + std::string(list.TYPE) +
                    ")");
            }},
        list.raw(), i.raw(), j.raw());
}
} // end namespace table
} // end namespace minilua