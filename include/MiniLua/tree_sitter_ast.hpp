
#ifndef MINILUA_TREE_SITTER_AST_HPP
#define MINILUA_TREE_SITTER_AST_HPP
#include <tree_sitter/tree_sitter.hpp>
#include<variant>
namespace minilua{
namespace details {
class Expression;
class Statement;
class Prefix;
class Return;
class Body{
    std::vector<ts::Node> nodes;
public:
    Body(std::vector<ts::Node>);
    auto statements() -> std::vector<Statement>;
    auto ret() -> std::optional<Return>;
};
class Identifier {
    ts::Node node;
public:
    Identifier(ts::Node);
};
enum class BinOpEnum {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    POW,
    LT,
    GT,
    LEQ,
    GEQ,
    EQ,
    NEQ,
    CONCAT,
    AND,
    OR,
    BSL,
    BSR,
    BWNOT,
    BWOR,
    BWAND,
    INTDIV
};

class BinaryOperation {
    ts::Node bin_op;

public:
    BinaryOperation(ts::Node node);
    auto left() -> Expression;
    auto right() -> Expression;
    auto op() -> BinOpEnum;
};
enum class UnOpEnum { NOT, NEG, LEN, BWNOT };
class UnaryOperation {
    ts::Node un_op;

public:
    UnaryOperation(ts::Node node);
    auto op() -> UnOpEnum;
    auto exp() -> Expression;
};
class ForStatement {
    ts::Node for_statement;

public:
    ForStatement(ts::Node node);
    auto loop_exp() -> ts::Node;
    auto body() -> Body;
};
class LoopExpression {
    ts::Node loop_exp;

public:
    LoopExpression(ts::Node);
    auto variable() -> Identifier;
    auto start() -> Expression;
    auto step() -> std::optional<Expression>;
    auto end() -> Expression;
};
class InLoopExpression{
    ts::Node loop_exp;
public:
    InLoopExpression(ts::Node);
    auto loop_vars() -> std::vector<Identifier>;
    auto loop_exps() -> std::vector<Expression>;
};
class ForInStatement {
    ts::Node for_in;
public:
    ForInStatement(ts::Node);
    auto body() -> std::vector<ts::Node>;
    auto loop_exp() -> InLoopExpression;
    auto ret() -> std::optional<Return>;
};
class WhileStatement {
    ts::Node while_statement;
    ts::Node exit_condition;
    std::vector<ts::Node> loop_body;

public:
    WhileStatement(ts::Node node);
    auto exit_cond() -> Expression;
    auto body() -> Body;
};
class RepeatStatement {
    ts::Node repeat_statement;
    ts::Node until_condition;
    std::vector<ts::Node> loop_body;

public:
    RepeatStatement(ts::Node node);
    auto until_cond() -> Expression;
    auto body() -> Body;
};
class ElseIf {
    ts::Node else_if;

public:
    ElseIf(ts::Node);
    auto body() -> Body;
    auto cond() -> Expression;
};
class Else {
    ts::Node else_statement;
public:
    Else(ts::Node);
    auto body() -> Body;
};
class IfStatement {
    ts::Node if_statement;

public:
    IfStatement(ts::Node node);
    auto body() -> Body;
    auto elseifs() -> std::vector<ElseIf>;
    auto cond() -> Expression;
    auto else_() -> std::optional<Else>;
};
//Return
class Return {
    ts::Node expressions;

public:
    Return(ts::Node node);
    auto explist() -> std::vector<Expression>;
};
//TableIndex
class TableIndex{

};
class VariableDeclarator {
    ts::Node dec;
public:
    VariableDeclarator(ts::Node node);
    auto var() -> std::variant<Identifier, FieldExpression, TableIndex>;
};
class VariableDeclaration {
    ts::Node var_dec;
public:
    VariableDeclaration(ts::Node node);
    auto declarators() -> std::vector<VariableDeclarator>;
    auto declarations() -> std::vector<Expression>;
};
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
    auto declarator() -> LocalVariableDeclarator;
    auto declarations() -> std::vector<Expression>;
};

class FieldExpression {
    ts::Node table_identifier;
    ts::Node property_identifier;

public:
    FieldExpression(ts::Node);
    auto table_id() -> Prefix;
    auto property_id() -> Identifier;
};
class DoStatement {
    std::vector<ts::Node> do_statement;
public:
    DoStatement(ts::Node);
    auto body() -> Body;
};
class GoTo {
    ts::Node go_to;

public:
    GoTo(ts::Node node);
    auto label() -> Identifier;
};
class Label {
    ts::Node label;

public:
    Label(ts::Node node);
    auto identifier() -> Identifier;
};
class FunctionDefinition {

};

class FunctionStatement {
    ts::Node function_name;
    std::vector<ts::Node> parameters;
    std::vector<ts::Node> function_body;
};
class FunctionName {
    std::vector<ts::Node> table_references;
    ts::Node name;
    bool method;
};
class LocalFunctionStatement {
    ts::Node local_function_name;
    std::vector<ts::Node> parameters;
    std::vector<ts::Node> function_body;
};
class FunctionCall {
    std::optional<ts::Node> table_reference;
    ts::Node function_name;
    bool method;
    std::vector<ts::Node> arguments;
};
enum GV { _G, _VERSION };
class GlobalVariable {
    ts::Node g_var;
public:
    GlobalVariable(ts::Node);
    auto type() -> GV;
};
class Field{
    ts::Node field;
public:
    Field(ts::Node);

};
class Table {
    ts::Node table;
public:
    Table(ts::Node);
    auto fields() -> std::vector<Field>;
};
class Spread {};
class Self {};
class Next {};
class String {};
class Number {};
class Nil {};
class True {};
class False {};

class Prefix {
    ts::Node prefix;
public:
    Prefix(ts::Node);
    auto options()
        -> std::variant<Self, GlobalVariable, VariableDeclarator, FunctionCall, Expression>;
};
class Expression {
    ts::Node exp;
public:
    Expression(ts::Node);
    auto options() -> std::variant<
        Spread, Prefix, Next, FunctionDefinition, Table, BinaryOperation, UnaryOperation, String, Number, Nil,
        True, False, Identifier>;
};
class Break {};
class Empty {};
class Statement {
    ts::Node statement;

public:
    Statement(ts::Node);
    auto options() -> std::variant<
        VariableDeclaration, LocalVariableDeclaration, DoStatement, IfStatement, WhileStatement,
        RepeatStatement, ForStatement, ForInStatement, GoTo, Break, Label, Empty, FunctionStatement,
        LocalFunctionStatement, FunctionCall, Expression>;
};
}
}

#endif // MINILUA_TREE_SITTER_AST_HPP
