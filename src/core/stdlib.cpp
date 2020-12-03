#include <sstream>
#include <regex>
#include <utility>
#include <typeinfo>

#include "MiniLua/values.hpp"

namespace minilua {
auto to_string(const CallContext& ctx) -> Value {
    auto arg = ctx.arguments().get(0);
    return std::visit(overloaded{
        [](Bool b) { return b.value ? "true" : "false"; },
        [](Number n) { return n.to_literal(); },
        [](String s) { return s.value; },
        [](Table t) { //TODO: maybe improve the way to get the address.
        //at the moment it could be that every time you call it the 
        //address has changed because of the change in the stack
            ostringstream get_address;
            get_address << &t;
            return get_address.str(); },
        [](Function f) { 
            ostringstream get_address;
            get_address << &f;
            return get_address.str(); },
        [](Nil nil) { return "nil"; }
        //TODO: add to_string for metatables
    }, arg.raw());
}

auto to_number(const CallContext& ctx) -> Value {
    auto number = ctx.arguments().get(0);
    auto base = ctx.arguments().get(1);
    
    return std::visit(overloaded{
        [](String number, Nil nil){
                //Yes: parse number to double
                //No: return Nil
                std::regex pattern_number = "\\s*\\d+\\.?\\d*";
                std::regex pattern_hex = "\\s*0[xX][\\dA-Fa-f]+\\.?[\\dA-Fa-f]*";
                std::regex pattern_exp = "\\s*\\d+\\.?\\d*[eE]-?\\d+";

                if(std::regex_match(number, pattern_number) || std::regex_match(number, pattern_hex) || std::regex_match(number, pattern_exp)){
                    return std::stod(number);
                }else{
                    return Nil;
                }
            }
        },
        [](String number, Number base){
            //match again with pattern, but this time with 1 .
                if(base < 2 || base > 36){
                    //throw error
                }else{
                    std::regex pattern_number = "\\s*\\d+\\.\\d*";
                    std::regex pattern_hex = "\\s*0[xX][\\dA-Za-z]+\\.[\\dA-Za-z]*";
                    std::regex pattern_exp = "\\s*\\d+\\.\\d*[eE]-?\\d+";
                    //parse number to double
                    if(std::regex_match(number, pattern_number) || std::regex_match(number, pattern_hex)){
                        auto parts = splitString(number, ".");
                        int precomma = std:stoi(parts.first, , base);
                        int postcomma = std::stoi(parts.second, , base);
                        return precomma + postcomma * std::pow(base, parts.second.length);
                    }else if (std::regex_match(number, pattern_exp)) {
                        auto number_exp = splitString(number, "e");
                        int exp = std::stoi(number_exp.second);
                        auto parts = splitString(number, ".");
                        int precomma = std:stoi(parts.first, , base);
                        int postcomma = std::stoi(parts.second, , base);
                        double number_res = precomma + precomma + postcomma * std::pow(base, parts.second.length);
                        return number_res * std::pow(base, exp);
                    }else{
                        return Nil;
                    }
                }
        },
        [](Number number, Nil nil){
            return number;
        }
    }, number.raw(), base.raw());
}

auto splitString(std::string s, char c) -> std::pair<std::string, std::string> {
    std::pair<std::string, std::string> result;
    std::stringstream split(s);
    std::string tmp;
    std::getline(split, tmp, c);
    result.first = tmp;
    std::getline(split, tmp, c);
    result.second = tmp;
    return result;
}
}