#include <algorithm>
#include <array>
#include <cstddef>
#include <ios>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "MiniLua/source_change.hpp"
#include "MiniLua/string.hpp"
#include "MiniLua/utils.hpp"
#include "MiniLua/values.hpp"

namespace minilua {
template <class Result>
auto static try_value_as(
    Value s, const std::string& method_name, int arg_index, bool needs_int_representation = false)
    -> Result {
    if (s.is_number()) {
        try {
            return std::get<Number>(s).convert_to<Result>(needs_int_representation);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(
                "bad argument #" + std::to_string(arg_index) + " to '" + method_name +
                "' (number has no integer representation)");
        }
    }

    auto tmp = Number(1);
    try {
        if (s.is_string()) {
            Value v = s.to_number();
            if (v == Nil()) {
                throw std::runtime_error("");
            }
            tmp = std::get<Number>(v);
        } else {
            throw std::runtime_error("");
        }
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(
            "bad argument #" + std::to_string(arg_index) + " to '" + method_name +
            "' (number expected, got " + s.type() + ")");
    }

    try {
        return tmp.convert_to<Result>(needs_int_representation);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(
            "bad argument #" + std::to_string(arg_index) + " to '" + method_name +
            "' (number has no integer representation)");
    }
}

auto static try_value_is_string(const Value& val, const std::string& method_name, int arg_index)
    -> Value {
    if (val.is_string() || val.is_number()) {
        return val.to_string();
    }
    throw std::runtime_error(
        "bad argument #" + std::to_string(arg_index) + " to '" + method_name +
        "' (string expected, got " + val.type() + ")");
}

auto static parse_string(const std::string& str, std::vector<Value> args) -> std::string {
    std::string_view s(str);
    std::stringstream ss;

    // find escapes and replace them with the argument
    int numEscapes = 0;
    size_t pos = 0;
    while (pos < s.length()) {

        size_t start_pos = s.find('%', pos);
        if (start_pos == std::string_view::npos) {
            ss << s.substr(pos);
            break;
        }

        // get string before escape
        // TODO: maybe useless since snprintf can use it. Rethink if necessary
        ss << s.substr(pos, start_pos - pos);
        pos = start_pos + 1;

        // search for flags and apply them
        for (bool in_flags = true; in_flags && pos < s.length(); ++pos) {
            switch (s[pos]) {
            case '#':
            case '0':
            case ' ':
            case '+':
            case '-':
                // do nothing, just recognize they exist
                break;
            default:
                in_flags = false;
                --pos;
                break;
            }
        }

        // search for width "%.5d"
        pos = s.find_first_not_of("0123456789", pos);

        // search for precision
        if (pos < s.length() && s[pos] == '.') {
            pos = s.find_first_not_of("0123456789", pos + 1);
        }

        pos = pos == std::string_view::npos ? s.length() : pos;

        // check for conversion type
        if (pos >= s.length()) {
            throw std::runtime_error("invalid format (width or precision too long)");
            // "invalid conversion '" + std::string(s.substr(start_pos, pos - start_pos)) +
            // "' to 'format'");
        }

        auto escape = std::string(s.substr(start_pos, pos - start_pos + 1));
        const auto* form = escape.c_str();
        auto format_escape = [&ss, &form](auto arg) -> void {
            std::vector<char> buffer;
            int size = std::snprintf(nullptr, 0, form, arg) + 1;
            buffer.resize(size);
            std::snprintf(buffer.data(), size, form, arg);
            ss << buffer.data();
        };
        auto arg_value = args.at(numEscapes);
        switch (s[pos++]) {
        case 'u': // one more error-case, the rest is the same behavior
            /* Only needed in lua5.4
            if (alternate_form) {
                throw std::runtime_error(
                    "invalid conversion specification: '" +
                    std::string(s.substr(start_pos, pos - start_pos)) + "'");
            }
            */
        case 'd':
        case 'i': // same error and type behavior as 'o'
        case 'X':
        case 'x': // same error and type behavior as 'o'
        case 'o': {
            form = escape.insert(escape.length() - 1, 1, 'l').c_str();
            format_escape(try_value_as<Number::Int>(arg_value, "format", numEscapes + 2, true));
            break;
        }
        // same error behavior for all of those. Formating is done by snprintf
        case 'E':
        case 'e':
        case 'f':
        case 'G':
        case 'g':
        case 'A':
        case 'a':
            format_escape(try_value_as<Number::Float>(arg_value, "format", numEscapes + 2));
            break;
        case 'c': {
            /*
            only needed for lua5.4

            if (alternate_form || zero_pad || blank || sign) {
                throw std::runtime_error(
                    "invalid conversion specification: '" +
                    std::string(s.substr(start_pos, pos - start_pos)) + "'");
            }
            */
            const char imm = try_value_as<Number::Int>(arg_value, "format", numEscapes + 2, true);
            format_escape(imm);
            break;
        }
        case 's': {
            format_escape(std::get<String>(arg_value.to_string()).value.c_str());
            break;
        }
        case '%':
            // no flags allowed
            if (escape.length() > 2) {
                throw std::runtime_error(
                    "invalid option '" + std::string(s.substr(start_pos, pos - start_pos)) +
                    "' to 'format'");
            }
            ss << '%';
            break;
        default:
            throw std::runtime_error(
                "invalid option '" + std::string(s.substr(start_pos, pos - start_pos)) +
                "' to 'format'");
        }

        ++numEscapes;
    }
    return ss.str();
}

auto create_string_table(MemoryAllocator* allocator) -> Table {
    std::unordered_map<Value, Value> string_functions;
    Table string(allocator);
    string.set("byte", string::byte);
    string.set("char", string::lua_char);
    // string.set("dump", string::dump);
    // string.set("find", string::find);
    string.set("format", string::format);
    // string.set("gmatch", string::gmatch);
    // string.set("gsub", string::gsub);
    string.set("len", string::len);
    string.set("lower", string::lower);
    // string.set("match", string::match);
    // string.set("pack", string::pack);
    // string.set("packsize", string::packsize);
    string.set("rep", string::rep);
    string.set("reverse", string::reverse);
    string.set("sub", string::sub);
    // string.set("unpack", string::unpack);
    string.set("upper", string::upper);

    return string;
}
namespace string {
auto byte(const CallContext& ctx) -> Vallist {
    auto values = std::vector<Value>(2);
    auto location = ctx.call_location();
    auto reverse = [](const Value& new_value, const Value& old_str,
                      const Value& idx) -> std::optional<SourceChangeTree> {
        if (!new_value.is_number()) {
            return std::nullopt;
        }
        auto s = std::get<String>(old_str).value;
        char new_char = std::get<Number>(new_value).try_as_int();
        Number::Int i = 0;

        if (!idx.is_nil()) {
            i = std::get<Number>(idx).try_as_int();
        }
        s[i] = new_char;
        return old_str.force(s);
    };
    Value byte_string = ctx.arguments().get(0);
    values.push_back(byte_string);
    auto i = ctx.arguments().get(1);
    auto j = ctx.arguments().get(2);
    std::vector<Value> result;

    return std::visit(
        overloaded{
            [&byte_string, &location,
             &reverse](const String& s, Nil /*unused*/, Nil /*unused*/) -> Vallist {
                Value result = Nil();

                if ((int)s.value[0] != 0) {
                    result = (int)s.value[0];
                }

                return Vallist(result.with_origin(Origin(BinaryOrigin{
                    .lhs = std::make_shared<Value>(byte_string),
                    .rhs = std::make_shared<Value>(Nil()),
                    .location = location,
                    .reverse = reverse})));
            },
            [&byte_string, &location,
             &reverse](const Number& str, Nil /*unused*/, Nil /*unused*/) -> Vallist {
                Value result = Nil();
                String s = std::get<String>(Value(str).to_string());

                if ((int)s.value[0] != 0) {
                    result = (int)s.value[0];
                }

                return Vallist(result.with_origin(Origin(BinaryOrigin{
                    .lhs = std::make_shared<Value>(byte_string),
                    .rhs = std::make_shared<Value>(Nil()),
                    .location = location,
                    .reverse = reverse})));
            },
            [&result, &i, &byte_string, &location,
             &reverse](const String& s, auto /*i*/, Nil) -> Vallist {
                std::string str = s.value;
                int i_int = try_value_as<Number::Int>(i, "byte", 2, true) - 1;
                if (i_int < 0) {
                    i_int = str.length() + i_int + 1;
                }
                int j = i_int;

                for (; i_int <= j && i_int < str.length(); ++i_int) {
                    Value v = (int)str[i_int];
                    result.push_back(v.with_origin(BinaryOrigin{
                        .lhs = std::make_shared<Value>(byte_string),
                        .rhs = std::make_shared<Value>(i_int),
                        .location = location,
                        .reverse = reverse}));
                }
                return {result};
            },
            [&result, &i, &byte_string, &location,
             &reverse](const Number& s, auto /*i*/, Nil) -> Vallist {
                std::string str = std::get<String>(Value(s).to_string()).value;
                int i_int = try_value_as<Number::Int>(i, "byte", 2, true) - 1;
                if (i_int < 0) {
                    i_int = str.length() + i_int + 1;
                }
                int j = i_int;

                for (; i_int <= j && i_int < str.length(); ++i_int) {
                    Value v = (int)str[i_int];
                    result.push_back(v.with_origin(BinaryOrigin{
                        .lhs = std::make_shared<Value>(byte_string),
                        .rhs = std::make_shared<Value>(i_int),
                        .location = location,
                        .reverse = reverse}));
                }
                return {result};
            },
            [&result, &j, &byte_string, &location,
             &reverse](const String& s, Nil, auto /*j*/) -> Vallist {
                std::string str = s.value;
                int i_int = 0;
                int j_int = try_value_as<Number::Int>(j, "byte", 3, true) - 1;
                if (j_int < 0) {
                    j_int = str.length() + j_int + 1;
                }

                for (; i_int <= j_int && i_int < str.length(); ++i_int) {
                    Value v = (int)str[i_int];
                    result.push_back(v.with_origin(BinaryOrigin{
                        .lhs = std::make_shared<Value>(byte_string),
                        .rhs = std::make_shared<Value>(i_int),
                        .location = location,
                        .reverse = reverse}));
                }
                return {result};
            },
            [&result, &j, &byte_string, &location,
             &reverse](const Number& s, Nil, auto /*j*/) -> Vallist {
                std::string str = std::get<String>(Value(s).to_string()).value;
                int i_int = 0;
                int j_int = try_value_as<Number::Int>(j, "byte", 3, true) - 1;
                if (j_int < 0) {
                    j_int = str.length() + j_int + 1;
                }

                for (; i_int <= j_int && i_int < str.length(); ++i_int) {
                    Value v = (int)str[i_int];
                    result.push_back(v.with_origin(BinaryOrigin{
                        .lhs = std::make_shared<Value>(byte_string),
                        .rhs = std::make_shared<Value>(i_int),
                        .location = location,
                        .reverse = reverse}));
                }
                return {result};
            },
            [&result, &i, &j, &byte_string, &location,
             &reverse](const String& s, auto /*i*/, auto /*j*/) -> Vallist {
                std::string str = s.value;
                int i_int = try_value_as<Number::Int>(i, "byte", 2, true) - 1;
                int j_int = try_value_as<Number::Int>(j, "byte", 3, true) - 1;
                if (i_int < 0) {
                    i_int = str.length() + i_int + 1;
                }
                if (j_int < 0) {
                    j_int = str.length() + j_int + 1;
                }

                for (; i_int <= j_int && i_int < str.length(); i_int++) {
                    Value v = (int)str[i_int];
                    result.push_back(v.with_origin(BinaryOrigin{
                        .lhs = std::make_shared<Value>(byte_string),
                        .rhs = std::make_shared<Value>(i_int),
                        .location = location,
                        .reverse = reverse}));
                }
                return {result};
            },
            [&result, &i, &j, &byte_string, &location,
             &reverse](const Number& s, auto /*i*/, auto /*j*/) -> Vallist {
                std::string str = std::get<String>(Value(s).to_string()).value;
                int i_int = try_value_as<Number::Int>(i, "byte", 2, true) - 1;
                int j_int = try_value_as<Number::Int>(j, "byte", 3, true) - 1;
                if (i_int < 0) {
                    i_int = str.length() + i_int + 1;
                }
                if (j_int < 0) {
                    j_int = str.length() + j_int + 1;
                }

                for (; i_int <= j_int && i_int < str.length(); i_int++) {
                    Value v = (int)str[i_int];
                    result.push_back(v.with_origin(BinaryOrigin{
                        .lhs = std::make_shared<Value>(byte_string),
                        .rhs = std::make_shared<Value>(i_int),
                        .location = location,
                        .reverse = reverse}));
                }
                return {result};
            },
            [&byte_string](
                const auto& /*unused*/, const auto& /*unused*/, const auto& /*unused*/) -> Vallist {
                throw std::runtime_error(
                    "bad argument #1 to 'byte' (string expected, got " + byte_string.type() + ")");
            }},
        byte_string.raw(), i.raw(), j.raw());
}

auto lua_char(const CallContext& ctx) -> Value {
    Origin origin = MultipleArgsOrigin{
        .values = std::make_shared<Vallist>(ctx.arguments()),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Vallist& args) -> std::optional<SourceChangeTree> {
            if (!new_value.is_string()) {
                return std::nullopt;
            }
            auto new_string = std::get<String>(new_value).value;
            if (new_string.length() != args.size()) {
                return std::nullopt;
            }
            SourceChangeCombination source_changes;
            for (int i = 0; i < new_string.length(); ++i) {
                int val = new_string[i];
                source_changes.add(args.get(i).force(val).value());
            }
            return source_changes;
        }};
    std::vector<char> result;
    int i = 1;

    for (const auto& arg : ctx.arguments()) {
        int lettr = try_value_as<Number::Int>(arg, "char", i, true);
        if (lettr < 0 || lettr > 255) {
            std::stringstream ss;
            ss << "bad argument #" << i << " to 'char' ";
            ss << "(value out of range)" << std::endl;
            throw std::runtime_error(ss.str());
        }
        result.emplace_back((char)lettr);
        i++;
    }

    return Value(std::string(result.begin(), result.end())).with_origin(origin);
}

auto format(const CallContext& ctx) -> Value {
    auto formatstring = ctx.arguments().get(0);
    std::vector<Value> args(ctx.arguments().size() - 1);

    if (!formatstring.is_string() && !formatstring.is_number()) {
        throw std::runtime_error(
            "bad argument #1 to 'format' (string expected, got " + formatstring.type() + ")");
    }

    // remove first argument formatstring from arguments-list
    std::copy(ctx.arguments().begin() + 1, ctx.arguments().end(), args.begin());

    return Value(parse_string(std::get<String>(formatstring).value, args)).with_origin(NoOrigin());
}

auto len(const CallContext& ctx) -> Value {
    auto s = try_value_is_string(ctx.arguments().get(0), "len", 1);

    String str = std::get<String>(s);
    // can't reverse length because with the change of the result more characters have to be added
    // to the string. Or if the string is shorter than previously, characters have to be removed.
    // But it is not clear where in the string.
    return Value((long)str.value.length()).with_origin(NoOrigin());
}

auto lower(const CallContext& ctx) -> Value {
    auto s = try_value_is_string(ctx.arguments().get(0), "lower", 1);

    std::string str = std::get<String>(Value(s)).value;
    std::transform(
        str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    return Value(str).with_origin(NoOrigin());
}

auto rep(const CallContext& ctx) -> Value {
    auto s = ctx.arguments().get(0);
    auto n = ctx.arguments().get(1);
    auto seperator = ctx.arguments().get(2);

    std::string str = std::get<String>(try_value_is_string(s, "rep", 1)).value;
    std::string sep;
    int reps = try_value_as<Number::Int>(n, "rep", 2, true);
    if (!seperator.is_nil()) {
        sep = std::get<String>(try_value_is_string(seperator, "rep", 3)).value;
    }

    if (reps <= 0) {
        return Value("").with_origin(NoOrigin());
    }
    std::stringstream result;
    result << str;
    for (int i = 1; i < reps; i++) {
        result << sep << str;
    }

    // Too many posibilites what could be changed for reverse (maybe everything if there is no
    // repeat in new string)
    return Value(result.str()).with_origin(NoOrigin());
}

auto reverse(const CallContext& ctx) -> Value {
    Origin origin = UnaryOrigin{
        .val = std::make_shared<Value>(ctx.arguments().get(0)),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Value& old_value) -> std::optional<SourceChangeTree> {
            if (!new_value.is_string()) {
                return std::nullopt;
            }
            std::string s = std::get<String>(new_value).value;
            std::reverse(s.begin(), s.end());
            return old_value.force(s);
        }};
    auto s = try_value_is_string(ctx.arguments().get(0), "reverse", 1);

    std::string str = std::get<String>(s).value;
    std::reverse(str.begin(), str.end());
    return Value(str).with_origin(origin);
}

auto sub(const CallContext& ctx) -> Value {
    Origin origin = MultipleArgsOrigin{
        .values = std::make_shared<Vallist>(ctx.arguments()),
        .location = ctx.call_location(),
        .reverse = [](const Value& new_value,
                      const Vallist& old_args) -> std::optional<SourceChangeTree> {
            if (!new_value.is_string()) {
                return std::nullopt;
            }
            std::string old_str = std::get<String>(old_args.get(0)).value;
            std::string new_str = std::get<String>(new_value).value;
            int i = std::get<Number>(old_args.get(1)).convert_to_int();
            int j = old_args.get(2).is_nil() ? old_str.length()
                                             : std::get<Number>(old_args.get(2)).convert_to_int();
            if (i < 0) {
                i = i = old_str.length() - std::abs(i) + 1;
            }
            if (j < 0) {
                j = old_str.length() - std::abs(j) + 1;
            }
            j = std::min(j, ((signed)old_str.length()));
            i = std::max(i - 1, 0);
            j = std::max(j - 1, 0);
            int distance = j - i + 1;

            // check if new string has same propotions as old string
            if (new_str.length() != distance) {
                return std::nullopt;
            }
            old_str.replace(i, distance, new_str);
            // TODO: return other source change alternatives like changing the result to another
            // part of the string so that i and j change. This will also open up the option to
            // change the length of the result
            return old_args.get(0).force(old_str);
        }};
    auto s = ctx.arguments().get(0);
    auto i = ctx.arguments().get(1);
    auto j = ctx.arguments().get(2);

    std::string str = std::get<String>(try_value_is_string(s, "sub", 1)).value;
    int start = try_value_as<Number::Int>(i, "sub", 2, true);
    int end = str.length();
    if (!j.is_nil()) {
        end = try_value_as<Number::Int>(j, "sub", 3, true);
    }
    if (start < 0) {
        start = str.length() - std::abs(start) + 1;
    }
    if (end < 0) {
        end = str.length() - std::abs(end) + 1;
    }
    if (end < start || start > (signed)str.length()) {
        return Value("").with_origin(origin);
    }
    start = std::max(start - 1, 0);
    end = std::max(end, 0);
    int distance = end - start;
    return Value(str.substr(start, distance)).with_origin(origin);
}

auto upper(const CallContext& ctx) -> Value {
    auto s = try_value_is_string(ctx.arguments().get(0), "upper", 1);

    std::string str = std::get<String>(Value(s)).value;
    std::transform(
        str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::toupper(c); });
    // No way to reverse: no way to know which characters where changed to upper case.
    return Value(str).with_origin(NoOrigin());
}
} // end of namespace string
} // end of namespace minilua
