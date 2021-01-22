#ifndef TREE_SITTER_HPP
#define TREE_SITTER_HPP

#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <tree_sitter/api.h>
#include <utility>
#include <vector>

extern "C" const TSLanguage* tree_sitter_lua();

/**
 * Wrapper types and helper functions for Tree-Sitter.
 *
 * Some of the methods and types by default use the lua tree-sitter grammar but
 * there are always also functions that accept a language as parameter.
 */
namespace ts {

/**
 * Base exception class for errors in the Tree-Sitter wrapper.
 */
class TreeSitterException {};

/*+
 * Thrown in the constructor of 'Parser' if the version of Tree-Sitter and
 * the Language are not compatible.
 *
 * Check the version with 'TREE_SITTER_VERSION', 'TREE_SITTER_MIN_VERSION' and
 * 'Language::version()'.
 */
class ParserLanguageException : public TreeSitterException {
public:
    [[nodiscard]] const char* what() const noexcept;
};

/**
 * Thrown by 'Parser::parse_string'.
 *
 * This should never actually be thrown because we:
 *
 * - always set a language
 * - never set a timeout
 * - never set the cancellation flag
 */
class ParseFailureException : public TreeSitterException {
public:
    [[nodiscard]] const char* what() const noexcept;
};

/**
 * Thrown by the constructor of 'Node' if you try to create a null node.
 *
 * This should rarely be thrown.
 */
class NullNodeException : public TreeSitterException {
public:
    [[nodiscard]] const char* what() const noexcept;
};

/**
 * Thrown by the constructor of 'Query' if there is an error in the syntax of
 * the query string.
 *
 * Contains the raw error type from Tree-Sitter and the position of the error
 * in the query string.
 */
class QueryException : public TreeSitterException, public std::runtime_error {
    const TSQueryError error_;
    const std::uint32_t error_offset_;

public:
    QueryException(TSQueryError error, std::uint32_t error_offset);

    [[nodiscard]] TSQueryError query_error() const;
    [[nodiscard]] std::uint32_t error_offset() const;
};

/**
 * Base class for exceptions related to applying edits to the tree.
 *
 * Subclasses of this are thrown by 'Tree::edit'.
 */
class EditException : public TreeSitterException {};

/**
 * Thrown by 'Tree::edit' if any of the edits contain newlines.
 */
class MultilineEditException : public EditException, public std::runtime_error {
public:
    MultilineEditException();
};

/**
 * Thrown by 'Tree::edit' if any of the edits overlap.
 */
class OverlappingEditException : public EditException, public std::runtime_error {
public:
    OverlappingEditException();
};

/**
 * Thrown by 'Tree::edit' if any of the edits have size zero.
 */
class ZeroSizedEditException : public EditException, public std::runtime_error {
public:
    ZeroSizedEditException();
};

/**
 * Version for langauges created using the current tree-sitter version.
 *
 * Can be thought of as the max version for langauges.
 */
const std::size_t TREE_SITTER_VERSION = TREE_SITTER_LANGUAGE_VERSION;

/**
 * Minimum supported version of languages.
 */
const std::size_t TREE_SITTER_MIN_VERSION = TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION;

/**
 * Numeric representation of the type of a node.
 */
using TypeId = TSSymbol;

/**
 * Numeric representation of a field.
 */
using FieldId = TSFieldId;

/**
 * Kind of a 'TypeId'.
 *
 * Analogous to 'TSSymbolType'.
 */
enum class TypeKind {
    Named,
    Anonymous,
    Hidden,
};

/**
 * Represents a location in source code as row and column.
 */
struct Point {
    std::uint32_t row;
    std::uint32_t column;

    std::string pretty(bool start_at_one = false) const;
};

bool operator==(const Point&, const Point&);
bool operator!=(const Point&, const Point&);
bool operator<(const Point&, const Point&);
bool operator<=(const Point&, const Point&);
bool operator>(const Point&, const Point&);
bool operator>=(const Point&, const Point&);
std::ostream& operator<<(std::ostream&, const Point&);

/**
 * Represents a location in source code as both a point (row and column)
 * and a byte offset.
 *
 * Locations can be <, <=, >, >= and ==, != compared. But you should only compare
 * locations created from the same source.
 */
struct Location {
    Point point;
    std::uint32_t byte;
};

bool operator==(const Location&, const Location&);
bool operator!=(const Location&, const Location&);
bool operator<(const Location&, const Location&);
bool operator<=(const Location&, const Location&);
bool operator>(const Location&, const Location&);
bool operator>=(const Location&, const Location&);
std::ostream& operator<<(std::ostream&, const Location&);

/**
 * Represents a range (start and end) in source code.
 */
struct Range {
    Location start;
    Location end;

    bool overlaps(const Range&) const;
};

bool operator==(const Range&, const Range&);
bool operator!=(const Range&, const Range&);
std::ostream& operator<<(std::ostream&, const Range&);
std::ostream& operator<<(std::ostream&, const std::vector<Range>&);

/**
 * Represents an edit of source code.
 *
 * Contains the range that should be replaced and the string it should be
 * replaced with.
 *
 * Note that the range and replacement string don't need to have the same size.
 */
struct Edit {
    Range range;
    std::string replacement;
};

bool operator==(const Edit&, const Edit&);
bool operator!=(const Edit&, const Edit&);
std::ostream& operator<<(std::ostream&, const Edit&);
std::ostream& operator<<(std::ostream&, const std::vector<Edit>&);

/**
 * Represents a language.
 *
 * This can be inspected (e.g. the nodes it can produce) and used for parsing.
 */
class Language {
    const TSLanguage* lang;

public:
    Language(const TSLanguage*) noexcept;

    /**
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * WARNING: Never free or otherwise delete this pointer.
     */
    [[nodiscard]] const TSLanguage* raw() const;

    /**
     * Get the number of distinct node types in the language.
     */
    [[nodiscard]] std::uint32_t node_type_count() const;

    /**
     * Get the node type string for the given numberic id.
     */
    [[nodiscard]] const char* node_type_name(TypeId) const;

    /**
     * Get the numeric id for the given node type string.
     */
    [[nodiscard]] TypeId node_type_id(std::string_view, bool is_named) const;

    /**
     * Get the number of distrinct field names in the langauge.
     */
    [[nodiscard]] std::uint32_t field_count() const;

    /**
     * Get the field name string for the given numeric id.
     */
    [[nodiscard]] const char* field_name(FieldId) const;

    /**
     * Getht the numeric id for the given field name string.
     */
    [[nodiscard]] FieldId field_id(std::string_view) const;

    /**
     * Get the kind of a node type id.
     */
    [[nodiscard]] TypeKind node_type_kind(TypeId) const;

    /**
     * Get the ABI version number for this langauge.
     *
     * Used to check if language was generated by a compatible version of
     * Tree-Sitter.
     */
    [[nodiscard]] std::uint32_t version() const;
};

/**
 * Check if a language is compatible with the linkes tree-sitter version.
 */
bool language_compatible(const Language&);

/**
 * Global lua language and node type constants.
 */
const Language LUA_LANGUAGE = Language(tree_sitter_lua());

// TODO we should really generate this automatically
// TODO how to deal with unnamed (e.g. operators "+", etc. are still useful)
const TypeId NODE_BREAK_STATEMENT = LUA_LANGUAGE.node_type_id("break_statement", true);
const TypeId NODE_SPREAD = LUA_LANGUAGE.node_type_id("spread", true);
const TypeId NODE_SELF = LUA_LANGUAGE.node_type_id("self", true);
const TypeId NODE_NEXT = LUA_LANGUAGE.node_type_id("next", true);
const TypeId NODE_NUMBER = LUA_LANGUAGE.node_type_id("number", true);
const TypeId NODE_NIL = LUA_LANGUAGE.node_type_id("nil", true);
const TypeId NODE_TRUE = LUA_LANGUAGE.node_type_id("true", true);
const TypeId NODE_FALSE = LUA_LANGUAGE.node_type_id("false", true);
const TypeId NODE_IDENTIFIER = LUA_LANGUAGE.node_type_id("identifier", true);
const TypeId NODE_COMMENT = LUA_LANGUAGE.node_type_id("comment", true);
const TypeId NODE_STRING = LUA_LANGUAGE.node_type_id("string", true);
const TypeId NODE_PROGRAM = LUA_LANGUAGE.node_type_id("program", true);
const TypeId NODE_RETURN_STATEMENT = LUA_LANGUAGE.node_type_id("return_statement", true);
const TypeId NODE_VARIABLE_DECLARATION = LUA_LANGUAGE.node_type_id("variable_declaration", true);
const TypeId NODE_LOCAL_VARIABLE_DECLARATION =
    LUA_LANGUAGE.node_type_id("local_variable_declaration", true);
const TypeId NODE_FIELD_EXPRESSION = LUA_LANGUAGE.node_type_id("field_expression", true);
const TypeId NODE_TABLE_INDEX = LUA_LANGUAGE.node_type_id("table_index", true);
const TypeId NODE_VARIABLE_DECLARATOR = LUA_LANGUAGE.node_type_id("variable_declarator", true);
const TypeId NODE_LOCAL_VARIABLE_DECLARATOR = LUA_LANGUAGE.node_type_id("local_variable_declarator", true);
const TypeId NODE_DO_STATEMENT = LUA_LANGUAGE.node_type_id("do_statement", true);
const TypeId NODE_IF_STATEMENT = LUA_LANGUAGE.node_type_id("if_statement", true);
const TypeId NODE_ELSEIF = LUA_LANGUAGE.node_type_id("elseif", true);
const TypeId NODE_ELSE = LUA_LANGUAGE.node_type_id("else", true);
const TypeId NODE_WHILE_STATEMENT = LUA_LANGUAGE.node_type_id("while_statement", true);
const TypeId NODE_REPEAT_STATEMENT = LUA_LANGUAGE.node_type_id("repeat_statement", true);
const TypeId NODE_FOR_STATEMENT = LUA_LANGUAGE.node_type_id("for_statement", true);
const TypeId NODE_FOR_IN_STATEMENT = LUA_LANGUAGE.node_type_id("for_in_statement", true);
const TypeId NODE_LOOP_EXPRESSION = LUA_LANGUAGE.node_type_id("loop_expression", true);
const TypeId NODE_GOTO_STATEMENT = LUA_LANGUAGE.node_type_id("goto_statement", true);
const TypeId NODE_LABEL_STATEMENT = LUA_LANGUAGE.node_type_id("label_statement", true);
const TypeId NODE_FUNCTION = LUA_LANGUAGE.node_type_id("function", true);
const TypeId NODE_LOCAL_FUNCTION = LUA_LANGUAGE.node_type_id("local_function", true);
const TypeId NODE_FUNCTION_CALL = LUA_LANGUAGE.node_type_id("function_call", true);
const TypeId NODE_ARGUMENTS = LUA_LANGUAGE.node_type_id("ARGUMENTS", true);
const TypeId NODE_FUNCTION_NAME = LUA_LANGUAGE.node_type_id("function_name", true);
const TypeId NODE_FUNCTION_NAME_FIELD = LUA_LANGUAGE.node_type_id("function_name_field", true);
const TypeId NODE_PARAMETERS = LUA_LANGUAGE.node_type_id("parameters", true);
const TypeId NODE_GLOBAL_VARIABLE = LUA_LANGUAGE.node_type_id("global_variable", true);
const TypeId NODE_FUNCTION_DEFINITION = LUA_LANGUAGE.node_type_id("function_definition", true);
const TypeId NODE_TABLE = LUA_LANGUAGE.node_type_id("table", true);
const TypeId NODE_FIELD = LUA_LANGUAGE.node_type_id("field", true);
const TypeId NODE_BINARY_OPERATION = LUA_LANGUAGE.node_type_id("binary_operation", true);
const TypeId NODE_UNARY_OPERATION = LUA_LANGUAGE.node_type_id("unary_operation", true);
const TypeId NODE_CONDITION_EXPRESSION = LUA_LANGUAGE.node_type_id("condition_expression", true);
const TypeId NODE_EXPRESSION = LUA_LANGUAGE.node_type_id("expression", true);
const TypeId NODE_METHOD = LUA_LANGUAGE.node_type_id("method", true);
const TypeId NODE_PROPERTY_IDENTIFIER = LUA_LANGUAGE.node_type_id("property_identifier", true);

const FieldId FIELD_OBJECT = LUA_LANGUAGE.field_id("object");

// forward declarations
class Cursor;
class Tree;

/**
 * Wrapper for a 'TSNode'.
 *
 * Nodes can be named or anonymous (see [Named vs Anonymous
 * Nodes](https://tree-sitter.github.io/tree-sitter/using-parsers#named-vs-anonymous-nodes)).
 * We are mostly interested in named nodes.
 *
 * Nodes can't be null. If you try to create a null node the constructor will
 * throw a 'NullNodeException'. (But this should only be used internally.)
 *
 * Note: This object is only valid for as long as the 'Tree' it was created from.
 * If the tree was edited methods on the node might return wrong results. In this
 * case you should retrieve the node from the tree again.
 *
 * 'type_id' is called *symbol* in Tree-Sitter.
 *
 * Features not included (because we currently don't use them):
 *
 * - Get child by filed:
 *   - `ts_node_child_by_field_name`
 *   - `ts_node_child_by_field_id`
 * - Get child/decendant for byte/point (range):
 *   - `ts_node_first_child_for_byte`
 *   - `ts_node_first_named_child_for_byte`
 *   - `ts_node_descendant_for_byte_range`
 *   - `ts_node_descendant_for_point_range`
 *   - `ts_node_named_descendant_for_byte_range`
 *   - `ts_node_named_descendant_for_point_range`
 * - Editing nodes directly:
 *   - `ts_node_edit`
 */
class Node {
    TSNode node;
    // not owned pointer
    const Tree* tree_;

public:
    struct unsafe_t {};
    // used as a token for the unsafe constructor
    constexpr static unsafe_t unsafe{};
    /**
     * Creates a new node from the given tree-sitter node and the tree.
     *
     * Will throw 'NullNodeException' if the node is null.
     *
     * NOTE: Should only be used internally.
     */
    Node(TSNode node, const Tree& tree);

    /**
     * Unsafe constructor that does not check for null nodes.
     *
     * Only call this if you know the node is not null.
     * This might make other node methods segfault if the node was null.
     */
    Node(unsafe_t, TSNode node, const Tree& tree) noexcept;

    /**
     * Return a Node or `std::nullopt` if the node is null.
     */
    static std::optional<Node> or_null(TSNode node, const Tree& tree) noexcept;

    /**
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * WARNING: Don't modify this by calling.
     */
    [[nodiscard]] TSNode raw() const;

    /**
     * Get the tree this node was created from.
     */
    [[nodiscard]] const Tree& tree() const;

    /**
     * Get the string representation of the type of the node.
     */
    [[nodiscard]] const char* type() const;

    /**
     * Get the numeric representation of the type of the node.
     *
     * In tree-sitter this is called *symbol*.
     *
     * NOTE: There is currently no real way to use it because there are no
     * constants available to compare to.
     */
    [[nodiscard]] TypeId type_id() const;

    /**
     * Check if the node is named.
     */
    [[nodiscard]] bool is_named() const;

    /**
     * Check if the node is *missing*.
     *
     * Missing nodes are used to recover from some kinds of syntax errors.
     */
    [[nodiscard]] bool is_missing() const;

    /**
     * Check if the node is *extra*.
     *
     * Extra nodes represent things like comments.
     */
    [[nodiscard]] bool is_extra() const;

    /**
     * Check if the node has been edited.
     */
    [[nodiscard]] bool has_changes() const;

    /**
     * Get true if the node is a syntax error or contains any syntax errors.
     */
    [[nodiscard]] bool has_error() const;

    /**
     * Gets the nodes parent.
     *
     * Can return a null node if the current node is the root node of a tree.
     */
    [[nodiscard]] std::optional<Node> parent() const;

    /**
     * Get the n-th child (0 indexed).
     *
     * This will also return anonymous nodes.
     *
     * Can return a null node.
     */
    [[nodiscard]] std::optional<Node> child(std::uint32_t index) const;

    /**
     * Get the count of all children.
     */
    [[nodiscard]] std::uint32_t child_count() const;

    /**
     * Get a list of all children.
     */
    [[nodiscard]] std::vector<Node> children() const;

    /**
     * Get the n-th named child (0 indexed).
     *
     * This will not return anonymous nodes and the index only considers named
     * nodes.
     *
     * Can return a null node.
     */
    [[nodiscard]] std::optional<Node> named_child(std::uint32_t index) const;

    /**
     * Get the count of named children.
     */
    [[nodiscard]] std::uint32_t named_child_count() const;

    /**
     * Get a list of all named children.
     */
    [[nodiscard]] std::vector<Node> named_children() const;

    /**
     * Get the node's next sibling.
     *
     * This will also return anonymous nodes.
     *
     * Can return a null node.
     */
    [[nodiscard]] std::optional<Node> next_sibling() const;

    /**
     * Get the node's previous sibling.
     *
     * This will also return anonymous nodes.
     *
     * Can return a null node.
     */
    [[nodiscard]] std::optional<Node> prev_sibling() const;

    /**
     * Get the node's next *named* sibling.
     *
     * This will not return anonymous nodes.
     *
     * Can return a null node.
     */
    [[nodiscard]] std::optional<Node> next_named_sibling() const;

    /**
     * Get the node's previous *named* sibling.
     *
     * This will not return anonymous nodes.
     *
     * Can return a null node.
     */
    [[nodiscard]] std::optional<Node> prev_named_sibling() const;

    /**
     * Get the start position as a byte offset.
     */
    [[nodiscard]] std::uint32_t start_byte() const;

    /**
     * Get the end position as a byte offset.
     *
     * Returns the position after the last character.
     */
    [[nodiscard]] std::uint32_t end_byte() const;

    /**
     * Get the start position as a 'Point' (row + column).
     */
    [[nodiscard]] Point start_point() const;

    /**
     * Get the end position as a 'Point' (row + column).
     */
    [[nodiscard]] Point end_point() const;

    /**
     * Get the start position as a 'Location' ('Point' + byte).
     */
    [[nodiscard]] Location start() const;

    /**
     * Get the end position as a 'Location' ('Point' + byte).
     */
    [[nodiscard]] Location end() const;

    /**
     * Get the 'Range' of the node (start and end location).
     */
    [[nodiscard]] Range range() const;

    /**
     * Get the original string this node represents.
     */
    [[nodiscard]] std::string text() const;

    /**
     * Returns the syntax tree starting from node represented as an s-expression.
     */
    [[nodiscard]] std::string as_s_expr() const;
};

bool operator==(const Node& lhs, const Node& rhs);
bool operator!=(const Node& lhs, const Node& rhs);
std::ostream& operator<<(std::ostream&, const Node&);
std::ostream& operator<<(std::ostream&, const std::optional<Node>&);

/**
 * Parser for the Lua language.
 *
 * Features not included (because we currently don't use them):
 *
 * - Partial document parsing:
 *   - `ts_parser_set_included_ranges`
 *   - `ts_parser_included_ranges`
 * - alternative parse sources (other than utf8 string)
 *   - generalized `ts_parser_parse` (with TSInput)
 *   - `ts_parser_parse_string_encoding`
 * - parsing timeout/cancellation
 *   - `ts_parser_reset`
 *   - `ts_parser_set_timeout_micros`
 *   - `ts_parser_timeout_micros`
 *   - `ts_parser_set_cancellation_flag`
 *   - `ts_parser_cancellation_flag`
 * - Grammar debug features:
 *   - `ts_parser_set_logger`
 *   - `ts_parser_logger`
 *   - `ts_parser_print_dot_graphs`
 */
class Parser {
    std::unique_ptr<TSParser, void (*)(TSParser*)> parser;

public:
    /**
     * Create a parser using the lua language.
     */
    Parser();

    /**
     * Create a parser using the given language.
     */
    Parser(const Language&);

    /**
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * WARNING: Never free or otherwise delete this pointer.
     */
    [[nodiscard]] TSParser* raw() const;

    /**
     * Get the parser language.
     */
    [[nodiscard]] Language language() const;

    /**
     * Parse a string and return a syntax tree.
     *
     * This takes the source code by copy (or move) and stores it in the tree.
     */
    Tree parse_string(std::string) const;

    Tree parse_string(const TSTree* old_tree, std::string source) const;
};

/**
 * Hold information about an applied edit.
 *
 * - 'before' is the range in the old source code string
 * - 'after' is the range in the new source code string
 * - 'old_source' is the string in the source code that was replaced
 * - 'replacement' is the string it was replaced with
 *
 * 'after' could for example be used to highlight changed locations in an editor.
 */
struct AppliedEdit {
    Range before;
    Range after;
    std::string old_source;
    std::string replacement;
};

bool operator==(const AppliedEdit&, const AppliedEdit&);
bool operator!=(const AppliedEdit&, const AppliedEdit&);
std::ostream& operator<<(std::ostream&, const AppliedEdit&);
std::ostream& operator<<(std::ostream&, const std::vector<AppliedEdit>&);

/**
 * Holds information about all applied edits. Returned by 'Tree::edit'.
 *
 * - 'changed_ranges' are the raw ranges of string that were changed (this does
 *   not directly correspond to the edits)
 * - 'applied_edits' the adjusted and applied edits (holds more information
 *   about the actually applied edits, including adjusted locations)
 */
struct EditResult {
    std::vector<Range> changed_ranges;
    std::vector<AppliedEdit> applied_edits;
};

bool operator==(const EditResult&, const EditResult&);
bool operator!=(const EditResult&, const EditResult&);
std::ostream& operator<<(std::ostream&, const EditResult&);

/**
 * Represents a syntax tree.
 *
 * This also contains a copy of the source code to allow the nodes to refer to
 * the text they were created from.
 */
class Tree {
    std::unique_ptr<TSTree, void (*)(TSTree*)> tree;
    // maybe a separate Input type is better to be more flexible
    std::string source_;

    // not owned pointer
    const Parser* parser_;

public:
    // explicit because this class handles the pointer as a ressource
    // automatic conversion from pointer to this type is dangerous
    explicit Tree(TSTree* tree, std::string source, const Parser& parser);

    // TSTree* can be safely (and fast) copied using ts_tree_copy
    Tree(const Tree&);
    Tree& operator=(const Tree&);

    // move constructor
    Tree(Tree&& other) noexcept = default;
    // move assignment
    Tree& operator=(Tree&& other) noexcept = default;
    friend void swap(Tree& self, Tree& other) noexcept;

    ~Tree() = default;

    /**
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * WARNING: Never free or otherwise delete this pointer.
     */
    [[nodiscard]] const TSTree* raw() const;

    /**
     * Get a reference to the source code.
     */
    [[nodiscard]] const std::string& source() const;

    /**
     * Get a reference to the used parser.
     */
    [[nodiscard]] const Parser& parser() const;

    /**
     * The returned node is only valid as long as this tree is not destructed.
     */
    [[nodiscard]] Node root_node() const;

    /**
     * Get the language that was used to parse the syntax tree.
     */
    [[nodiscard]] Language language() const;

    /**
     * Edit the syntax tree and source code and return the changed ranges.
     *
     * You need to specify all edits you want to apply to the syntax tree in one
     * call. Because this method changes both the syntax tree and source code
     * string any other 'Edit's will be invalid and trying to apply them is
     * undefined behaviour.
     *
     * The edits can't be duplicate or overlapping. Multiline edits are also
     * currently not supported.
     *
     * The returned result contains information about the raw string ranges
     * that changed and it also contains the adjusted location of the edits that
     * can e.g. be used for highlighting in an editor.
     *
     * Any previously retrieved nodes will become (silently) invalid.
     *
     * NOTE: This takes the edits by value because they should not be used after
     * calling this function and we need to modify the vector internally.
     */
    EditResult edit(std::vector<Edit>);

    /**
     * Print a dot graph to the given file.
     *
     * 'file' has to be a null-terminated string (e.g. from a std::string).
     */
    void print_dot_graph(std::string_view file) const;
};

EditResult edit_tree(std::vector<Edit> edits, Tree& tree, TSTree* old_tree);

/**
 * Allows more efficient walking of a 'Tree' than with the methods on 'Node'.
 *
 * Features not included (because we currently don't use them):
 *
 * - `ts_tree_cursor_goto_first_child_for_byte`
 */
class Cursor {
    // TSTreeCursor internally allocates heap.
    // It can be copied with "ts_tree_cursor_copy"
    TSTreeCursor cursor;
    const Tree* tree;

public:
    /**
     * Create a cursor starting at the given node.
     */
    explicit Cursor(Node) noexcept;

    /**
     * Create a cursor starting at the root node of the given tree.
     */
    explicit Cursor(const Tree&) noexcept;
    ~Cursor() noexcept;

    // copy constructor
    Cursor(const Cursor&) noexcept;
    // copy assignment
    Cursor& operator=(const Cursor&) noexcept;

    Cursor(Cursor&&) = default;
    Cursor& operator=(Cursor&&) = default;

    friend void swap(Cursor&, Cursor&) noexcept;

    /**
     * Reset the cursor to the given node.
     */
    void reset(Node);

    /**
     * Reset the cursor to the root node of the given tree.
     */
    void reset(const Tree&);

    /**
     * Get the node the cursor is currently pointing at.
     */
    [[nodiscard]] Node current_node() const;

    /**
     * Get the field name of the node the cursor is currently pointing at.
     */
    [[nodiscard]] const char* current_field_name() const;

    /**
     * Get the field id of the node the cursor is currently pointing at.
     */
    [[nodiscard]] FieldId current_field_id() const;

    /**
     * Move the cursor to the parent of the current node.
     *
     * Returns only false if the cursor is already at the root node.
     */
    bool goto_parent();

    /**
     * Move the cursor to the next sibling of the current node.
     *
     * Returns false if there was no next sibling.
     */
    bool goto_next_sibling();

    /**
     * Similar to calling 'goto_next_sibling' n times.
     *
     * Returns the number of siblings skipped.
     */
    int skip_n_siblings(int n);

    /**
     * Move the cursor to the first child of the current node.
     *
     * Returns false if there were no children.
     */
    bool goto_first_child();

    /**
     * Move the cursor to the next named sibling of the current node.
     *
     * Returns false if there was no next sibling.
     *
     * NOTE: This method might move the cursor to another unnamed node and then
     * still return false if there is no named node.
     */
    bool goto_next_named_sibling();

    /**
     * Move the cursor to the next named sibling of the current node.
     *
     * Returns false if there was no next sibling.
     *
     * NOTE: This method might move the cursor to another unnamed node and then
     * still return false if there is no named node.
     */
    bool goto_first_named_child();

    /**
     * Skips over nodes while the given callback returns true.
     *
     * The method returns false if there were no more siblings to skip while the
     * callback still returned true.
     */
    template <typename Fn> bool skip_siblings_while(Fn fn) {
        if (!this->goto_next_sibling()) {
            return false;
        }
        while (fn(this->current_node())) {
            if (!this->goto_next_sibling()) {
                return false;
            }
        }

        return true;
    }

    /**
     * Calls the provided callback for every sibling and moves the cursor.
     *
     * The callback will also be called on the current node. So it will always
     * be called at least once.
     */
    template <typename Fn> void foreach_remaining_siblings(Fn fn) {
        do {
            fn(this->current_node());
        } while (this->goto_next_sibling());
    }

    /**
     * Returns a list of all child nodes of the current node.
     *
     * This will also move the cursor to the last child but you can 'reset' the
     * cursor to point at any of the returned children or call 'parent' to get
     * back to the current node.
     */
    std::vector<Node> children();

    /**
     * Returns a list of all named child nodes of the current node.
     *
     * This will also move the cursor to the last child but you can 'reset' the
     * cursor to point at any of the returned children or call 'parent' to get
     * back to the current node.
     */
    std::vector<Node> named_children();
};

/**
 * A query is a "pre-compiled" string of S-expression patterns.
 *
 * Features not included (because we currently don't use them):
 *
 * - Predicates (evaluation is not handled by tree-sitter anyway):
 *   - `ts_query_predicates_for_pattern`
 *   - `ts_query_step_is_definite`
 */
class Query {
    std::unique_ptr<TSQuery, void (*)(TSQuery*)> query;

public:
    /**
     * Create query from the given query string.
     *
     * NOTE: The string_view does not need to be null terminated.
     */
    Query(std::string_view);

    // no copying because TSQuery can't be copied
    Query(const Query&) = delete;
    Query& operator=(const Query&) = delete;

    // move constructor
    Query(Query&&) noexcept = default;
    // move assignment
    Query& operator=(Query&&) noexcept = default;
    friend void swap(Query& self, Query& other) noexcept;

    ~Query() = default;

    /**
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * WARNING: Never free or otherwise delete this pointer.
     */
    [[nodiscard]] const TSQuery* raw() const;

    /**
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * WARNING: Never free or otherwise delete this pointer.
     */
    [[nodiscard]] TSQuery* raw();

    /**
     * Get the number of patterns in the query.
     */
    [[nodiscard]] std::uint32_t pattern_count() const;

    /**
     * Get the number of captures in the query.
     */
    [[nodiscard]] std::uint32_t capture_count() const;

    /**
     * Get the number of string literals in the query.
     */
    [[nodiscard]] std::uint32_t string_count() const;

    /**
     * Get the byte offset where the pattern starts in the query'y source.
     *
     * Can be useful when combining queries.
     */
    [[nodiscard]] std::uint32_t start_byte_for_pattern(std::uint32_t) const;

    /**
     * Get the name of one of the query's captures.
     *
     * Each capture is associated with a numeric id based on the order
     * that it appeared in the query's source.
     *
     * NOTE: The returned string_view is only valid as long the query is.
     */
    [[nodiscard]] std::string_view capture_name_for_id(std::uint32_t id) const;

    /**
     * Get one of the query's string literals.
     *
     * Each string literal is associated with a numeric id based on the order
     * that it appeared in the query's source.
     *
     * NOTE: The returned string_view is only valid as long the query is.
     */
    [[nodiscard]] std::string_view string_value_for_id(std::uint32_t id) const;

    /**
     * Disable a capture within a query.
     *
     * This prevents the capture from being returned in matches and avoid
     * ressource usage.
     *
     * This can not be undone.
     */
    void disable_capture(std::string_view);

    /**
     * Disable a pattern within a query.
     *
     * This prevents the pattern from matching and removes most of the overhead.
     *
     * This can not be undone.
     */
    void disable_pattern(std::uint32_t id);
};

/**
 * Represents a capture of a node in a syntax tree.
 */
struct Capture {
    Node node;
    std::uint32_t index;

    Capture(TSQueryCapture, const Tree&) noexcept;
};

std::ostream& operator<<(std::ostream&, const Capture&);
std::ostream& operator<<(std::ostream& os, const std::vector<Capture>&);

/**
 * Represents a match of a pattern in a syntax tree.
 */
struct Match {
    uint32_t id;
    uint16_t pattern_index;
    std::vector<Capture> captures;

    Match(TSQueryMatch, const Tree&) noexcept;

    /**
     * Returns the first capture with the given index if any.
     *
     * Note: This does a linear search for a capture with the given index.
     */
    [[nodiscard]] std::optional<Capture> capture_with_index(std::uint32_t index) const;
};

std::ostream& operator<<(std::ostream&, const Match&);
std::ostream& operator<<(std::ostream& os, const std::vector<Match>&);

/**
 * Stores the state needed to execute a query and iteratively search for matches.
 *
 * You first have to call 'exec' with the query and then you can retrieve matches
 * with the other functions.
 *
 * You can iterate over the result matches by calling 'next_match'. This is only
 * useful if you provided multiple patterns.
 *
 * You can also iterate over the captures if you don't care which patterns matched.
 *
 * At any point you can call 'exec' again and start using the cursor with another
 * query.
 *
 * Features not include (because we currently don't use them):
 *
 * - setting byte/point range to search in:
 *   - `ts_query_cursor_set_byte_range`
 *   - `ts_query_cursor_set_point_range`
 */
class QueryCursor {
    std::unique_ptr<TSQueryCursor, void (*)(TSQueryCursor*)> cursor;
    const Tree* tree;

public:
    explicit QueryCursor(const Tree&) noexcept;

    // can't copy because TSQueryCursor can't be copied
    QueryCursor(const QueryCursor&) = delete;
    QueryCursor& operator=(const QueryCursor&) = delete;

    QueryCursor(QueryCursor&&) = default;
    QueryCursor& operator=(QueryCursor&&) = default;

    ~QueryCursor() = default;

    /**
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * WARNING: Never free or otherwise delete this pointer.
     */
    [[nodiscard]] const TSQueryCursor* raw() const;

    /**
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * WARNING: Never free or otherwise delete this pointer.
     */
    [[nodiscard]] TSQueryCursor* raw();

    /**
     * Start running a given query on a given node.
     */
    void exec(const Query&, Node);

    /**
     * Start running a given query on the root of the tree.
     */
    void exec(const Query&);

    /**
     * Advance to the next match of the currently running query if possible.
     */
    std::optional<Match> next_match();

    // void ts_query_cursor_remove_match(TSQueryCursor *, uint32_t id);

    /**
     * Advance to the next capture of the currently running query if possible.
     */
    std::optional<Capture> next_capture();

    /**
     * Get all matches.
     *
     * This needs to internally advance over the matches so you can only call
     * this once. Subsequent calls will return an empty vector.
     *
     * This will also omit matches that were already retrieved by calling
     * 'next_match'.
     */
    std::vector<Match> matches();
};

/**
 * Prints a debug representation of the node (and all child nodes).
 *
 * This is easier to read than 'Node::as_s_expr' and contains more information.
 *
 * Additional node properties are indicated by a symbol after the node name:
 *
 * - has_changes: *
 * - has_errors: E
 * - is_names: N
 * - is_missing: ?
 * - is_extra: +
 */
std::string debug_print_tree(Node node);
std::string debug_print_node(Node node);

} // namespace ts

#endif
