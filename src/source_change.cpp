#include <utility>

#include "MiniLua/source_change.hpp"

namespace minilua {

// struct Location
auto operator<<(std::ostream& os, const Location& self) -> std::ostream& {
    return os << "Location{ line = " << self.line << ", column = " << self.column
              << ", byte = " << self.byte << " }";
}

// struct Range
auto operator<<(std::ostream& os, const Range& self) -> std::ostream& {
    return os << "Range{ start = " << self.start << ", end = " << self.end << " }";
}

// struct SourceChange
SourceChange::SourceChange() = default;
SourceChange::SourceChange(SCSingle change) : change(change) {}
SourceChange::SourceChange(SCAnd change) : change(change) {}
SourceChange::SourceChange(SCOr change) : change(change) {}
SourceChange::SourceChange(Type change) : change(std::move(change)) {}

[[nodiscard]] auto SourceChange::origin() const -> const std::string& {
    return this->visit([](const auto& change) -> const std::string& { return change.origin; });
}
[[nodiscard]] auto SourceChange::hint() const -> const std::string& {
    return this->visit([](const auto& change) -> const std::string& { return change.hint; });
}
void SourceChange::set_origin(std::string origin) {
    this->visit([&origin](auto& change) { change.origin = std::move(origin); });
}
void SourceChange::set_hint(std::string hint) {
    this->visit([&hint](auto& change) { change.hint = std::move(hint); });
}

auto SourceChange::operator*() -> Type& { return change; }
auto SourceChange::operator*() const -> const Type& { return change; }
auto SourceChange::operator->() -> Type* { return &change; }

auto operator==(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool {
    return *lhs == *rhs;
}
auto operator!=(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const SourceChange& self) -> std::ostream& {
    os << "SourceChange{ change = ";
    self.visit([&os](const auto& change) { os << change; });
    return os << " }";
}

// struct SCSingle
SCSingle::SCSingle() = default;
SCSingle::SCSingle(Range range, std::string replacement)
    : range(range), replacement(std::move(replacement)) {}

auto operator==(const SCSingle& lhs, const SCSingle& rhs) noexcept -> bool {
    return lhs.range == rhs.range && lhs.replacement == rhs.replacement &&
           lhs.origin == rhs.origin && lhs.hint == rhs.hint;
}
auto operator!=(const SCSingle& lhs, const SCSingle& rhs) noexcept -> bool { return !(lhs == rhs); }
auto operator<<(std::ostream& os, const SCSingle& self) -> std::ostream& {
    return os << "SCSingle{ range = " << self.range << ", replacement = \"" << self.replacement
              << "\", origin = \"" << self.origin << "\", hint = \"" << self.hint << "\" }";
}

// struct SCAnd
SCAnd::SCAnd() = default;
SCAnd::SCAnd(std::vector<SourceChange> changes) : changes(std::move(changes)) {}

void SCAnd::add(SourceChange change) { changes.push_back(std::move(change)); }

auto operator==(const SCAnd& lhs, const SCAnd& rhs) noexcept -> bool {
    return lhs.changes == rhs.changes;
}
auto operator!=(const SCAnd& lhs, const SCAnd& rhs) noexcept -> bool { return !(lhs == rhs); }
auto operator<<(std::ostream& os, const SCAnd& self) -> std::ostream& {
    os << "SCAnd { ";
    const char* sep = "";
    for (const auto& change : self.changes) {
        os << sep << change;
        sep = ", ";
    }
    return os << " }";
}

// struct SCOr
SCOr::SCOr() = default;
SCOr::SCOr(std::vector<SourceChange> changes) : changes(std::move(changes)) {}

void SCOr::add(SourceChange change) { changes.push_back(change); }

auto operator==(const SCOr& lhs, const SCOr& rhs) noexcept -> bool {
    return lhs.changes == rhs.changes;
}
auto operator!=(const SCOr& lhs, const SCOr& rhs) noexcept -> bool { return !(lhs == rhs); }
auto operator<<(std::ostream& os, const SCOr& self) -> std::ostream& {
    os << "SCOr { ";
    const char* sep = "";
    for (const auto& change : self.changes) {
        os << sep << change;
        sep = ", ";
    }
    return os << " }";
}

} // namespace minilua
