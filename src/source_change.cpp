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
auto operator==(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool {
    return lhs.range == rhs.range && lhs.replacement == rhs.replacement;
}
auto operator!=(const SourceChange& lhs, const SourceChange& rhs) noexcept -> bool {
    return !(lhs == rhs);
}
auto operator<<(std::ostream& os, const SourceChange& self) -> std::ostream& {
    return os << "SourceChange{ range = " << self.range << ", replacement = \"" << self.replacement
              << "\" }";
}

} // namespace minilua
