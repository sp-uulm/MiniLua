#include "tree-sitter/tree-sitter.hpp"
#include <cstdio>
#include <iostream>
#include <optional>
#include <string>
#include <tree_sitter/api.h>

namespace ts {

// struct Point
bool operator==(const Point& self, const Point& other) {
    return self.row == other.row && self.column == other.column;
}
std::ostream& operator<<(std::ostream& o, const Point& self) {
    return o << "Point{ .row = " << self.row << ", .column = " << self.column << "}";
}

// struct Location
bool operator==(const Location& self, const Location& other) {
    return self.point == other.point && self.byte == other.byte;
}
std::ostream& operator<<(std::ostream& o, const Location& self) {
    return o << "Location{ .point = " << self.point << ", .byte = " << self.byte << "}";
}

// struct Range
bool operator==(const Range& self, const Range& other) {
    return self.start == other.start && self.end == other.end;
}
std::ostream& operator<<(std::ostream& o, const Range& self) {
    return o << "Range{ .start = " << self.start << ", .end = " << self.end << "}";
}

// struct Edit
bool operator==(const Edit& self, const Edit& other) {
    return self.range == other.range && self.replacement == other.replacement;
}
std::ostream& operator<<(std::ostream& o, const Edit& self) {
    return o << "Edit{ .range = " << self.range << ", .replacement = " << self.replacement << "}";
}

// class Node
Node::Node(TSNode node, const Tree& tree) noexcept : node(node), tree_(tree) {}

TSNode Node::raw() const { return this->node; }
const Tree& Node::tree() const { return this->tree_; }

bool Node::is_null() const { return ts_node_is_null(this->node); }

const char* Node::type() const { return ts_node_type(this->node); }

std::uint32_t Node::child_count() const { return ts_node_child_count(this->node); }

Node Node::child(std::uint32_t index) const {
    return Node(ts_node_child(this->node, index), this->tree_);
}

std::uint32_t Node::named_child_count() const { return ts_node_named_child_count(this->node); }

Node Node::named_child(std::uint32_t index) const {
    return Node(ts_node_named_child(this->node, index), this->tree_);
}

std::uint32_t Node::start_byte() const { return ts_node_start_byte(this->node); }
std::uint32_t Node::end_byte() const { return ts_node_end_byte(this->node); }

Point Node::start_point() const {
    TSPoint point = ts_node_start_point(this->node);
    return {.row = point.row, .column = point.column};
}
Point Node::end_point() const {
    TSPoint point = ts_node_end_point(this->node);
    return {.row = point.row, .column = point.column};
}

Location Node::start() const {
    return Location{
        .point = this->start_point(),
        .byte = this->start_byte(),
    };
}
Location Node::end() const {
    return Location{
        .point = this->end_point(),
        .byte = this->end_byte(),
    };
}

Range Node::range() const {
    return Range{
        .start = this->start(),
        .end = this->end(),
    };
}

std::string Node::text() const {
    auto start = this->start_byte();
    auto count = this->end_byte() - start;
    return this->tree().source().substr(start, count);
}

std::string Node::as_string() const {
    char* raw_string = ts_node_string(this->node);
    if (raw_string == nullptr) {
        return std::string();
    } else {
        std::string str = std::string(raw_string);
        free(raw_string);
        return str;
    }
}

// class Tree
Tree::Tree(TSTree* tree, std::string& source, Parser& parser)
    : tree(tree, ts_tree_delete), source_(source), parser(parser) {}

// move constructor
Tree::Tree(Tree&& other) noexcept
    : tree(std::exchange(other.tree, nullptr)), source_(std::move(other.source_)),
      parser(other.parser) {}
// move assignment
Tree& Tree::operator=(Tree&& other) noexcept {
    swap(*this, other);
    return *this;
}
void swap(Tree& self, Tree& other) noexcept {
    using std::swap;
    swap(self.tree, other.tree);
    swap(self.source_, other.source_);
    swap(self.parser, other.parser);
}

const TSTree* Tree::raw() const { return this->tree.get(); }

const std::string& Tree::source() const { return this->source_; }

Node Tree::root_node() const { return Node(ts_tree_root_node(this->tree.get()), *this); }

void Tree::edit(const Edit& edit) {
    std::uint32_t old_size = edit.range.end.byte - edit.range.start.byte;

    this->source_.replace(edit.range.start.byte, old_size, edit.replacement);

    std::uint32_t size_diff = old_size - static_cast<std::uint32_t>(edit.replacement.size());

    TSInputEdit input_edit{
        .start_byte = edit.range.start.byte,
        .old_end_byte = edit.range.end.byte,
        .new_end_byte = edit.range.end.byte + size_diff,
        .start_point =
            TSPoint{
                .row = edit.range.start.point.row,
                .column = edit.range.start.point.column,
            },
        .old_end_point =
            TSPoint{
                .row = edit.range.end.point.row,
                .column = edit.range.end.point.column,
            },
        .new_end_point =
            TSPoint{
                .row = edit.range.end.point.row,
                .column = edit.range.end.point.column + size_diff,
            },
    };

    ts_tree_edit(this->tree.get(), &input_edit);
}

void Tree::sync() {
    TSTree* new_tree = ts_parser_parse_string(this->parser.raw(), this->tree.get(),
                                              this->source().c_str(), this->source().size());
    // delete old tree and insert new tree pointer
    this->tree.reset(new_tree);
}

void Tree::print_dot_graph(std::string path) const {
    std::FILE* file = std::fopen(path.c_str(), "w");
    ts_tree_print_dot_graph(this->raw(), file);
    fclose(file);
}

// class Cursor
Cursor::Cursor(Node node) noexcept : cursor(ts_tree_cursor_new(node.raw())), tree(node.tree()) {}
Cursor::Cursor(const Tree& tree) noexcept : Cursor(tree.root_node()) {}
Cursor::~Cursor() noexcept { ts_tree_cursor_delete(&this->cursor); }
Cursor::Cursor(const Cursor& cursor) noexcept
    : cursor(ts_tree_cursor_copy(&cursor.cursor)), tree(cursor.tree) {}

Node Cursor::current_node() const {
    return Node(ts_tree_cursor_current_node(&this->cursor), this->tree);
}

bool Cursor::goto_parent() { return ts_tree_cursor_goto_parent(&this->cursor); }
bool Cursor::goto_first_child() {
    do {
        if (!ts_tree_cursor_goto_first_child(&this->cursor)) {
            return false;
        }
    } while (!ts_node_is_named(ts_tree_cursor_current_node(&this->cursor)));
    return true;
}
bool Cursor::goto_next_sibling() {
    do {
        if (!ts_tree_cursor_goto_next_sibling(&this->cursor)) {
            return false;
        }
    } while (!ts_node_is_named(ts_tree_cursor_current_node(&this->cursor)));
    return true;
}

// class Parser
Parser::Parser() : parser(ts_parser_new(), ts_parser_delete) {
    if (!ts_parser_set_language(this->parser.get(), tree_sitter_lua())) {
        // only occurs when version of lua parser and tree-sitter library
        // are incompatible
        // see: ts_language_version, TREE_SITTER_LANGUAGE_VERSION,
        // TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION
        throw std::runtime_error("failed to set language on tree-sitter parser");
    }
}

// move constructor
Parser::Parser(Parser&& other) noexcept : parser(std::exchange(other.parser, nullptr)) {}
// move assignment
Parser& Parser::operator=(Parser&& other) noexcept {
    swap(*this, other);
    return *this;
}
void swap(Parser& self, Parser& other) noexcept { std::swap(self.parser, other.parser); }

TSParser* Parser::raw() { return this->parser.get(); }

Tree Parser::parse_string(const TSTree* old_tree, std::string& source) {
    TSTree* tree = ts_parser_parse_string(this->raw(), old_tree, source.c_str(), source.length());
    if (tree == nullptr) {
        // This can occur when:
        // - there is no language set (should not happen because we manage that)
        // - or the timeout was reached (see ts_parser_set_timeout_micros)
        // - or the parsing was cancelled using ts_parser_set_cancellation_flag
        // In the latter two cases the parser can be restarted by calling it
        // with the same arguments.
        throw std::runtime_error("failed to parse");
    }
    return Tree(tree, source, *this);
}
// Tree Parser::parse_string(Tree old_tree, std::string& str) {
//     return parse_string(old_tree.raw(), str);
// }
Tree Parser::parse_string(std::string& str) { return parse_string(nullptr, str); }

} // namespace ts
