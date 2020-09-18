#include "MiniLua/environment.hpp"

#include <string>

#include "MiniLua/values.hpp"

namespace minilua {

struct Environment::Impl {
    std::unordered_map<std::string, Value> global;
};

Environment::Environment() = default;
Environment::Environment(const Environment&) = default;
// NOLINTNEXTLINE
Environment::Environment(Environment&&) = default;
Environment::~Environment() = default;
auto Environment::operator=(const Environment&) -> Environment& = default;
// NOLINTNEXTLINE
auto Environment::operator=(Environment &&) -> Environment& = default;
void swap(Environment& a, Environment& b) {
    swap(a.impl, b.impl);
}

void Environment::add_default_stdlib() {}
void Environment::add(const std::string& name, Value value) {
    impl->global.insert_or_assign(name, value);
}
void Environment::add(std::string&& name, Value value) {
    impl->global.insert_or_assign(std::move(name), value);
}

void Environment::add_all(std::unordered_map<std::string, Value> values) {
    impl->global.insert(values.begin(), values.end());
}
void Environment::add_all(std::initializer_list<std::pair<const std::string, Value>> values) {
    impl->global.insert(values);
}

auto Environment::get(const std::string& name) -> Value& {
    return impl->global.at(name);
}

auto Environment::size() const -> size_t {
    return impl->global.size();
}

auto operator==(const Environment& a, const Environment& b) noexcept -> bool {
    return a.impl->global == b.impl->global;
}
auto operator!=(const Environment& a, const Environment& b) noexcept -> bool {
    return !(a == b);
}
auto operator<<(std::ostream& os, const Environment& self) -> std::ostream& {
    os << "Environment{";
    for (const auto& [key, value] : self.impl->global) {
        os << "[\"" << key << "\"] = " << value << ", ";
    }
    return os << "}";
}

}; // namespace minilua
