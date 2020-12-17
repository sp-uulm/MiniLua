#include "internal_env.hpp"

namespace minilua {

Env::Env() : table(), in(&std::cin), out(&std::cout), err(&std::cerr) {}

auto Env::global() -> std::unordered_map<std::string, Value>& { return this->table; }
auto Env::global() const -> const std::unordered_map<std::string, Value>& { return this->table; }

void Env::set_stdin(std::istream* in) {
    if (in == nullptr) {
        throw std::invalid_argument("can't use nullptr as stdin");
    }
    this->in = in;
}
void Env::set_stdout(std::ostream* out) {
    if (out == nullptr) {
        throw std::invalid_argument("can't use nullptr as stdout");
    }
    this->out = out;
}
void Env::set_stderr(std::ostream* err) {
    if (err == nullptr) {
        throw std::invalid_argument("can't use nullptr as stderr");
    }
    this->err = err;
}

auto Env::get_stdin() -> std::istream* { return this->in; }
auto Env::get_stdout() -> std::ostream* { return this->out; }
auto Env::get_stderr() -> std::ostream* { return this->err; }

}; // namespace minilua
