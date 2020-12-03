#include <sstream>
#include <regex>

#include "Minilua/values.hpp"

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
    }, arg);
}

auto to_number(const CallContext& ctx) -> Value {
    auto number = ctx.arguments().get(0);
    auto base = ctx.arguments().get(1);
    
    if(base == Nil){
        //matches pattern \w*\d+\.?\d*
        //Yes: parse number to double
        //No: return Nil
        std::regex pattern_number = "\\s*\\d+\\.?\\d*";
        std::regex pattern_hex = "\\s*0x[\\dA-Fa-f]+\\.?[\\dA-Fa-f]*";
        std::regex pattern_exp = "\\s*\\d+\\.?\\d*[eE]-?\\d+";

        if(std::regex_match(number, pattern_number)){

        }else if (std::regex_match(number, pattern_hex)){

        }else if (std::regex_match(number, pattern_exp)) {

        }else{
            //TODO: Not a valid number
        }
    }else{
        //match again with pattern, but this time with 1 .
        //no .:
        if(base < 2 || base > 36){
            //throw error
        }else{
            //parse number to double
        }
    }
    return std::visit(overloaded{
        [](Bool b) { return b.value ? "true" : "false"; },
        [](Number n) { return n.to_literal(); },
        [](String s) { return s.value; },
        [](Table t) { //TODO: maybe improve the way to get the address.
        //at the moment it could be that every time you call it the address 
        //has changed because of the cahange in the stack
            ostringstream get_address;
            get_address << &t;
            return get_address.str(); },
        [](Function f) { 
            ostringstream get_address;
            get_address << &f;
            return get_address.str(); },
        [](Nil nil) { return "nil"; }
        //TODO: add to_string for metatables
    }, arg);
}
}