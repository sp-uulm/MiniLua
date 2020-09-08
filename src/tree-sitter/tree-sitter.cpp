#include "tree-sitter/tree-sitter.hpp"
#include <cstdio>
#include <iostream>
#include <optional>
#include <string>
#include <tree_sitter/api.h>

namespace ts {

// class Point
bool operator==(const Point& self, const Point& other) {
    return self.row == other.row && self.column == other.column;
}
std::ostream& operator<<(std::ostream& o, const Point& self) {
    return o << "Point{ .row = " << self.row << ", .column = " << self.column << "}";
}

// class Node
Node::Node(TSNode node) noexcept : node(node) {}

TSNode Node::get_raw() const noexcept { return this->node; }

bool Node::is_null() const noexcept { return ts_node_is_null(this->node); }

const char* Node::get_type() const { return ts_node_type(this->node); }

std::uint32_t Node::get_child_count() const { return ts_node_child_count(this->node); }

Node Node::get_child(std::uint32_t index) const { return Node(ts_node_child(this->node, index)); }

std::uint32_t Node::get_named_child_count() const { return ts_node_named_child_count(this->node); }

Node Node::get_named_child(std::uint32_t index) const {
    return Node(ts_node_named_child(this->node, index));
}

std::uint32_t Node::get_start_byte() const { return ts_node_start_byte(this->node); }
std::uint32_t Node::get_end_byte() const { return ts_node_end_byte(this->node); }

Point Node::get_start_point() const {
    TSPoint point = ts_node_start_point(this->node);
    return {.row = point.row, .column = point.column};
}
Point Node::get_end_point() const {
    TSPoint point = ts_node_end_point(this->node);
    return {.row = point.row, .column = point.column};
}

std::string Node::get_text(const std::string& source_code) const {
    auto start = this->get_start_byte();
    auto count = this->get_end_byte() - start;
    return source_code.substr(start, count);
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
Tree::Tree(TSTree* tree) : tree(tree, ts_tree_delete) {}

// move constructor
Tree::Tree(Tree&& other) noexcept : tree(std::exchange(other.tree, nullptr)) {}
// move assignment
Tree& Tree::operator=(Tree&& other) noexcept {
    swap(*this, other);
    return *this;
}
void swap(Tree& self, Tree& other) noexcept { std::swap(self.tree, other.tree); }

const TSTree* Tree::get_raw() const { return this->tree.get(); }

Node Tree::get_root_node() const noexcept { return Node(ts_tree_root_node(this->tree.get())); }

void Tree::print_dot_graph(std::string path) const {
    std::FILE* file = std::fopen(path.c_str(), "w");
    ts_tree_print_dot_graph(this->get_raw(), file);
    fclose(file);
}

// class Cursor
Cursor::Cursor(Node node) noexcept : cursor(ts_tree_cursor_new(node.get_raw())) {}
Cursor::Cursor(Tree& tree) noexcept : Cursor(tree.get_root_node()) {}
Cursor::~Cursor() noexcept { ts_tree_cursor_delete(&this->cursor); }
Cursor::Cursor(const Cursor& cursor) noexcept : cursor(ts_tree_cursor_copy(&cursor.cursor)) {}

Node Cursor::current_node() const noexcept {
    return Node(ts_tree_cursor_current_node(&this->cursor));
}

bool Cursor::goto_parent() noexcept { return ts_tree_cursor_goto_parent(&this->cursor); }
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

TSParser* Parser::get_raw() { return this->parser.get(); }

Tree Parser::parse_string(const TSTree* old_tree, std::string& str) {
    TSTree* tree = ts_parser_parse_string(this->get_raw(), old_tree, str.c_str(), str.length());
    if (tree == nullptr) {
        // This can occur when:
        // - there is no language set (should not happen because we manage that)
        // - or the timeout was reached (see ts_parser_set_timeout_micros)
        // - or the parsing was cancelled using ts_parser_set_cancellation_flag
        // In the latter two cases the parser can be restarted by calling it
        // with the same arguments.
        throw std::runtime_error("failed to parse");
    }
    return Tree(tree);
}
Tree Parser::parse_string(Tree old_tree, std::string& str) {
    return parse_string(old_tree.get_raw(), str);
}
Tree Parser::parse_string(std::string& str) { return parse_string(nullptr, str); }

} // namespace ts
