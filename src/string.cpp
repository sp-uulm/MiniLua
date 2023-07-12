#include <array>
#include <cstddef>
#include <ios>
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
        const int default_precision = ss.precision();
        const int default_width = ss.width();
        const char default_fill = ' ';
        while (pos < s.length()) {
            ss << std::dec;
            ss << std::nouppercase;
            ss << std::noshowbase;
            ss << std::noshowpoint;
            ss << std::right;
            ss.precision(default_precision);
            ss.width(default_width);
            ss.fill(default_fill);

            int length = default_width;
            int precision = 1;
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

            ss << s.substr(pos, start_pos - pos);
            pos = ++start_pos;

            //search for flags and apply them
            for (bool in_flags = true; in_flags && pos < s.length(); ++pos) {
                switch (s[pos]) {
                    case '#':
                        alternate_form = true;
                        ss << std::showbase;
                        break;
                    case '0':
                        if (!left_adjust) {
                            zero_pad = true;
                            ss.fill('0');
                        }
                        break;
                    case ' ':
                        blank = true;
                        break;
                    case '+':
                        sign = true;
                        ss << std::showpoint;
                        break;
                    case '-':
                        zero_pad = false;
                        ss.fill(default_fill);
                        left_adjust = true;
                        ss << std::left;
                        break;
                    default:
                        in_flags = false;
                        --pos;
                        break;
                }
            }

            //search for width and
            if (pos < s.length() && isdigit(s[pos])) {
                size_t end;
                try {
                    length = std::stoi(std::string(s.substr(pos)), &end);
                    pos += end;
                    ss.width(length);
                } catch (const std::invalid_argument& e) {}
            }

            //search for precision
            if (pos + 1 < s.length() && s[pos] == '.' && isdigit(s[pos + 1])) {
                size_t end;
                try {
                    precision = std::stoi(std::string(s.substr(++pos)), &end);
                    pos += end;
                    ss.precision(precision);
                } catch (const std::invalid_argument& e) {}
            }

            //check for conversion type
            if (pos >= s.length()) {
                throw std::runtime_error("invalid conversion '" + std::string(s.substr(start_pos, pos - start_pos)) + "' to 'format'");
            }
            auto arg_value = args.at(numEscapes);
            switch (s[pos++]) {
                case 'd':
                case 'i': {
                    if (precision == 0) {
                        zero_pad = false;
                        ss.fill(default_fill);
                    }
                    const auto imm = try_value_as<Number::Int>(arg_value, "format", numEscapes+2);
                    if (imm == 0 && precision == 0) { continue; } // output 0 with precision 0 results in empty output
                    if (blank && imm >= 0) { ss << " "; }
                    ss << std::right;
                    ss.fill('0');
                    ss.width(precision);
                    ss << imm;
                    break;
                }
                case 'o': {
                    if (precision == 0) {
                        zero_pad = false;
                        ss.fill(default_fill);
                    }
                    ss << std::oct;
                    const auto imm = try_value_as<Number::Int>(arg_value, "format", numEscapes+2);
                    if (imm == 0 && precision == 0) { continue; } // output 0 with precision 0 results in empty output
                    ss << std::right;
                    ss.fill('0');
                    ss.width(precision);
                    ss << imm;
                    break;
                }
                case 'u': {
                    if (alternate_form) { throw std::runtime_error("invalid conversion specification: '" + std::string(s.substr(start_pos, pos - start_pos)) + "'"); }
                    if (precision == 0) {
                        zero_pad = false;
                        ss.fill(default_fill);
                    }
                    const unsigned long imm = try_value_as<Number::Int>(arg_value, "format", numEscapes + 2);
                    if (imm == 0 && precision == 0) { continue; } // output 0 with precision 0 results in empty output
                    ss << std::right;
                    ss.fill('0');
                    ss.width(precision);
                    ss << imm;
                    break;
                }

                case 'X':
                    ss << std::uppercase;
                case 'x':{
                    if (precision == 0) {
                        zero_pad = false;
                        ss.fill(default_fill);
                    }
                    const auto imm = try_value_as<Number::Float>(arg_value, "format", numEscapes+2);
                    if (imm == 0 && precision == 0) { continue; } // output 0 with precision 0 results in empty output
                    ss << std::right;
                    ss << std::hex;
                    ss.fill('0');
                    ss.width(precision);
                    ss << imm;
                    break;
                }
                case 'E':
                    ss << std::uppercase;
                case 'e':{
                    const auto imm = try_value_as<Number::Float>(arg_value, "format", numEscapes+2);
                    ss << std::scientific;
                    ss << imm;
                    ss << std::defaultfloat;
                    break;
                }
                case 'f':{
                    const auto imm = try_value_as<Number::Float>(arg_value, "format", numEscapes+2);
                    std::cout << "imm: " << imm << "\n";
                    ss << std::fixed;
                    ss << imm;
                    ss << std::defaultfloat;
                    break;
                }
                case 'G':
                    ss << std::uppercase;
                case 'g': {
                    const auto imm = try_value_as<Number::Float>(arg_value, "format", numEscapes+2);
                    ss << std::defaultfloat;
                    ss << imm;
                    break;
                }
                case 'A':
                    ss << std::uppercase;
                case 'a': {
                    const auto imm = try_value_as<Number::Float>(arg_value, "format", numEscapes+2);
                    ss << std::hexfloat;
                    ss << imm;
                    ss << std::defaultfloat;
                    break;
                }
                case 'c': {
                    if (alternate_form || zero_pad || blank || sign) {
                        throw std::runtime_error("invalid conversion specification: '" + std::string(s.substr(start_pos, pos - start_pos)) + "'");
                    }
                    const char imm = try_value_as<Number::Int>(arg_value, "format", numEscapes+2);
                    ss << imm;
                    break;
                }
                case 's': {
                    ss << arg_value.to_string();
                    break;
                }
                case '%':
                    //no flags allowed
                    if (alternate_form || zero_pad || sign || blank || left_adjust) {
                        throw std::runtime_error("invalid conversion specification: '" + std::string(s.substr(start_pos, pos - start_pos)) + "'");
                    }
                    ss << "%";
                    break;
                default:
                    throw std::runtime_error("invalid conversion '" + std::string(s.substr(start_pos, pos - start_pos)) + "' to 'format'");
                    break;
            }

            numEscapes++;
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
        auto s = ctx.arguments().get(0);
        auto i = ctx.arguments().get(1);
        auto j = ctx.arguments().get(2);
        std::vector<Value> result;

        return std::visit(overloaded{
            [](const String& s, Nil /*unused*/, Nil /*unused*/) -> Vallist {
                return Vallist((int)s.value[0]);
            },
            [&result, &i](const String& s, auto /*i*/, Nil) -> Vallist {
                std::string str = s.value;
                int i_int = try_value_as<Number::Int>(i, "byte", 2) - 1;
                int j = i_int;

                for (; i_int <= j && i_int < str.length(); i_int++){
                    result.emplace_back((int) s.value[i_int]);
                }
                return {result};
            },
            [&result, &j](const String& s, Nil, auto /*j*/) -> Vallist {
                std::string str = s.value;
                int i_int = 0;
                int j_int = try_value_as<Number::Int>(j, "byte", 3) - 1;

                for (; i_int <= j_int && i_int < str.length(); i_int++) {
                    result.emplace_back((int) str[i_int]);
                }
                return {result};
            },
            [&result, &i, &j](const String& s, auto /*i*/, auto /*j*/) -> Vallist {
                std::string str = s.value;
                int i_int = try_value_as<Number::Int>(i, "byte", 2) - 1;
                int j_int = try_value_as<Number::Int>(j, "byte", 3) - 1;

                for (; i_int <= j_int && i_int < str.length(); i_int++) {
                    result.emplace_back((int) str[i_int]);
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
        std::vector<Value> args;
        const auto& tmp_args = ctx.arguments();
        auto iterator = tmp_args.begin()+1;

        //remove first argument formatstring from arguments-list
        for (auto arg = *iterator;iterator != tmp_args.end();iterator++) {
            args.push_back(arg);
        }

        if (!formatstring.is_string() && formatstring.is_number()) {
            throw std::runtime_error("bad argument #1 to 'format' (string expected, got " + formatstring.type() + ")");
        }

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
