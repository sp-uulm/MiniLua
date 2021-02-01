#ifndef MINILUA_AST_HPP
#define MINILUA_AST_HPP

#include "MiniLua/values.hpp"
#include <tree_sitter/tree_sitter.hpp>
#include <variant>

namespace minilua::details::ast {

// Some forward declarations
class Expression;
class Identifier;
class Statement;
class Prefix;
class Return;
class FieldExpression;

using IndexField = std::pair<Expression, Expression>;
using IdentifierField = std::pair<Identifier, Expression>;

enum class LiteralType{TRUE,FALSE,NIL,NUMBER,STRING};
class Literal {
    std::string literal_content;
    LiteralType literal_type;
    ts::Range literal_range;
public:
    Literal(LiteralType,std::string,ts::Range);
    auto content() const -> std::string;
    auto type() const -> LiteralType;
    auto range() const -> minilua::Range;
};
/**
 * The Body class groups a variable amount of statements together
 * the last statement of a Body might be a return_statement
 */
class Body {
    std::vector<ts::Node> nodes;

public:
    explicit Body(std::vector<ts::Node>);
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
    auto raw() const -> ts::Node;
};
/**
 * class for identifier_nodes
 */
class Identifier {
    ts::Node id;

public:
    explicit Identifier(ts::Node);
    /**
     * get the identifier name as a string
     * @return the identifer as a string
     */
    auto string() const -> std::string;
    auto range() const -> minilua::Range;
    auto raw() const -> ts::Node;
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
    ts::Node bin_op;

public:
    explicit BinaryOperation(ts::Node node);
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
    auto raw() const -> ts::Node;
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
    ts::Node un_op;

public:
    explicit UnaryOperation(ts::Node node);
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
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
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
    auto range() const -> minilua::Range;
    auto raw() const -> ts::Node;
};
/**
 * class for while_statement nodes
 */
class WhileStatement {
    ts::Node while_statement;

public:
    explicit WhileStatement(ts::Node node);
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
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
};
/**
 * class for if_statement nodes
 */
class IfStatement {
    ts::Node if_statement;

public:
    explicit IfStatement(ts::Node node);
    /**
     *
     * @return a body containing the statements of the if block excluding else_if and else statements
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
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
};
/**
 * class for variable_declarator nodes
 */
class VariableDeclarator {
    ts::Node dec;

public:
    explicit VariableDeclarator(ts::Node node);
    /**
     *
     * @return a variant containing the class this variable declarator gets resolved to
     */
    auto options() const -> std::variant<Identifier, FieldExpression, TableIndex>;
    auto range() const -> minilua::Range;
    auto raw() const -> ts::Node;
};
/**
 * class for variable_declaration and local_variable_declaration nodes
 */
class VariableDeclaration {
    ts::Node var_dec;
    bool local_dec;

public:
    explicit VariableDeclaration(ts::Node node);
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
    auto declarations() const-> std::vector<Expression>;
    auto range() const -> minilua::Range;
    auto raw() const -> ts::Node;
};
/**
 * class for field_expression nodes
 */
class FieldExpression {
    ts::Node exp;

public:
    explicit FieldExpression(ts::Node);
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
    auto raw() const -> ts::Node;
};
/**
 * class for do_statement nodes
 */
class DoStatement {
    ts::Node do_statement;

public:
    explicit DoStatement(ts::Node);
    /**
     *
     * @return a body containing all statements of the do block
     */
    auto body() const -> Body;
    auto range() const -> minilua::Range;
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
};
/**
 * an enum defining the different positions a Spread can occur in the parameters of a method
 */
enum SpreadPos { BEGIN, END, NO_SPREAD };
/**
 * a class for parameter nodes
 */
class Parameters {
    ts::Node parameters;

public:
    explicit Parameters(ts::Node);
    /**
     * self can only be the first parameter this method looks if the self keyword is present in this
     * parameter list
     * @return true if the first parameter is "self"
     *          false otherwise
     */
    auto leading_self() const -> bool;
    /**
     *
     * @return a vector containing all parameters excluding a potential spread at the beginning or
     * and or a potential self at the beginning
     */
    auto params() const -> std::vector<Identifier>;
    /**
     * specifies the position of a potential spread contained within the parameters
     * SpreadPos::BEGIN and a leading self is not possible
     * @return SpreadPos::BEGIN if there is a spread as the first parameter
     *          SpreadPos::END if there is a spread as the last parameter
     *          SpreadPos::NO_SPREAD if there is no spread amongst the parameters
     */
    auto spread() const -> SpreadPos;
    auto range() const -> minilua::Range;
    auto raw() const -> ts::Node;
};
/**
 * class for function_definition nodes
 */
class FunctionDefinition {
    ts::Node func_def;

public:
    explicit FunctionDefinition(ts::Node);
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
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
};
class FunctionCall {
    ts::Node func_call;

public:
    explicit FunctionCall(ts::Node);
    /**
     *
     * @return
     * If the call is a method call id() the Prefix should refer to to a table
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
    auto raw() const -> ts::Node;
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
    auto content() const -> std::variant<IndexField , IdentifierField , Expression>;
    auto range() const -> minilua::Range;
    auto raw() const -> ts::Node;
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
    auto raw() const -> ts::Node;
};
// a few empty classes that are just used as additional return types
class Spread {};
class Self {};
class Break {};
/**
 * class for prefix nodes
 */
class Prefix {
    ts::Node prefix;

public:
    explicit Prefix(ts::Node);
    /**
     *
     * @return a variant containing the class this Prefix gets resolved to
     */
    auto options() const
        -> std::variant<Self, VariableDeclarator, FunctionCall, Expression>;
    auto range() const -> minilua::Range;
    auto raw() const -> ts::Node;
};
/**
 * class for expression nodes
 */
class Expression {
    ts::Node exp;

public:
    explicit Expression(ts::Node);
    /**
     *
     * @return a variant containing the class this expression gets resolved to
     */
    auto options() const -> std::variant<
        Spread, Prefix, FunctionDefinition, Table, BinaryOperation, UnaryOperation,
        Literal, Identifier>;
    auto range() const -> minilua::Range;
    auto raw() const -> ts::Node;
};

class Statement {
    ts::Node statement;

public:
    explicit Statement(ts::Node);
    /**
     *
     * @return a variant containing the class this statement gets resolved to
     */
    auto options() const -> std::variant<
        VariableDeclaration, DoStatement, IfStatement, WhileStatement,
        RepeatStatement, ForStatement, ForInStatement, GoTo, Break, Label, FunctionStatement,
        FunctionCall, Expression>;
    auto range() const -> minilua::Range;
    auto raw() const -> ts::Node;
};
} // namespace minilua::details::ast

#endif // MINILUA_AST_HPP
