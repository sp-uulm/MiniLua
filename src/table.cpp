#include "table.hpp"
#include <MiniLua/allocator.hpp>

#include <algorithm>
#include <set>
#include <utility>

namespace minilua {

// struct TableImpl
void TableImpl::set(const Value& key, Value value) { this->value[key] = std::move(value); }

auto TableImpl::calc_border() const -> int {
    auto has_value = [this](int key) -> bool {
        auto value = this->value.find(key);
        return value != this->value.end() && value->second != Nil();
    };

    if (!has_value(1)) {
        return 0;
    }

    // Binary search (slightly modified)
    // to find the integer key where the predicate for the border is true
    //   (border == 0 or t[border] ~= nil) and t[border + 1] == nil
    //
    // This does not in all cases return the same border as the official lua
    // interpreter. But this is premitted.
    // See: https://www.lua.org/manual/5.3/manual.html#3.4.7
    int lower = 1;
    int upper = this->value.size();

    while (lower <= upper) {
        int border = (upper + lower) / 2;

        bool it_has = has_value(border);
        bool next_has = has_value(border + 1);

        if (it_has && next_has) {
            lower = border + 1;
        } else if (!it_has) {
            upper = border - 1;
        } else if (it_has && !next_has) {
            return border;
        }
    }

    throw std::runtime_error("implemented of operator # has a bug");
}

// struct Table
Table::iterator::iterator() = default;
Table::iterator::iterator(const Table::iterator&) = default;
Table::iterator::~iterator() = default;

auto Table::iterator::operator=(const Table::iterator&) -> Table::iterator& = default;
auto Table::iterator::operator==(const Table::iterator& other) const -> bool {
    return this->impl->iter == other.impl->iter;
}
auto Table::iterator::operator!=(const Table::iterator& other) const -> bool {
    return !(*this == other);
}
auto Table::iterator::operator++() -> Table::iterator& {
    this->impl->iter++;
    return *this;
}
auto Table::iterator::operator++(int) -> Table::iterator {
    auto tmp = *this;
    ++(this->impl->iter);
    return tmp;
}

auto Table::iterator::operator*() const -> Table::iterator::reference { return *this->impl->iter; }
auto Table::iterator::operator->() const -> Table::iterator::pointer {
    return this->impl->iter.operator->();
}

struct Table::const_iterator::Impl {
    std::unordered_map<Value, Value>::const_iterator iter;
};
Table::const_iterator::const_iterator() = default;
Table::const_iterator::const_iterator(const Table::const_iterator&) = default;
Table::const_iterator::~const_iterator() = default;

auto Table::const_iterator::operator=(const Table::const_iterator&)
    -> Table::const_iterator& = default;
auto Table::const_iterator::operator==(const Table::const_iterator& other) const -> bool {
    return this->impl->iter == other.impl->iter;
}
auto Table::const_iterator::operator!=(const Table::const_iterator& other) const -> bool {
    return !(*this == other);
}
auto Table::const_iterator::operator++() -> Table::const_iterator& {
    this->impl->iter++;
    return *this;
}
auto Table::const_iterator::operator++(int) -> Table::const_iterator {
    auto tmp = *this;
    ++(this->impl->iter);
    return tmp;
}

auto Table::const_iterator::operator*() const -> Table::const_iterator::reference {
    return *this->impl->iter;
}
auto Table::const_iterator::operator->() const -> Table::const_iterator::pointer {
    return this->impl->iter.operator->();
}

Table::Table() : Table(&GLOBAL_ALLOCATOR) {}
Table::Table(MemoryAllocator* allocator)
    : allocator(allocator), impl(allocator->allocate_table()) {}
Table::Table(std::unordered_map<Value, Value> values, MemoryAllocator* allocator)
    : Table(allocator) {
    for (const auto& [key, value] : values) {
        this->impl->value.insert_or_assign(key, value);
    }
}
Table::Table(
    std::initializer_list<std::pair<const Value, Value>> values, MemoryAllocator* allocator)
    : Table(allocator) {
    for (const auto& [key, value] : values) {
        this->impl->value.insert_or_assign(key, value);
    }
}

Table::Table(const Table& other, MemoryAllocator* allocator) : Table(allocator) {
    for (const auto& [key, value] : other) {
        this->impl->value.insert_or_assign(Value(key, allocator), Value(value, allocator));
    }
}

Table::Table(const Table& other) = default;
Table::Table(Table&& other) noexcept : Table() {
    // NOTE: it is very important to make sure `this` is a valid object before
    // calling `swap`. Otherwise accessing other after calling this constructor
    // will be undefined behaviour (and likely segfaults).
    swap(*this, other);
};
Table::~Table() noexcept = default;
auto Table::operator=(const Table& other) -> Table& = default;
auto Table::operator=(Table&& other) noexcept -> Table& {
    swap(*this, other);
    return *this;
};
void swap(Table& self, Table& other) {
    std::swap(self.allocator, other.allocator);
    std::swap(self.impl, other.impl);
}

auto Table::border() const -> int { return this->impl->calc_border(); }

auto Table::get(const Value& key) -> Value {
    auto value = impl->value.find(key);
    if (value == impl->value.end()) {
        return Nil();
    } else {
        return Value(value->second);
    }
}
auto Table::has(const Value& key) -> bool { return impl->value.find(key) != impl->value.end(); }
void Table::set(const Value& key, Value value) { impl->set(key, std::move(value)); }
void Table::set(Value&& key, Value value) { impl->set(key, std::move(value)); }
void Table::set_all(const Table& other) {
    for (const auto& [key, value] : other) {
        this->set(key, value);
    }
}
[[nodiscard]] auto Table::size() const -> size_t { return impl->value.size(); }

auto Table::begin() -> Table::iterator {
    Table::iterator iterator;
    iterator.impl->iter = this->impl->value.begin();
    return iterator;
}
[[nodiscard]] auto Table::begin() const -> Table::const_iterator {
    Table::const_iterator iterator;
    iterator.impl->iter = this->impl->value.cbegin();
    return iterator;
}
[[nodiscard]] auto Table::cbegin() const -> Table::const_iterator {
    Table::const_iterator iterator;
    iterator.impl->iter = this->impl->value.cbegin();
    return iterator;
}

auto Table::end() -> Table::iterator { return Table::iterator(); }
[[nodiscard]] auto Table::end() const -> Table::const_iterator { return Table::const_iterator(); }
[[nodiscard]] auto Table::cend() const -> Table::const_iterator { return Table::const_iterator(); }

[[nodiscard]] auto Table::to_literal() const -> std::string {
    // NOTE: recursive table check needs to be in a lambda because Table::Impl is private and we
    // don't want a helper function in the public interface
    std::set<TableImpl*> visited;
    auto table_to_literal = [&visited](const Table& table, const auto& rec) -> std::string {
        visited.insert(table.impl);
        auto visit_nested = [&rec, &visited](const Value& value) -> std::string {
            return std::visit(
                overloaded{
                    [&visited, &rec](const Table& nested) -> std::string {
                        if (visited.find(nested.impl) != visited.end()) {
                            throw std::runtime_error(
                                "self recursive table can't be converted to literal");
                        }
                        return rec(nested, rec);
                    },
                    [](const auto& nested) -> std::string { return nested.to_literal(); }},
                value.raw());
        };

        // TODO should we sort keys for consistency?
        std::string str;
        str.append("{");

        const char* sep = " ";

        for (const auto& [key, value] : table.impl->value) {
            if (value.is_nil()) {
                continue;
            }

            str.append(sep);

            // use strings directly as identifiers if possible
            if (key.is_valid_identifier()) {
                str.append(std::get<String>(key.raw()).value);
            } else {
                str.append("[");
                str.append(visit_nested(key));
                str.append("]");
            }

            str.append(" = ");
            str.append(visit_nested(value));
            sep = ", ";
        }

        if (!table.impl->value.empty()) {
            str.append(" ");
        }

        str.append("}");
        return str;
    };

    return table_to_literal(*this, table_to_literal);
}

auto Table::operator[](const Value& index) -> Value& { return impl->value[index]; }
auto Table::operator[](const Value& index) const -> const Value& { return impl->value[index]; }

Table::operator bool() const { return true; }

auto operator==(const Table& a, const Table& b) noexcept -> bool { return a.impl == b.impl; }
auto operator!=(const Table& a, const Table& b) noexcept -> bool { return !(a == b); }
auto operator<<(std::ostream& os, const Table& self) -> std::ostream& {
    os << "Table { ";
    for (const auto& [key, value] : self.impl->value) {
        os << "[" << key << "] = " << value << ", ";
    }
    return os << " }";
}

auto Table::next(const Value& key) const -> Vallist {
    return std::visit(
        overloaded{
            [this](Nil /*unused*/) {
                auto it = impl->value.begin();
                if (it != impl->value.end()) {
                    std::pair<Value, Value> p = *it;
                    return Vallist({p.first, p.second});
                } else {
                    return Vallist();
                }
            },
            [this](auto key) {
                auto it = impl->value.find(key);
                if (it != impl->value.end()) {
                    // key in table, but last eleement of table
                    if (++it == impl->value.end()) {
                        return Vallist();
                    } else {
                        // key is somewhere in the table
                        std::pair<Value, Value> p = *it;
                        return Vallist({p.first, p.second});
                    }
                    // Key not in table
                } else {
                    throw std::runtime_error("Invalid key to 'next'");
                }
            }},
        key.raw());
}

auto Table::get_metatable() const -> std::optional<Table> { return this->impl->metatable; }
void Table::set_metatable(std::optional<Table> metatable) {
    this->impl->metatable = std::move(metatable);
}

} // namespace minilua
