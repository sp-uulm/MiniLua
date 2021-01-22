
#ifndef MINILUA_AST_HPP
#define MINILUA_AST_HPP

#include "MiniLua/values.hpp"
#include <tree_sitter/tree_sitter.hpp>
#include <variant>
namespace minilua::details {
#define INDEX_FIELD std::pair<Expression, Expression>
#define ID_FIELD std::pair<Identifier, Expression>
// Some forward declarations
class Expression;
class Statement;
class Prefix;
class Return;
class FieldExpression;
/**
 * The Body class groups a variable amount of statements together
 * the last statement of a Body might be a return_statement
 */
class Body {
    std::vector<ts::Node> nodes;

public:
    Body(std::vector<ts::Node>);
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
    auto ret() -> std::optional<Return>;
};
/**
 * class for program nodes. It only holds a body
 */
class Program {
    ts::Node program;

public:
    Program(ts::Node);
    /**
     * the children nodes of the Program get put into a Body class by this method
     * @return a Body containing the full program
     */
    auto body() -> Body;
};
/**
 * class for identifier_nodes
 */
class Identifier {
    ts::Node id;

public:
    Identifier(ts::Node);
    /**
     * get the identifier name as a string
     * @return the identifer as a string
     */
    auto str() -> std::string;
};
/**
 * this enum holds all possible BinaryOperators in lua
 */
enum class BinOpEnum {
    ADD,    //      +
    SUB,    //      -
    MUL,    //      *
    DIV,    //      /
    MOD,    //      %
    POW,    //      ^
    LT,     //       <
    GT,     //       >
    LEQ,    //      <=
    GEQ,    //      >=
    EQ,     //       ==
    NEQ,    //      ~=
    CONCAT, //   ..
    AND,    //      and
    OR,     //       or
    BSL,    //      <<
    BSR,    //      >>
    BWNOT,  //    ~
    BWOR,   //     |
    BWAND,  //    &
    INTDIV  //    //
};
/**
 * class for binary_operation nodes
 */
class BinaryOperation {
    ts::Node bin_op;

public:
    BinaryOperation(ts::Node node);
    /**
     *
     * @return The left operand
     */
    auto left() -> Expression;
    /**
     *
     * @return The right operand
     */
    auto right() -> Expression;
    /**
     *
     * @return the operator of the operation
     */
    auto op() -> BinOpEnum;
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
    UnaryOperation(ts::Node node);
    /**
     *
     * @return the operator of the operation
     */
    auto op() -> UnOpEnum;
    /**
     *
     * @return the operand of
     */
    auto exp() -> Expression;
};
/**
 * class for loop_expression  nodes
 */
class LoopExpression {
    ts::Node loop_exp;

public:
    LoopExpression(ts::Node);
    /**
     *
     * @return The identifier of the loop variable
     */
    auto variable() -> Identifier;
    /**
     *
     * @return The start value of the loop variable
     */
    auto start() -> Expression;
    /**
     *
     * @return the step size of the loop variable if specified
     *          otherwise an empty optional (then the step size is 1 by default)
     */
    auto step() -> std::optional<Expression>;
    /**
     *
     * @return the end value of the loop variable
     */
    auto end() -> Expression;
};
/**
 * class for for_statement nodes
 */
class ForStatement {
    ts::Node for_statement;

public:
    ForStatement(ts::Node node);
    /**
     *
     * @return returns the loop expression of the loop
     */
    auto loop_exp() -> LoopExpression;
    /**
     *
     * @return a Body containing all statements inside the loop
     */
    auto body() -> Body;
};
/**
 * class for in_loop_expression nodes
 */
class InLoopExpression {
    ts::Node loop_exp;

public:
    InLoopExpression(ts::Node);
    /**
     *
     * @return all identifiers that are specified as loop variables by the loop
     */
    auto loop_vars() -> std::vector<Identifier>;
    /**
     * the loop expressions usually should eveluate to a functioncall
     * @return the loop expressions
     */
    auto loop_exps() -> std::vector<Expression>;
};
/**
 * class for for_in_statement nodes
 */
class ForInStatement {
    ts::Node for_in;

public:
    ForInStatement(ts::Node);
    /**
     *
     * @return a Body containing all statements inside the loop
     */
    auto body() -> Body;
    /**
     *
     * @return the corresponding loop expression
     */
    auto loop_exp() -> InLoopExpression;
};
/**
 * class for while_statement nodes
 */
class WhileStatement {
    ts::Node while_statement;

public:
    WhileStatement(ts::Node node);
    /**
     * This method gets the contitional expression of the loop. In while loops this is always
     * checked before executing the body statements
     * @return an expression containing the conditional expression of the loop
     */
    auto exit_cond() -> Expression;
    /**
     *
     * @return a body with all statements inside the loop
     */
    auto body() -> Body;
};
/**
 * class for repeat_statement nodes
 */
class RepeatStatement {
    ts::Node repeat_statement;

public:
    RepeatStatement(ts::Node node);
    /**
     * This method gets the conditional expression of the loop. In repeat loops this is always
     * checked after executing the statements of the body
     * @return an expression containing the conditional expression of the loop
     */
    auto until_cond() -> Expression;
    /**
     *
     * @return a body with all statements inside the loop
     */
    auto body() -> Body;
};
/**
 * class for else_if nodes
 */
class ElseIf {
    ts::Node else_if;

public:
    ElseIf(ts::Node);
    /**
     *
     * @return a body with all statements inside the else if statement
     */
    auto body() -> Body;
    /**
     *
     * @return an expression that holds the conditional expression of the else_if_statement
     */
    auto cond() -> Expression;
};
/**
 * class for else nodes
 */
class Else {
    ts::Node else_statement;

public:
    Else(ts::Node);
    /**
     *
     * @return a body containing the statements in the else block
     */
    auto body() -> Body;
};
/**
 * class for if_statement nodes
 */
class IfStatement {
    ts::Node if_statement;

public:
    IfStatement(ts::Node node);
    /**
     *
     * @return a ody conatining the statements of the if block excluding else_if and else statements
     */
    auto body() -> Body;
    /**
     *
     * @return all else_if statements that are inside this if_statement
     */
    auto elseifs() -> std::vector<ElseIf>;
    /**
     *
     * @return an expression that holds the conditional expression of the if_statement
     */
    auto cond() -> Expression;
    /**
     *
     * @return an Else class if there is one
     *          otherwise an empty optional
     */
    auto else_() -> std::optional<Else>;
};
/**
 * a class for return_statement nodes
 */
class Return {
    ts::Node expressions;

public:
    Return(ts::Node node);
    /**
     *
     * @return a vector holding all expressions that will be returned by this statement
     *          the vector can be empty
     */
    auto explist() -> std::vector<Expression>;
};
/**
 * class for table_index nodes
 */
class TableIndex {
    ts::Node table_index;
public:
    TableIndex(ts::Node);
    /**
     *
     * @return a prefix that eveluates to the table of this table index
     */
    auto table() -> Prefix;
    /**
     *
     * @return an expression that eveluates to the index
     */
    auto index() -> Expression;
};
/**
 * class for variable_declarator nodes
 */
class VariableDeclarator {
    ts::Node dec;

public:
    VariableDeclarator(ts::Node node);
    /**
     *
     * @return a variant containing the class this variable declarator gets resolved to
     */
    auto var() -> std::variant<Identifier, FieldExpression, TableIndex>;
};
/**
 * class for variable_declaration nodes
 */
class VariableDeclaration {
    ts::Node var_dec;

public:
    VariableDeclaration(ts::Node node);
    /**
     *
     * @return a vector containing all variables declared by the varaible declaration
     */
    auto declarators() -> std::vector<VariableDeclarator>;
    /**
     *
     * @return a vector with the expressions that get assigned to to the declared variables
     *          there should always be at least one element in it
     */
    auto declarations() -> std::vector<Expression>;
};
/**
 * class for local_variable_declarator nodes
 */
class LocalVariableDeclarator {
    ts::Node var_dec;

public:
    LocalVariableDeclarator(ts::Node node);
    auto vars() -> std::vector<Identifier>;
};

class LocalVariableDeclaration {
    ts::Node local_var_dec;

public:
    LocalVariableDeclaration(ts::Node node);
    /**
     * this method gets all Identifiers of variables declared in this statement
     * @return a vector with all Identifiers
     */
    auto declarators() -> std::vector<Identifier>;
    /**
     *
     * @return a vector with the expressions that get assigned to to the declaraed variables
     */
    auto declarations() -> std::vector<Expression>;
};
/**
 * class for field_expression nodes
 */
class FieldExpression {
    ts::Node exp;

public:
    FieldExpression(ts::Node);
    /**
     *
     * @return a Prefix containing the Identifier for the table
     */
    auto table_id() -> Prefix;
    /**
     *
     * @return an Identifier for a property of the Table
     */
    auto property_id() -> Identifier;
};
/**
 * class for do_statement nodes
 */
class DoStatement {
    ts::Node do_statement;

public:
    DoStatement(ts::Node);
    /**
     *
     * @return a body containing all statements of the do block
     */
    auto body() -> Body;
};
/**
 * class for go_to_statements
 */
class GoTo {
    ts::Node go_to;

public:
    GoTo(ts::Node node);
    /**
     *
     * @return the Identifier of the Label that is specified
     */
    auto label() -> Identifier;
};
/**
 * class for label_statements
 */
class Label {
    ts::Node label;

public:
    Label(ts::Node node);
    /**
     *
     * @return the identifier of this label
     */
    auto id() -> Identifier;
};
/**
 * class for function_name nodes
 */
class FunctionName {
    ts::Node func_name;

public:
    FunctionName(ts::Node);
    /**
     * This method looks through the function name and returns Identifiers in order they reference
     * on tables e.g. the vector [id_1,id_2]  refers to a function name "id_1.id_2" in lua syntax
     * @return a vector containing all identifiers used for the function name
     *
     */
    auto identifier() -> std::vector<Identifier>;
    /**
     * The identifier returned here is the last one if it is present
     * e.g. identifiers() returns [id_1,id_2] and method returns id_5 then the full functionname is:
     * "id_1.id_2:id_5" in lua syntax
     * @return
     * an empty optional if the function is not a method
     * the function-/method- name if the function is a method
     */
    auto method() -> std::optional<Identifier>;
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
    Parameters(ts::Node);
    /**
     * self can only be the first parameter this method looks if the self keyword is present in this
     * parameter list
     * @return true if the first parameter is "self"
     *          false otherwise
     */
    auto leading_self() -> bool;
    /**
     *
     * @return a vector containing all parameters excluding a potential spread at the beginning or
     * and or a potential self at the beginning
     */
    auto params() -> std::vector<Identifier>;
    /**
     * specifies the position of a potential spread contained within the parameters
     * SpreadPos::BEGIN and a leading self is not possible
     * @return SpreadPos::BEGIN if there is a spread as the first parameter
     *          SpreadPos::END if there is a spread as the last parameter
     *          SpreadPos::NO_SPREAD if there is no spread amongst the parameters
     */
    auto spread() -> SpreadPos;
};
/**
 * class for function_definition nodes
 */
class FunctionDefinition {
    ts::Node func_def;

public:
    FunctionDefinition(ts::Node);
    /**
     *
     * @return a body containing all statements of this function
     */
    auto body() -> Body;
    /**
     *
     * @return the parameters of this function
     */
    auto parameters() -> Parameters;
};
/**
 * class for function_statements
 */
class FunctionStatement {
    ts::Node func_stat;

public:
    FunctionStatement(ts::Node);
    /**
     *
     * @return a FunctionName class that can be resolved to the function name
     */
    auto name() -> FunctionName;
    /**
     *
     * @return a body containing all statements of this function
     */
    auto body() -> Body;
    /**
     *
     * @return a Parameter class containing all information about the Parameters of this function
     */
    auto parameters() -> Parameters;
};
/**
 * class for local_function_statements
 */
class LocalFunctionStatement {
    ts::Node func_stat;

public:
    LocalFunctionStatement(ts::Node);
    auto name() -> Identifier;
    auto body() -> Body;
    auto parameters() -> Parameters;
};
class FunctionCall {
    ts::Node func_call;

public:
    FunctionCall(ts::Node);
    /**
     *
     * @return
     * If the call is a method call id() the Prefix should refer to to a table
     * else the Prefix states the functionname
     */
    auto id() -> Prefix;
    /**
     *
     * @return
     * an empty optional if it is not a method call
     * the functionname if it is a method call
     */
    auto method() -> std::optional<Identifier>;
    auto args() -> std::vector<Expression>;
};
enum GV { _G, _VERSION };
class GlobalVariable {
    ts::Node g_var;

public:
    GlobalVariable(ts::Node);
    /**
     *
     * @return the Type of this Global Variable
     */
    auto type() -> GV;
};
/**
 * class for field_nodes
 */
class Field {
    ts::Node field;

public:
    Field(ts::Node);
    /**
     * this method splits a field into the identifier of the field and the content of the field
     * or just returns the expression
     * an INDEX_FIELD looks like this in lua: "[INDEX_FIELD.first()] = INDEX_FIELD.second()"
     * an ID_FIELD looks like this in lua: "ID_FIELD.first() = ID_FIELD.second"
     * the Expression is
     * @return a variant containing the right format for the field
     */
    auto content() -> std::variant<INDEX_FIELD, ID_FIELD, Expression>;
};
/**
 * class for table nodes
 */
class Table {
    ts::Node table;

public:
    Table(ts::Node);
    /**
     *
     * @return a vector containing all fields of the table
     */
    auto fields() -> std::vector<Field>;
};
// a few empty classes that are just used as additional return types
class Spread {};
class Self {};
class Next {};
class Break {};
/**
 * class for prefix nodes
 */
class Prefix {
    ts::Node prefix;

public:
    Prefix(ts::Node);
    /**
     *
     * @return a variant containing the class this Prefix gets resolved to
     */
    auto options()
        -> std::variant<Self, GlobalVariable, VariableDeclarator, FunctionCall, Expression>;
};
/**
 * class for expression nodes
 */
class Expression {
    ts::Node exp;

public:
    Expression(ts::Node);
    /**
     *
     * @return a variant containing the class this expression gets resolved to
     */
    auto options() -> std::variant<
        Spread, Prefix, Next, FunctionDefinition, Table, BinaryOperation, UnaryOperation,
        minilua::Value, Identifier>;
};

class Statement {
    ts::Node statement;

public:
    Statement(ts::Node);
    /**
     *
     * @return a variant containing the class this statement gets resolved to
     */
    auto options() -> std::variant<
        VariableDeclaration, LocalVariableDeclaration, DoStatement, IfStatement, WhileStatement,
        RepeatStatement, ForStatement, ForInStatement, GoTo, Break, Label, FunctionStatement,
        LocalFunctionStatement, FunctionCall, Expression>;
};
} // namespace minilua::details

#endif // MINILUA_AST_HPP
