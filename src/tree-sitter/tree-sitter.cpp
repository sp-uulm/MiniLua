#include "tree-sitter/tree-sitter.hpp"
#include <optional>
#include <string>
#include <tree_sitter/api.h>

namespace ts {

// class Node
Node::Node(TSNode node) noexcept : node(node) {}

bool Node::is_null() const noexcept { return ts_node_is_null(this->node); }

const char* Node::get_type() const { return ts_node_type(this->node); }

Node Node::get_child(std::uint32_t index) const { return Node(ts_node_child(this->node, index)); }

std::uint32_t Node::get_named_child_count() const { return ts_node_named_child_count(this->node); }

Node Node::get_named_child(std::uint32_t index) const {
    return Node(ts_node_named_child(this->node, index));
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

const TSTree* Tree::get_raw() const noexcept { return this->tree.get(); }

Node Tree::get_root_node() const noexcept { return Node(ts_tree_root_node(this->tree.get())); }

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

Tree Parser::parse_string(const TSTree* old_tree, std::string& str) {
    TSTree* tree = ts_parser_parse_string(this->parser.get(), old_tree, str.c_str(), str.length());
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
