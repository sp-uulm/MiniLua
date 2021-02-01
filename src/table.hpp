#ifndef MINILUA_DETAILS_TABLE_H
#define MINILUA_DETAILS_TABLE_H

#include <MiniLua/values.hpp>

#include <unordered_map>

namespace minilua {

struct Table::Impl {
    std::unordered_map<Value, Value> value;
};

struct Table::iterator::Impl {
    std::unordered_map<Value, Value>::iterator iter;
};

} // namespace minilua

#endif
