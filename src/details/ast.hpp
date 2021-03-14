#ifndef MINILUA_AST_HPP
#define MINILUA_AST_HPP

#include "../tree_sitter_lua.hpp"
#include "MiniLua/values.hpp"
#include <tree_sitter/tree_sitter.hpp>
#include <variant>

namespace minilua::details::ast {

// Some forward declarations
class Body;
class Expression;
class Identifier;
class Statement;
class Prefix;
class Return;
class FieldExpression;

using IndexField = std::pair<Expression, Expression>;
using IdentifierField = std::pair<Identifier, Expression>;

enum GEN_CAUSE { FOR_LOOP_DESUGAR, FOR_IN_LOOP_DESUGAR, FUNCTION_STATEMENT_DESUGAR, PLACEHOLDER };
enum class LiteralType { TRUE, FALSE, NIL, NUMBER, STRING };
class Literal {
    std::string literal_content;
    LiteralType literal_type;
    minilua::Range literal_range;

public:
    Literal(LiteralType, std::string, minilua::Range);
    auto content() const -> std::string;
    auto type() const -> LiteralType;
    auto range() const -> minilua::Range;
};
/**
 * class for program nodes. It only holds a body
 */
class Program {
    ts::Node program;

public:
    explicit Program(ts::Node);
    /**
     * the children nodes of the Program get put into a Body class by this method
     * @return a Body containing the full program
     */
    auto body() const -> Body;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for do_statement nodes
 */
class DoStatement {
    using DoTuple = std::tuple<std::shared_ptr<Body>, minilua::Range>;
    enum TupleIndex { BODY, RANGE };
    struct DoStruct {
        std::shared_ptr<Body> body;
        minilua::Range range;
        GEN_CAUSE cause;
    };
    std::variant<ts::Node, DoTuple> content;

public:
    explicit DoStatement(ts::Node);
    explicit DoStatement(const Body&, minilua::Range);
    /**
     *
     * @return a body containing all statements of the do block
     */
    auto body() const -> Body;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for identifier_nodes
 */
class Identifier {
    using IdTuple = std::tuple<std::string, minilua::Range>;
    enum TupleIndex { STRING, RANGE };
    std::variant<ts::Node, IdTuple> content;

public:
    explicit Identifier(ts::Node);
    explicit Identifier(const std::string&, minilua::Range);
    /**
     * get the identifier name as a string
     * @return the identifer as a string
     */
    auto string() const -> std::string;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * this enum holds all possible BinaryOperators in lua
 */
enum class BinOpEnum {
    ADD,         //      +
    SUB,         //      -
    MUL,         //      *
    DIV,         //      /
    MOD,         //      %
    POW,         //      ^
    LT,          //       <
    GT,          //       >
    LEQ,         //      <=
    GEQ,         //      >=
    EQ,          //       ==
    NEQ,         //      ~=
    CONCAT,      //   ..
    AND,         //      and
    OR,          //       or
    SHIFT_LEFT,  //      <<
    SHIFT_RIGHT, //      >>
    BIT_XOR,     //    ~
    BIT_OR,      //     |
    BIT_AND,     //    &
    INT_DIV      //    //
};
/**
 * class for binary_operation nodes
 */
class BinaryOperation {
    using BinOpTuple = std::tuple<
        std::shared_ptr<Expression>, BinOpEnum, std::shared_ptr<Expression>, minilua::Range>;
    enum TupleIndex { LEFT, OPERATOR, RIGHT, RANGE };
    std::variant<ts::Node, BinOpTuple> content;

public:
    explicit BinaryOperation(ts::Node node);
    explicit BinaryOperation(const Expression&, BinOpEnum, const Expression&, minilua::Range);
    /**
     *
     * @return The left operand
     */
    auto left() const -> Expression;
    /**
     *
     * @return The right operand
     */
    auto right() const -> Expression;
    /**
     *
     * @return the operator of the operation
     */
    auto binary_operator() const -> BinOpEnum;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * This enum holds all unary Operators of lua
 */
enum class UnOpEnum {
    NOT,  //    not
    NEG,  //    -
    LEN,  //    #
    BWNOT //   ~
};
/**
 * class for unary_operation nodes
 */
class UnaryOperation {
    using UnOpTuple = std::tuple<UnOpEnum, std::shared_ptr<Expression>, minilua::Range>;
    enum TupleIndex { OPERATOR, OPERAND, RANGE };
    std::variant<ts::Node, UnOpTuple> content;

public:
    explicit UnaryOperation(ts::Node node);
    explicit UnaryOperation(UnOpEnum, const Expression&, minilua::Range);
    /**
     *
     * @return the operator of the operation
     */
    auto unary_operator() const -> UnOpEnum;
    /**
     *
     * @return the operand of
     */
    auto expression() const -> Expression;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for loop_expression  nodes
 */
class LoopExpression {
    ts::Node loop_exp;

public:
    explicit LoopExpression(ts::Node);
    /**
     *
     * @return The identifier of the loop variable
     */
    auto variable() const -> Identifier;
    /**
     *
     * @return The start value of the loop variable
     */
    auto start() const -> Expression;
    /**
     *
     * @return the step size of the loop variable if specified
     *          otherwise an empty optional (then the step size is 1 by default)
     */
    auto step() const -> std::optional<Expression>;
    /**
     *
     * @return the end value of the loop variable
     */
    auto end() const -> Expression;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for for_statement nodes
 */
class ForStatement {
    ts::Node for_statement;

public:
    explicit ForStatement(ts::Node node);
    /**
     *
     * @return returns the loop expression of the loop
     */
    auto loop_expression() const -> LoopExpression;
    /**
     *
     * @return a Body containing all statements inside the loop
     */
    auto body() const -> Body;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
    auto desugar() const -> DoStatement;
};
/**
 * class for in_loop_expression nodes
 */
class InLoopExpression {
    ts::Node loop_exp;

public:
    explicit InLoopExpression(ts::Node);
    /**
     *
     * @return all identifiers that are specified as loop variables by the loop
     */
    auto loop_vars() const -> std::vector<Identifier>;
    /**
     * the loop expressions usually should eveluate to a functioncall
     * @return the loop expressions
     */
    auto loop_exps() const -> std::vector<Expression>;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for for_in_statement nodes
 */
class ForInStatement {
    ts::Node for_in;

public:
    explicit ForInStatement(ts::Node);
    /**
     *
     * @return a Body containing all statements inside the loop
     */
    auto body() const -> Body;
    /**
     *
     * @return the corresponding loop expression
     */
    auto loop_expression() const -> InLoopExpression;
    auto desugar() const -> DoStatement;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for while_statement nodes
 */
class WhileStatement {
    using WhileTuple =
        std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Body>, minilua::Range>;
    enum TupleIndex { CONDITION, BODY, RANGE };
    std::variant<ts::Node, WhileTuple> content;

public:
    explicit WhileStatement(ts::Node node);
    explicit WhileStatement(const Expression&, const Body&, minilua::Range);
    /**
     * @return an expression containing the conditional expression of the loop
     */
    auto repeat_conditon() const -> Expression;
    /**
     *
     * @return a body with all statements inside the loop
     */
    auto body() const -> Body;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for repeat_statement nodes
 */
class RepeatStatement {
    ts::Node repeat_statement;

public:
    explicit RepeatStatement(ts::Node node);
    /**
     * @return an expression containing the conditional expression of the loop
     */
    auto repeat_condition() const -> Expression;
    /**
     *
     * @return a body with all statements inside the loop
     */
    auto body() const -> Body;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for else_if nodes
 */
class ElseIf {
    ts::Node else_if;

public:
    explicit ElseIf(ts::Node);
    /**
     *
     * @return a body with all statements inside the else if statement
     */
    auto body() const -> Body;
    /**
     *
     * @return an expression that holds the conditional expression of the else_if_statement
     */
    auto condition() const -> Expression;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for else nodes
 */
class Else {
    ts::Node else_statement;

public:
    explicit Else(ts::Node);
    /**
     *
     * @return a body containing the statements in the else block
     */
    auto body() const -> Body;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for if_statement nodes
 */
class IfStatement {
    using IfTuple = std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Body>, minilua::Range>;
    enum TupleIndex { CONDITION, BODY, RANGE };
    std::variant<ts::Node, IfTuple> content;

public:
    explicit IfStatement(ts::Node node);
    /**
     * constructs an if-statement without elseifs or an else
     */
    explicit IfStatement(const Expression&, const Body&, minilua::Range);
    /**
     *
     * @return a body containing the statements of the if block excluding else_if and else
     * statements
     */
    auto body() const -> Body;
    /**
     *
     * @return all else_if statements that are inside this if_statement
     */
    auto elseifs() const -> std::vector<ElseIf>;
    /**
     *
     * @return an expression that holds the conditional expression of the if_statement
     */
    auto condition() const -> Expression;
    /**
     *
     * @return an Else class if there is one
     *          otherwise an empty optional
     */
    auto else_statement() const -> std::optional<Else>;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * a class for return_statement nodes
 */
class Return {
    ts::Node expressions;

public:
    explicit Return(ts::Node node);
    /**
     *
     * @return a vector holding all expressions that will be returned by this statement
     *          the vector can be empty
     */
    auto exp_list() const -> std::vector<Expression>;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for table_index nodes
 */
class TableIndex {
    ts::Node table_index;

public:
    explicit TableIndex(ts::Node);
    /**
     *
     * @return a prefix that eveluates to the table of this table index
     */
    auto table() const -> Prefix;
    /**
     *
     * @return an expression that eveluates to the index
     */
    auto index() const -> Expression;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for field_expression nodes
 */
class FieldExpression {
    using FieldExpTuple = std::tuple<std::shared_ptr<Prefix>, Identifier, minilua::Range>;
    enum TupleIndex { TABLE, PROPERTY, RANGE };
    std::variant<ts::Node, FieldExpTuple> content;

public:
    explicit FieldExpression(ts::Node);
    explicit FieldExpression(const Prefix&, const Identifier&, minilua::Range);
    /**
     *
     * @return a Prefix containing the Identifier for the table
     */
    auto table_id() const -> Prefix;
    /**
     *
     * @return an Identifier for a property of the Table
     */
    auto property_id() const -> Identifier;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
using VarDecVariant = std::variant<Identifier, FieldExpression, TableIndex>;
/**
 * class for variable_declarator nodes
 */
class VariableDeclarator {
    using VDTuple = std::tuple<VarDecVariant, Range>;
    enum TupleIndex { VD_VARIANT, RANGE };
    std::variant<ts::Node, VDTuple> content;

public:
    explicit VariableDeclarator(ts::Node node);
    explicit VariableDeclarator(const Identifier&);
    explicit VariableDeclarator(const FieldExpression&);
    /**
     *
     * @return a variant containing the class this variable declarator gets resolved to
     */
    auto options() const -> VarDecVariant;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for variable_declaration and local_variable_declaration nodes
 */
class VariableDeclaration {
    using VDTuple =
        std::tuple<std::vector<VariableDeclarator>, std::vector<Expression>, minilua::Range>;
    enum TupleIndex { DECLARATORS, DECLARATIONS, RANGE };
    std::variant<ts::Node, VDTuple> content;
    bool local_dec;

public:
    explicit VariableDeclaration(ts::Node node);
    explicit VariableDeclaration(
        bool, const std::vector<VariableDeclarator>&, const std::vector<Expression>&,
        minilua::Range);
    /**
     *
     * @return true if the declaration is local
     */
    auto local() const -> bool;
    /**
     *
     * @return a vector containing all variables declared by the varaible declaration
     */
    auto declarators() const -> std::vector<VariableDeclarator>;
    /**
     *
     * @return a vector with the expressions that get assigned to to the declared variables
     */
    auto declarations() const -> std::vector<Expression>;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for go_to_statements
 */
class GoTo {
    ts::Node go_to;

public:
    explicit GoTo(ts::Node node);
    /**
     *
     * @return the Identifier of the Label that is specified
     */
    auto label() const -> Identifier;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for label_statements
 */
class Label {
    ts::Node label;

public:
    explicit Label(ts::Node node);
    /**
     *
     * @return the identifier of this label
     */
    auto id() const -> Identifier;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for function_name nodes
 */
class FunctionName {
    ts::Node func_name;

public:
    explicit FunctionName(ts::Node);
    /**
     * This method looks through the function name and returns Identifiers in order they reference
     * on tables e.g. the vector [id_1,id_2]  refers to a function name "id_1.id_2" in lua syntax
     * @return a vector containing all identifiers used for the function name
     *
     */
    auto identifier() const -> std::vector<Identifier>;
    /**
     * The identifier returned here is the last one if it is present
     * e.g. identifiers() returns [id_1,id_2] and method returns id_5 then the full functionname is:
     * "id_1.id_2:id_5" in lua syntax
     * @return
     * an empty optional if the function is not a method
     * the function-/method- name if the function is a method
     */
    auto method() const -> std::optional<Identifier>;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * a class for parameter nodes
 */
class Parameters {
    using ParamTuple = std::tuple<std::vector<Identifier>, bool, minilua::Range>;
    enum TupleIndex { IDENTIFIERS, SPREAD, RANGE };
    std::variant<ts::Node, ParamTuple> content;

public:
    explicit Parameters(ts::Node);
    explicit Parameters(const std::vector<Identifier>&, bool, minilua::Range);
    /**
     *
     * @return a vector containing all parameters excluding a potential spread
     */
    auto params() const -> std::vector<Identifier>;
    /**
     * @return true if the last parameter is "spread"
     */
    auto spread() const -> bool;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for function_definition nodes
 */
class FunctionDefinition {
    using FuncDefTuple = std::tuple<Parameters, std::shared_ptr<Body>, minilua::Range>;
    enum TupleIndex { PARAMETERS, BODY, RANGE };
    std::variant<ts::Node, FuncDefTuple> content;

public:
    explicit FunctionDefinition(ts::Node);
    explicit FunctionDefinition(const Parameters&, const Body&, minilua::Range);
    /**
     *
     * @return a body containing all statements of this function
     */
    auto body() const -> Body;
    /**
     *
     * @return the parameters of this function
     */
    auto parameters() const -> Parameters;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for function_statements
 */
class FunctionStatement {
    ts::Node func_stat;
    bool is_local;

public:
    explicit FunctionStatement(ts::Node);
    /**
     *
     * @return a FunctionName class that can be resolved to the function name
     */
    auto name() const -> FunctionName;
    /**
     *
     * @return a body containing all statements of this function
     */
    auto body() const -> Body;
    /**
     *
     * @return a Parameter class containing all information about the Parameters of this function
     */
    auto parameters() const -> Parameters;
    auto local() const -> bool;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
    auto desugar() const -> VariableDeclaration;
};
class FunctionCall {
    using FuncCallTuple = std::tuple<
        std::shared_ptr<Prefix>, std::optional<Identifier>, std::vector<Expression>,
        minilua::Range>;
    enum TupleIndex { PREFIX, METHOD, ARGS, RANGE };
    std::variant<ts::Node, FuncCallTuple> content;

public:
    explicit FunctionCall(ts::Node);
    explicit FunctionCall(
        const Prefix&, const std::optional<Identifier>&, const std::vector<Expression>&,
        minilua::Range);
    /**
const      *
     * @return
   &  * If the call is a method call id() the Prefix should refer to to a table
     * else the Prefix states the functionname
     */
    auto id() const -> Prefix;
    /**
     *
     * @return
     * an empty optional if it is not a method call
     * the functionname if it is a method call
     */
    auto method() const -> std::optional<Identifier>;
    auto args() const -> std::vector<Expression>;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for field_nodes
 */
class Field {
    ts::Node field;

public:
    explicit Field(ts::Node);
    /**
     * this method splits a field into the identifier of the field and the content of the field
     * or just returns the expression
     * an IndexField looks like this in lua: "[IndexField.first()] = IndexField.second()"
     * an IdentifierField looks like this in lua: "IdentifierField.first() = IdentifierField.second"
     * the Expression is
     * @return a variant containing the right format for the field
     */
    auto content() const -> std::variant<IndexField, IdentifierField, Expression>;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * class for table nodes
 */
class Table {
    ts::Node table;

public:
    explicit Table(ts::Node);
    /**
     *
     * @return a vector containing all fields of the table
     */
    auto fields() const -> std::vector<Field>;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
// a few empty classes that are just used as additional return types
class Spread {};
class Self {};
class Break {};
using PrefixVariant = std::variant<Self, VariableDeclarator, FunctionCall, Expression>;
/**
 * class for prefix nodes
 */
class Prefix {
    using modifiedPrefixVariant =
        std::variant<Self, VariableDeclarator, FunctionCall, std::shared_ptr<Expression>>;
    using PrefixTuple = std::tuple<modifiedPrefixVariant, minilua::Range>;
    enum TupleIndex { PFX_VARIANT, RANGE };
    std::variant<ts::Node, PrefixTuple> content;

public:
    explicit Prefix(ts::Node);
    explicit Prefix(const VariableDeclarator&);
    explicit Prefix(const FunctionCall&);
    /**
     *
     * @return a variant containing the class this Prefix gets resolved to
     */
    auto options() const -> PrefixVariant;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
    auto to_string() const -> std::string;
};
using ExpressionVariant = std::variant<
    Spread, Prefix, FunctionDefinition, Table, BinaryOperation, UnaryOperation, Literal,
    Identifier>;
/**
 * class for expression nodes
 */
class Expression {
    using ExpTup = std::tuple<ExpressionVariant, minilua::Range>;
    std::variant<ts::Node, std::tuple<ExpressionVariant, minilua::Range>> content;
    enum TupleIndex { EXP_VARIANT, RANGE };

public:
    explicit Expression(ts::Node);
    explicit Expression(const BinaryOperation&);
    explicit Expression(const UnaryOperation&);
    explicit Expression(const FunctionDefinition&);
    explicit Expression(const Prefix&);
    explicit Expression(const Literal&);
    explicit Expression(const Identifier&);
    /**
     *
     * @return a variant containing the class this expression gets resolved to
     */
    auto options() const -> ExpressionVariant;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
    auto to_string() const -> std::string;
};
using StatementVariant = std::variant<
    VariableDeclaration, DoStatement, IfStatement, WhileStatement, RepeatStatement, ForStatement,
    ForInStatement, GoTo, Break, Label, FunctionStatement, FunctionCall, Expression>;
class Statement {
    using StatementTuple = std::tuple<StatementVariant, minilua::Range>;
    enum TupleIndex { STAT_VARIANT, RANGE };
    std::variant<ts::Node, StatementTuple> content;

public:
    explicit Statement(ts::Node);
    explicit Statement(const VariableDeclaration&);
    explicit Statement(const FunctionCall&);
    explicit Statement(const WhileStatement&);
    explicit Statement(const IfStatement&);
    explicit Statement(const DoStatement&);
    explicit Statement(const Break&, minilua::Range);
    /**
     *
     * @return a variant containing the class this statement gets resolved to
     */
    auto options() const -> StatementVariant;
    auto range() const -> minilua::Range;
    auto debug_print() const -> std::string;
};
/**
 * The Body class groups a variable amount of statements together
 * the last statement of a Body might be a return_statement
 */
class Body {
    using BodyPair = std::pair<std::vector<Statement>, std::optional<Return>>;
    std::variant<std::vector<ts::Node>, BodyPair> content;

public:
    explicit Body(std::vector<ts::Node>);
    explicit Body(std::vector<Statement>, std::optional<Return>);
    /**
     * This method maps the statement Nodes to Statement classes
     * @return the statements in this body excluding a potential return_statement
     */
    auto statements() -> std::vector<Statement>;
    /**
     * This checks for a return node
     * the return_statement is always the last statement of the body
     * @return a Return class if the body has a return statement
     *          else an empty optional
     */
    auto return_statement() -> std::optional<Return>;
};
} // namespace minilua::details::ast

#endif // MINILUA_AST_HPP
