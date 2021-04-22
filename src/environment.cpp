#include "MiniLua/environment.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "MiniLua/values.hpp"
#include "internal_env.hpp"

namespace minilua {

Environment::Environment() : Environment(&GLOBAL_ALLOCATOR) {}
Environment::Environment(MemoryAllocator* allocator) : impl(make_owning<Impl>(allocator)) {}
Environment::Environment(Impl impl) : impl(make_owning<Impl>(std::move(impl))) {}

Environment::Environment(const Environment&) = default;
// NOLINTNEXTLINE
Environment::Environment(Environment&&) = default;
Environment::~Environment() = default;
auto Environment::operator=(const Environment&) -> Environment& = default;
// NOLINTNEXTLINE
auto Environment::operator=(Environment&&) -> Environment& = default;
void swap(Environment& a, Environment& b) { swap(a.impl, b.impl); }
auto Environment::allocator() const -> MemoryAllocator* { return this->impl->inner.allocator(); }

auto Environment::make_table() const -> Table { return this->impl->inner.make_table(); }

void Environment::add(const std::string& name, Value value) {
    impl->inner.global().set(name, std::move(value));
}
void Environment::add(std::string&& name, Value value) {
    impl->inner.global().set(std::move(name), std::move(value));
}

// helper for the Environment::add_all methods
template <typename I>
static auto transform_values(I begin, I end) -> std::vector<std::pair<Value, Value>> {
    std::vector<std::pair<Value, Value>> to_insert;
    to_insert.reserve(std::distance(begin, end));

    std::transform(begin, end, std::back_inserter(to_insert), [](auto pair) {
        return std::make_pair(Value(std::move(pair.first)), Value(std::move(pair.second)));
    });

    return to_insert;
}

auto Environment::add_table(const std::string& name) -> Table {
    auto table = this->make_table();
    this->add(name, table);
    return table;
}

void Environment::add_all(std::unordered_map<std::string, Value> values) {
    for (auto [key, value] : std::move(values)) {
        // NOTE: can't move key because that would change the structure of the map
        impl->inner.global().set(key, std::move(value));
    }
}
void Environment::add_all(std::initializer_list<std::pair<std::string, Value>> values) {
    for (auto [key, value] : values) {
        impl->inner.global().set(key, std::move(value));
    }
}
void Environment::add_all(std::vector<std::pair<std::string, Value>> values) {
    for (auto [key, value] : std::move(values)) {
        impl->inner.global().set(std::move(key), std::move(value));
    }
}

auto Environment::get(const std::string& name) -> Value { return impl->inner.global().get(name); }

auto Environment::has(const std::string& name) -> bool { return impl->inner.global().has(name); }

void Environment::set_stdin(std::istream* in) { impl->inner.set_stdin(in); }
void Environment::set_stdout(std::ostream* out) { impl->inner.set_stdout(out); }
void Environment::set_stderr(std::ostream* err) { impl->inner.set_stderr(err); }

auto Environment::get_stdin() -> std::istream* { return impl->inner.get_stdin(); }
auto Environment::get_stdout() -> std::ostream* { return impl->inner.get_stdout(); }
auto Environment::get_stderr() -> std::ostream* { return impl->inner.get_stderr(); }

auto Environment::size() const -> size_t { return impl->inner.global().size(); }

void Environment::set_file(std::optional<std::shared_ptr<std::string>> file) {
    impl->inner.set_file(std::move(file));
}
auto Environment::get_file() const -> std::optional<std::shared_ptr<std::string>> {
    return impl->inner.get_file();
}

auto Environment::get_raw_impl() -> Impl& { return *this->impl; }

auto operator==(const Environment& a, const Environment& b) noexcept -> bool {
    return a.impl->inner.global() == b.impl->inner.global();
}
auto operator!=(const Environment& a, const Environment& b) noexcept -> bool { return !(a == b); }
auto operator<<(std::ostream& os, const Environment& self) -> std::ostream& {
    return os << "Environment{" << self.impl->inner.global() << "}";
}

}; // namespace minilua
