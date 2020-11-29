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
SourceChangeTree::SourceChangeTree(SourceChange change) : change(change) {}
SourceChangeTree::SourceChangeTree(SourceChangeCombination change) : change(change) {}
SourceChangeTree::SourceChangeTree(SourceChangeAlternative change) : change(change) {}
SourceChangeTree::SourceChangeTree(Type change) : change(std::move(change)) {}

[[nodiscard]] auto SourceChangeTree::origin() const -> const std::string& {
    return this->visit([](const auto& change) -> const std::string& { return change.origin; });
}
auto SourceChangeTree::origin() -> std::string& {
    return this->visit([](auto& change) -> std::string& { return change.origin; });
}
[[nodiscard]] auto SourceChangeTree::hint() const -> const std::string& {
    return this->visit([](const auto& change) -> const std::string& { return change.hint; });
}
auto SourceChangeTree::hint() -> std::string& {
    return this->visit([](auto& change) -> std::string& { return change.hint; });
}

[[nodiscard]] auto SourceChangeTree::collect_first_alternative() const
    -> std::vector<SourceChange> {
    std::vector<SourceChange> changes;

    this->visit_first_alternative(
        [&changes](const SourceChange& single) { changes.push_back(single); });

    return changes;
}

auto SourceChangeTree::operator*() -> Type& { return change; }
auto SourceChangeTree::operator*() const -> const Type& { return change; }
auto SourceChangeTree::operator->() -> Type* { return &change; }

auto operator==(const SourceChangeTree& lhs, const SourceChangeTree& rhs) noexcept -> bool {
    return *lhs == *rhs;
}
auto operator!=(const SourceChangeTree& lhs, const SourceChangeTree& rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const SourceChangeTree& self) -> std::ostream& {
    os << "SourceChange{ change = ";
    self.visit([&os](const auto& change) { os << change; });
    return os << " }";
}

// struct SCSingle
SourceChange::SourceChange(Range range, std::string replacement)
    : range(range), replacement(std::move(replacement)) {}

auto operator==(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool {
    return lhs.range == rhs.range && lhs.replacement == rhs.replacement &&
           lhs.origin == rhs.origin && lhs.hint == rhs.hint;
}
auto operator!=(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const SourceChange& self) -> std::ostream& {
    return os << "SCSingle{ range = " << self.range << ", replacement = \"" << self.replacement
              << "\", origin = \"" << self.origin << "\", hint = \"" << self.hint << "\" }";
}

// struct SCAnd
SourceChangeCombination::SourceChangeCombination() = default;
SourceChangeCombination::SourceChangeCombination(std::vector<SourceChangeTree> changes)
    : changes(std::move(changes)) {}

void SourceChangeCombination::add(SourceChangeTree change) { changes.push_back(std::move(change)); }

auto operator==(const SourceChangeCombination& lhs, const SourceChangeCombination& rhs) noexcept
    -> bool {
    return lhs.changes == rhs.changes;
}
auto operator!=(const SourceChangeCombination& lhs, const SourceChangeCombination& rhs) noexcept
    -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const SourceChangeCombination& self) -> std::ostream& {
    os << "SCAnd { ";
    os << "origin = \"" << self.origin << "\", ";
    os << "hint = \"" << self.hint << "\"";
    for (const auto& change : self.changes) {
        os << ", " << change;
    }
    return os << " }";
}

// struct SCOr
SourceChangeAlternative::SourceChangeAlternative() = default;
SourceChangeAlternative::SourceChangeAlternative(std::vector<SourceChangeTree> changes)
    : changes(std::move(changes)) {}

void SourceChangeAlternative::add(SourceChangeTree change) { changes.push_back(std::move(change)); }
void SourceChangeAlternative::add_if_some(std::optional<SourceChangeTree> change) {
    if (change) {
        this->add(change.value());
    }
}

auto operator==(const SourceChangeAlternative& lhs, const SourceChangeAlternative& rhs) noexcept
    -> bool {
    return lhs.changes == rhs.changes;
}
auto operator!=(const SourceChangeAlternative& lhs, const SourceChangeAlternative& rhs) noexcept
    -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const SourceChangeAlternative& self) -> std::ostream& {
    os << "SCOr { ";
    os << "origin = \"" << self.origin << "\", ";
    os << "hint = \"" << self.hint << "\"";
    for (const auto& change : self.changes) {
        os << ", " << change;
    }
    return os << " }";
}

} // namespace minilua
