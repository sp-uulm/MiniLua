#include "MiniLua/environment.hpp"

#include <string>

#include "MiniLua/values.hpp"

namespace minilua {

struct Environment::Impl {
    std::unordered_map<std::string, Value> global;
    // iostreams
    std::istream* in;
    std::ostream* out;
    std::ostream* err;
};

Environment::Environment()
    : impl(make_owning<Impl>(Impl{
          .global = std::unordered_map<std::string, Value>(),
          .in = &std::cin,
          .out = &std::cout,
          .err = &std::cerr})) {}
Environment::Environment(const Environment&) = default;
// NOLINTNEXTLINE
Environment::Environment(Environment&&) = default;
Environment::~Environment() = default;
auto Environment::operator=(const Environment&) -> Environment& = default;
// NOLINTNEXTLINE
auto Environment::operator=(Environment &&) -> Environment& = default;
void swap(Environment& a, Environment& b) { swap(a.impl, b.impl); }

void Environment::add_default_stdlib() {
    // TODO add actual stdlib
}
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

auto Environment::get(const std::string& name) -> Value& { return impl->global.at(name); }

void Environment::set_stdin(std::istream* in) {
    if (in == nullptr) {
        throw std::invalid_argument("can't use nullptr as stdin");
    }
    impl->in = in;
}
void Environment::set_stdout(std::ostream* out) {
    if (out == nullptr) {
        throw std::invalid_argument("can't use nullptr as stdout");
    }
    impl->out = out;
}
void Environment::set_stderr(std::ostream* err) {
    if (err == nullptr) {
        throw std::invalid_argument("can't use nullptr as stderr");
    }
    impl->err = err;
}

auto Environment::get_stdin() -> std::istream* { return impl->in; }
auto Environment::get_stdout() -> std::ostream* { return impl->out; }
auto Environment::get_stderr() -> std::ostream* { return impl->err; }

auto Environment::size() const -> size_t { return impl->global.size(); }

auto operator==(const Environment& a, const Environment& b) noexcept -> bool {
    return a.impl->global == b.impl->global;
}
auto operator!=(const Environment& a, const Environment& b) noexcept -> bool { return !(a == b); }
auto operator<<(std::ostream& os, const Environment& self) -> std::ostream& {
    os << "Environment{";
    for (const auto& [key, value] : self.impl->global) {
        os << "[\"" << key << "\"] = " << value << ", ";
    }
    return os << "}";
}

}; // namespace minilua
