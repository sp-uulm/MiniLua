#include <iterator>
#include <utility>

#include "MiniLua/source_change.hpp"

namespace minilua {

// struct Location
auto operator<<(std::ostream& os, const Location& self) -> std::ostream& {
    return os << "Location{" << self.line << ":" << self.column << "(" << self.byte << ")}";
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
    os << "Range{" << self.start.line << ":" << self.start.column << "(" << self.start.byte << ")"
       << "-" << self.end.line << ":" << self.end.column << "(" << self.end.byte << ")";
    if (self.file) {
        os << " \"" << **self.file << "\"";
    }
    return os << "}";
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

auto SourceChangeTree::simplify() const -> std::optional<SourceChangeTree> {
    return this->visit(
        [](const auto& change) -> std::optional<SourceChangeTree> { return change.simplify(); });
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
    os << "SourceChangeTree{";
    self.visit([&os](const auto& change) { os << change; });
    return os << " }";
}
auto operator<<(std::ostream& os, const std::optional<SourceChangeTree>& self) -> std::ostream& {
    if (self.has_value()) {
        return os << *self;
    } else {
        return os << "nullopt";
    }
}

auto simplify(const std::optional<SourceChangeTree>& tree) -> std::optional<SourceChangeTree> {
    if (tree.has_value()) {
        return tree->simplify();
    } else {
        return std::nullopt;
    }
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
    : range(std::move(range)), replacement(std::move(replacement)) {}

auto SourceChange::simplify() const -> SourceChange { return *this; }

auto operator==(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool {
    return lhs.range == rhs.range && lhs.replacement == rhs.replacement &&
           lhs.origin == rhs.origin && lhs.hint == rhs.hint;
}
auto operator!=(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const SourceChange& self) -> std::ostream& {
    return os << "SourceChange{" << self.range << " => " << self.replacement << ", origin = \""
              << self.origin << "\", hint = \"" << self.hint << "\"}";
}

// struct SCAnd
SourceChangeCombination::SourceChangeCombination() = default;
SourceChangeCombination::SourceChangeCombination(std::vector<SourceChangeTree> changes)
    : changes(std::move(changes)) {}

void SourceChangeCombination::add(SourceChangeTree change) { changes.push_back(std::move(change)); }

auto SourceChangeCombination::simplify() const -> std::optional<SourceChangeTree> {
    if (this->changes.empty()) {
        return std::nullopt;
    } else {
        std::vector<SourceChangeTree> changes;
        changes.reserve(this->changes.size());

        for (const auto& change : this->changes) {
            auto simplified = change.simplify();
            if (simplified.has_value()) {
                changes.push_back(simplified.value());
            }
        }

        if (changes.empty()) {
            return std::nullopt;
        } else if (changes.size() == 1) {
            auto change = changes[0];

            // set hints only if they are empty
            if (change.origin().empty()) {
                change.origin() = this->origin;
            }
            if (change.hint().empty()) {
                change.hint() = this->hint;
            }

            return change;
        } else {
            auto change = SourceChangeCombination(changes);
            change.origin = this->origin;
            change.hint = this->hint;
            return change;
        }
    }
}

auto operator==(const SourceChangeCombination& lhs, const SourceChangeCombination& rhs) noexcept
    -> bool {
    return lhs.changes == rhs.changes;
}
auto operator!=(const SourceChangeCombination& lhs, const SourceChangeCombination& rhs) noexcept
    -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const SourceChangeCombination& self) -> std::ostream& {
    os << "SourceChangeCombination{";
    os << "origin = \"" << self.origin << "\", ";
    os << "hint = \"" << self.hint << "\"";
    for (const auto& change : self.changes) {
        os << ", " << change;
    }
    return os << "}";
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

auto SourceChangeAlternative::simplify() const -> std::optional<SourceChangeTree> {
    if (this->changes.empty()) {
        return std::nullopt;
    } else {
        std::vector<SourceChangeTree> changes;
        changes.reserve(this->changes.size());

        for (const auto& change : this->changes) {
            auto simplified = change.simplify();
            if (simplified.has_value()) {
                changes.push_back(simplified.value());
            }
        }

        if (changes.empty()) {
            return std::nullopt;
        } else if (changes.size() == 1) {
            auto change = changes[0];

            // set hints only if they are empty
            if (change.origin().empty()) {
                change.origin() = this->origin;
            }
            if (change.hint().empty()) {
                change.hint() = this->hint;
            }

            return change;
        } else {
            auto change = SourceChangeAlternative(changes);
            change.origin = this->origin;
            change.hint = this->hint;
            return change;
        }
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
    os << "SourceChangeAlternative{";
    os << "origin = \"" << self.origin << "\", ";
    os << "hint = \"" << self.hint << "\"";
    for (const auto& change : self.changes) {
        os << ", " << change;
    }
    return os << "}";
}

} // namespace minilua

namespace std {
auto std::hash<minilua::Location>::operator()(const minilua::Location& location) const -> size_t {
    std::size_t h1 = std::hash<uint32_t>()(location.byte);
    std::size_t h2 = std::hash<uint32_t>()(location.column);
    std::size_t h3 = std::hash<uint32_t>()(location.line);
    std::size_t seed = h1;

    // implementation of boost::hash_combine
    seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);

    return seed;
}

auto std::hash<minilua::Range>::operator()(const minilua::Range& range) const -> size_t {
    std::size_t h1 = std::hash<minilua::Location>()(range.start);
    std::size_t h2 = std::hash<minilua::Location>()(range.end);
    // NOTE: ignores the file
    std::size_t seed = h1;

    // implementation of boost::hash_combine
    seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2); // NOLINT

    return seed;
}
} // namespace std
