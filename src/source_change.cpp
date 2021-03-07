#include <utility>

#include "MiniLua/source_change.hpp"

namespace minilua {

// struct Location
auto operator<<(std::ostream& os, const Location& self) -> std::ostream& {
    return os << "Location{ line = " << self.line << ", column = " << self.column
              << ", byte = " << self.byte << " }";
}

// struct Range
auto Range::with_file(std::optional<std::shared_ptr<const std::string>> file) const -> Range {
    return Range{
        .start = this->start,
        .end = this->end,
        .file = std::move(file),
    };
}
auto operator==(const Range& lhs, const Range& rhs) noexcept -> bool {
    // check the internal string for equality instead of the shared_ptr
    bool file_equals = (!lhs.file.has_value() && !lhs.file.has_value()) ||
                       (lhs.file.has_value() && rhs.file.has_value() && **lhs.file == **rhs.file);
    return lhs.start == rhs.start && lhs.end == rhs.end && file_equals;
}
auto operator!=(const Range& lhs, const Range& rhs) noexcept -> bool { return !(lhs == rhs); }
auto operator<<(std::ostream& os, const Range& self) -> std::ostream& {
    os << "Range{ start = " << self.start << ", end = " << self.end << ", file = ";
    if (self.file) {
        os << "\"" << **self.file << "\"";
    } else {
        os << "nullopt";
    }
    return os << " }";
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

void SourceChangeTree::remove_filename() {
    this->visit_all([](SourceChange& change) { change.range.file = std::nullopt; });
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
    os << "SourceChangeTree{ change = ";
    self.visit([&os](const auto& change) { os << change; });
    return os << " }";
}

auto combine_source_changes(
    const std::optional<SourceChangeTree>& lhs, const std::optional<SourceChangeTree>& rhs)
    -> std::optional<SourceChangeTree> {
    if (lhs.has_value() && rhs.has_value()) {
        return SourceChangeCombination({*lhs, *rhs});
    } else if (lhs.has_value()) {
        return lhs;
    } else {
        return rhs;
    }
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
    return os << "SourceChange{ range = " << self.range << ", replacement = \"" << self.replacement
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
    os << "SourceChangeCombination { ";
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
    os << "SourceChangeAlternative { ";
    os << "origin = \"" << self.origin << "\", ";
    os << "hint = \"" << self.hint << "\"";
    for (const auto& change : self.changes) {
        os << ", " << change;
    }
    return os << " }";
}

} // namespace minilua
