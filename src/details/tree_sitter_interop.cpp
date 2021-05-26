#include "tree_sitter_interop.hpp"

namespace minilua {

auto to_ts_location(minilua::Location location) -> ts::Location {
    return ts::Location{
        .point =
            ts::Point{
                .row = location.line,
                .column = location.column,
            },
        .byte = location.byte,
    };
}

auto from_ts_location(ts::Location location) -> minilua::Location {
    return minilua::Location{
        .line = location.point.row,
        .column = location.point.column,
        .byte = location.byte,
    };
}

auto to_ts_range(minilua::Range range) -> ts::Range {
    return ts::Range{
        .start = to_ts_location(range.start),
        .end = to_ts_location(range.end),
    };
}

auto from_ts_range(ts::Range range) -> minilua::Range {
    return minilua::Range{
        .start = from_ts_location(range.start),
        .end = from_ts_location(range.end),
    };
}

auto to_ts_edit(minilua::SourceChange change) -> ts::Edit {
    return ts::Edit{
        .range = to_ts_range(change.range),
        .replacement = change.replacement,
    };
}

} // namespace minilua
