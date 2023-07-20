#include <array>
#include <cstddef>
#include <ios>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <regex>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>
#include <algorithm>

#include "MiniLua/source_change.hpp"
#include "MiniLua/string.hpp"
#include "MiniLua/utils.hpp"
#include "MiniLua/values.hpp"

namespace minilua {
    template<class Result>
    auto static try_value_as(Value s, const std::string& method_name, int arg_index) -> Result {
        if (s.is_number()) {
            return std::get<Number>(s).convert_to<Result>();
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
            return tmp.convert_to<Result>();
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(
                "bad argument #" + std::to_string(arg_index) + " to '" + method_name +
                "' (number has no integer representation)");
        }
    }

    auto static try_value_is_string(const Value& val, const std::string& method_name, int arg_index) -> Value {
        if (val.is_string() || val.is_number()) {
            return std::get<String>(val.to_string());
        }
        throw std::runtime_error(
                "bad argument #" + std::to_string(arg_index) + " to '" + method_name +
                "' (string expected, got " + val.type() + ")");
    }

    auto static parse_string(const std::string& str, std::vector<Value> args)-> std::string {
        std::string_view s(str);
        std::stringstream ss;

        //find escapes and replace them with the argument
        int numEscapes = 0;
        size_t pos = 0;
        while (pos < s.length()) {
            bool alternate_form = false;
            bool zero_pad = false;
            bool left_adjust = false;
            bool sign = false;
            bool blank = false;

            size_t start_pos = s.find('%', pos);
            if (start_pos == std::string_view::npos) {
                ss << s.substr(pos);
                break;
            }

            // get string before escape
            // TODO: maybe useless since snprintf can use it. Rethink if necessary
            ss << s.substr(pos, start_pos - pos);
            pos = start_pos + 1;

            //search for flags and apply them
            for (bool in_flags = true; in_flags && pos < s.length(); ++pos) {
                switch (s[pos]) {
                    case '#':
                        alternate_form = true;
                        break;
                    case '0':
                        zero_pad = !left_adjust;
                        break;
                    case ' ':
                        blank = true;
                        break;
                    case '+':
                        sign = true;
                        break;
                    case '-':
                        zero_pad = false;
                        left_adjust = true;
                        break;
                    default:
                        in_flags = false;
                        --pos;
                        break;
                }
            }

            //search for width "%.5d"
            pos = s.find_first_not_of("0123456789", pos);

            //search for precision
            if (pos < s.length() && s[pos] == '.') {
                pos = s.find_first_not_of("0123456789", pos+1);
            }

            pos = pos == std::string_view::npos ? s.length() : pos;

            //check for conversion type
            if (pos >= s.length()) {
                throw std::runtime_error("invalid conversion '" + std::string(s.substr(start_pos, pos - start_pos)) + "' to 'format'");
            }

            auto escape = std::string(s.substr(start_pos, pos - start_pos + 1));
            const auto *form = escape.c_str();
            auto format_escape = [&ss, form](auto arg) -> void {
                std::vector<char> buffer;
                int size = std::snprintf(nullptr, 0, form, arg) + 1;
                buffer.resize(size);
                std::snprintf(buffer.data(), size, form, arg);
                ss << buffer.data();
            };
            auto arg_value = args.at(numEscapes);
            switch (s[pos++]) {
                case 'u': //one more error-case, the rest is the same behavior
                    if (alternate_form) { throw std::runtime_error("invalid conversion specification: '" + std::string(s.substr(start_pos, pos - start_pos)) + "'"); }
                case 'd':
                case 'i': //same error and type behavior as 'o'
                case 'X':
                case 'x': //same error and type behavior as 'o'
                case 'o': {
                    format_escape(try_value_as<Number::Int>(arg_value, "format", numEscapes+2));
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
                    format_escape(try_value_as<Number::Float>(arg_value, "format", numEscapes+2));
                    break;
                case 'c': {
                    if (alternate_form || zero_pad || blank || sign) {
                        throw std::runtime_error("invalid conversion specification: '" + std::string(s.substr(start_pos, pos - start_pos)) + "'");
                    }
                    const char imm = try_value_as<Number::Int>(arg_value, "format", numEscapes+2);
                    format_escape(imm);
                    break;
                }
                case 's': {
                    format_escape(std::get<String>(arg_value.to_string()).value.c_str());
                    break;
                }
                case '%':
                    //no flags allowed
                    if (alternate_form || zero_pad || sign || blank || left_adjust) {
                        throw std::runtime_error("invalid conversion specification: '" + std::string(s.substr(start_pos, pos - start_pos)) + "'");
                    }
                    ss << '%';
                    break;
                default:
                    throw std::runtime_error("invalid conversion '" + std::string(s.substr(start_pos, pos - start_pos)) + "' to 'format'");
            }

            ++numEscapes;
        }
        return ss.str();
    }

    auto create_string_table(MemoryAllocator* allocator) -> Table {
        std::unordered_map<Value, Value> string_functions;
        Table string(allocator);
        string.set("byte", string::byte);
        string.set("char", string::Char);
        //string.set("dump", string::dump);
        //string.set("find", string::find);
        string.set("format", string::format);
        //string.set("gmatch", string::gmatch);
        //string.set("gsub", string::gsub);
        string.set("len", string::len);
        string.set("lower", string::lower);
        //string.set("match", string::match);
        //string.set("pack", string::pack);
        //string.set("packsize", string::packsize);
        string.set("rep", string::rep);
        string.set("reverse", string::reverse);
        string.set("sub", string::sub);
        //string.set("unpack", string::unpack);
        string.set("upper", string::upper);

        return string;
    }
namespace string {
    auto byte(const CallContext& ctx) -> Vallist {
        auto values = std::vector<Value>(2);
        auto location = ctx.call_location();
        auto reverse = [](const Value& new_value, const Vallist& args) -> std::optional<SourceChangeTree> {
                if (!new_value.is_number()) {
                    return std::nullopt;
                }
                const auto& old_str = args.get(0);
                auto s = std::get<String>(old_str).value;
                char new_char = std::get<Number>(new_value).try_as_int();
                Number::Int i = 1;

                if (!args.get(1).is_nil()) {
                    i = std::get<Number>(args.get(1)).try_as_int();
                }

                s[i-1] = new_char;
                return old_str.force(s);
        };
        auto s = ctx.arguments().get(0);
        values.push_back(s);
        auto i = ctx.arguments().get(1);
        auto j = ctx.arguments().get(2);
        std::vector<Value> result;

        return std::visit(overloaded{
            [&values, &location, &reverse](const String& s, Nil /*unused*/, Nil /*unused*/) -> Vallist {
                Value result = (int)s.value[0];

                return Vallist(result.with_origin(Origin(MultipleArgsOrigin{
                    .values = std::make_shared<Vallist>(values),
                    .location = location,
                    .reverse = reverse
                })));
            },
            [&result, &i, &values, &location, &reverse](const String& s, auto /*i*/, Nil) -> Vallist {
                std::string str = s.value;
                int i_int = try_value_as<Number::Int>(i, "byte", 2) - 1;
                int j = i_int;

                for (; i_int <= j && i_int < str.length(); i_int++){
                    Value v = (int) str[i_int];
                    values.emplace_back(i_int);
                    result.push_back(v.with_origin(MultipleArgsOrigin{
                        .values = std::make_shared<Vallist>(values),
                        .location = location,
                        .reverse = reverse
                    }));
                }
                return {result};
            },
            [&result, &j, &values, &location, &reverse](const String& s, Nil, auto /*j*/) -> Vallist {
                std::string str = s.value;
                int i_int = 0;
                int j_int = try_value_as<Number::Int>(j, "byte", 3) - 1;

                for (; i_int <= j_int && i_int < str.length(); i_int++) {
                    Value v = (int) str[i_int];
                    values.emplace_back(i_int);
                    result.push_back(v.with_origin(MultipleArgsOrigin{
                        .values = std::make_shared<Vallist>(values),
                        .location = location,
                        .reverse = reverse
                    }));
                }
                return {result};
            },
            [&result, &i, &j, &values, &location, &reverse](const String& s, auto /*i*/, auto /*j*/) -> Vallist {
                std::string str = s.value;
                int i_int = try_value_as<Number::Int>(i, "byte", 2) - 1;
                int j_int = try_value_as<Number::Int>(j, "byte", 3) - 1;

                for (; i_int <= j_int && i_int < str.length(); i_int++) {
                    Value v = (int) str[i_int];
                    values.emplace_back(i_int);
                    result.push_back(v.with_origin(MultipleArgsOrigin{
                        .values = std::make_shared<Vallist>(values),
                        .location = location,
                        .reverse = reverse
                    }));
                }
                return {result};
            },
            [&s](const auto& /*unused*/, const auto& /*unused*/, const auto& /*unused*/) -> Vallist{
                throw std::runtime_error(
            "bad argument #1 to 'next' (table expected, got " + s.type() + ")");
            }
        }, s.raw(), i.raw(), j.raw());
    }

    auto Char(const CallContext& ctx) -> Value {
        std::vector<char> result;
        int i = 0;

        for (const auto& arg : ctx.arguments()) {
            int lettr = try_value_as<Number::Int>(arg, "char", i);
            result.emplace_back((char) lettr);
            i++;
        }

        return {std::string(result.begin(), result.end())};
    }

    auto format(const CallContext &ctx) -> Value {
        auto formatstring = ctx.arguments().get(0);
        std::vector<Value> args(ctx.arguments().size() - 1);

        if (!formatstring.is_string() && formatstring.is_number()) {
            throw std::runtime_error("bad argument #1 to 'format' (string expected, got " + formatstring.type() + ")");
        }

        //remove first argument formatstring from arguments-list
        std::copy(ctx.arguments().begin() + 1, ctx.arguments().end(), args.begin());

        return parse_string(std::get<String>(formatstring).value, args);
    }

    auto len(const CallContext &ctx) -> Value {
        auto s = try_value_is_string(ctx.arguments().get(0), "len", 1);

        String str = std::get<String>(s.raw());
        return (long) str.value.length();
    }

    auto lower(const CallContext& ctx) -> Value {
        auto s = try_value_is_string(ctx.arguments().get(0), "lower", 1);

        std::string str = std::get<String>(Value(s)).value;
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){return std::tolower(c);});
        return str;
    }

    auto rep(const CallContext& ctx) -> Value {
        auto s = ctx.arguments().get(0);
        auto n = ctx.arguments().get(1);
        auto seperator = ctx.arguments().get(2);

        std::string str = std::get<String>(try_value_is_string(s, "rep", 1)).value;
        std::string sep;
        if (!seperator.is_nil()){
            sep = std::get<String>(try_value_is_string(seperator, "rep", 3)).value;
        }
        int reps = try_value_as<Number::Int>(n, "rep", 2);

        std::string result = str;
        for (int i = 1; i < reps; i++) {
            result += sep + str;
        }

        return result;
    }

    auto reverse(const CallContext& ctx) -> Value {
        auto s = try_value_is_string(ctx.arguments().get(0), "reverse", 1);

        std::string str = std::get<String>(s).value;
        std::reverse(str.begin(), str.end());
        return str;
    }

    auto sub(const CallContext& ctx) -> Value {
        auto s = ctx.arguments().get(0);
        auto i = ctx.arguments().get(1);
        auto j = ctx.arguments().get(2);

        std::string str = std::get<String>(try_value_is_string(s, "sub", 1)).value;
        int start = try_value_as<Number::Int>(i, "sub", 2) - 1;
        int distance = str.length(); // this overshoots the end of the string, but substr of std stops at the end
        return str.substr(start, distance);
    }

    auto upper(const CallContext& ctx) -> Value {
        auto s = try_value_is_string(ctx.arguments().get(0), "upper", 1);

        std::string str = std::get<String>(Value(s)).value;
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){return std::toupper(c);});
        return str;
    }
} // end of namespace string
} // end of namespace minilua
