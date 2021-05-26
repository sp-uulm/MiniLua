#ifndef MINILUA_DETAILS_TREE_SITTER_INTEROP_HPP
#define MINILUA_DETAILS_TREE_SITTER_INTEROP_HPP

#include <MiniLua/source_change.hpp>
#include <tree_sitter/tree_sitter.hpp>

namespace minilua {

auto to_ts_location(minilua::Location location) -> ts::Location;
auto from_ts_location(ts::Location location) -> minilua::Location;

auto to_ts_range(minilua::Range range) -> ts::Range;
auto from_ts_range(ts::Range range) -> minilua::Range;

auto to_ts_edit(minilua::SourceChange change) -> ts::Edit;

} // namespace minilua

#endif
