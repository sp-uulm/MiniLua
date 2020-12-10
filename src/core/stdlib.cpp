#include <sstream>
#include <regex>
#include <variant>
//#include <typeinfo>

#include "MiniLua/stdlib.hpp"
#include "MiniLua/values.hpp"

namespace minilua {
static auto split_string(std::string s, char c) -> std::pair<std::string, std::string> {
    std::pair<std::string, std::string> result;
    std::stringstream split(s);
    std::string tmp;
    std::getline(split, tmp, c);
    result.first = tmp;
    std::getline(split, tmp, c);
    result.second = tmp;
    return result;
}

auto to_string(const CallContext& ctx) -> Value {
    auto arg = ctx.arguments().get(0);
    return std::visit(overloaded{
        [](Bool b) -> Value { return b.value ? "true" : "false"; },
        [](Number n) -> Value { return n.to_literal(); },
        [](String s) -> Value { return s.value; },
        [](Table t) -> Value { //TODO: maybe improve the way to get the address.
        //at the moment it could be that every time you call it the 
        //address has changed because of the change in the stack
            ostringstream get_address;
            get_address << &t;
            return get_address.str(); },
        [](Function f) -> Value { 
            ostringstream get_address;
            get_address << &f;
            return get_address.str(); },
        [](Nil nil) -> Value { return "nil"; }
        //TODO: add to_string for metatables
    }, arg.raw());
}

auto to_number(const CallContext& ctx) -> Value {
    auto number = ctx.arguments().get(0);
    auto base = ctx.arguments().get(1);
    
    return std::visit(overloaded{
        [](String number, Nil nil) -> Value {
            //Yes: parse number to double
            //No: return Nil
            std::regex pattern_number ("\\s*\\d+\\.?\\d*");
            std::regex pattern_hex ("\\s*0[xX][\\dA-Fa-f]+\\.?[\\dA-Fa-f]*");
            std::regex pattern_exp ("\\s*\\d+\\.?\\d*[eE]-?\\d+");

            if(std::regex_match(number.value, pattern_number) || std::regex_match(number.value, pattern_hex) || std::regex_match(number.value, pattern_exp)){
                return std::stod(number.value);
            }else{
                return Nil();
            }
        },
        [](String number, Number base) -> Value {
            //match again with pattern, but this time with 1 .
            if(base < 2 || base > 36){
                //throw error
            }else{
                std::regex pattern_number ("\\s*\\d+\\.\\d*");
                std::regex pattern_hex ("\\s*0[xX][\\dA-Za-z]+\\.[\\dA-Za-z]*");
                std::regex pattern_exp ("\\s*\\d+\\.\\d*[eE]-?\\d+");
                //parse number to double
                if(std::regex_match(number.value, pattern_number) || std::regex_match(number.value, pattern_hex)){
                    auto parts = split_string(number.value, '.');
                    int precomma = std::stoi(parts.first, 0, base.value);
                    int postcomma = std::stoi(parts.second, 0, base.value);
                    return precomma + postcomma * std::pow(base.value, parts.second.size());
                }else if (std::regex_match(number.value, pattern_exp)) {
                    auto number_exp = split_string(number.value, 'e');
                    int exp = std::stoi(number_exp.second);
                    auto parts = split_string(number_exp.first, '.');
                    int precomma = std::stoi(parts.first, 0, base.value);
                    int postcomma = std::stoi(parts.second, 0, base.value);
                    double number_res = precomma + precomma + postcomma * std::pow(base.value, parts.second.size());
                    return number_res * std::pow(base.value, exp);
                }else{
                    return Nil();
                }
            }
        },
        [](Number number, Nil /*unused*/) -> Value {
            return number;
        },
        [](auto a, auto b) -> Value { return Nil();}
    }, number.raw(), base.raw());
}

auto type(const CallContext& ctx) -> Value {
    auto v = ctx.arguments().get(0);

    //TODO: Change return to static variable of Struct of type
    return std::visit(overloaded{
        [](Bool /*b*/) -> Value { return "boolean"; },
        [](Number  /*n*/) -> Value { return "number"; },
        [](String  /*s*/) -> Value { return "string"; },
        [](Table  /*t*/) -> Value { return "table"; },
        [](Function  /*f*/) -> Value { return "function"; },
        [](Nil  /*nil*/) -> Value { return "nil"; }
        //TODO: add type for metatables
    }, v.raw());
}
}