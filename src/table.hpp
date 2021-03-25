#ifndef MINILUA_DETAILS_TABLE_H
#define MINILUA_DETAILS_TABLE_H

#include <MiniLua/values.hpp>

#include <optional>
#include <unordered_map>

namespace minilua {

struct TableImpl {
    std::unordered_map<Value, Value> value;

    std::optional<Table> metatable;

    void set(const Value& key, Value value);
    auto calc_border() const -> int;
};

struct Table::iterator::Impl {
    std::unordered_map<Value, Value>::iterator iter;
};

} // namespace minilua

#endif
