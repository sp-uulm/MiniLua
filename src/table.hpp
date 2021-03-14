#ifndef MINILUA_DETAILS_TABLE_H
#define MINILUA_DETAILS_TABLE_H

#include <MiniLua/values.hpp>

#include <unordered_map>

namespace minilua {

struct TableImpl {
    std::unordered_map<Value, Value> value;

    void set(const Value& key, Value value);
    auto calc_border() const -> int;
};

struct Table::iterator::Impl {
    std::unordered_map<Value, Value>::iterator iter;
};

} // namespace minilua

#endif
