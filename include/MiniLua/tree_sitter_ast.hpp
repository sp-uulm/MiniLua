
#ifndef MINILUA_TREE_SITTER_AST_HPP
#define MINILUA_TREE_SITTER_AST_HPP
#include <tree_sitter/tree_sitter.hpp>
namespace lua {
namespace rt {
enum class BinOpEnum { ADD, SUB, MUL, DIV, MOD, POW, LT, GT, LEQ, GEQ, EQ, NEQ, CONCAT, AND, OR, BSL,  BSR, BWNOT, BWOR, BWAND, INTDIV};
class Expression {};
class BinaryOperation {
    ts::Node left_node;
    ts::Node right_node;
    ts::Node op_node;

public:
    BinaryOperation(ts::Node node);
    auto left() -> ts::Node;
    auto right() -> ts::Node;
    auto op() -> BinOpEnum;
};
enum class UnOpEnum { NOT, NEG };
class UnaryOperation {
    ts::Node operand_node;
    ts::Node operator_node;

public:
    UnaryOperation(ts::Node node);
    auto op() -> UnOpEnum;
    auto exp() -> ts::Node;
};
class ForStatement {
    ts::Node loop_expression;
    std::vector<ts::Node> loop_body;
    // TODO loopexpression into forstatement
public:
    ForStatement(ts::Node node);
    auto loop_exp() -> ts::Node;
    auto body() -> std::vector<ts::Node>;
};
class LoopExpression {
    ts::Node loop_var;
    ts::Node start_val;
    std::optional<ts::Node> step_val;
    ts::Node end_val;
public:
    LoopExpression(ts::Node);
    auto variable() -> ts::Node;
    auto start() -> ts::Node;
    auto step() -> std::optional<ts::Node>;
    auto end() -> ts::Node;
};
class ForInStatement {
    std::vector<ts::Node> loop_exp_namelist;
    std::vector<ts::Node> loop_exp_explist;
    std::vector<ts::Node> loop_body;
public:
    ForInStatement(ts::Node);
    auto body() -> ts::Node;
    auto loop_vars() -> std::vector<ts::Node>;
    auto loop_exps() -> std::vector<ts::Node>;
};
class WhileStatement {
    ts::Node exit_condition;
    std::vector<ts::Node> loop_body;

public:
    WhileStatement(ts::Node node);
    auto exit_cond() -> ts::Node;
    auto body() -> std::vector<ts::Node>;
};
class RepeatStatement {
    ts::Node until_condition;
    std::vector<ts::Node> loop_body;

public:
    RepeatStatement(ts::Node node);
    auto until_cond() -> ts::Node;
    auto body() -> std::vector<ts::Node>;
};
class IfStatement {
    ts::Node condition;
    std::vector<ts::Node> if_body;
    std::vector<ts::Node> elseif_statements;
    std::optional<ts::Node> else_node;

public:
    IfStatement(ts::Node node);
    auto body() -> std::vector<ts::Node>;
    auto elseifs() -> std::vector<ts::Node>;
    auto cond() -> ts::Node;
    auto else_() -> std::optional<ts::Node>;
};

class ElseIf{
    ts::Node condition;
    std::vector<ts::Node> else_if_body;
public:
    auto body() -> std::vector<ts::Node>;
    auto cond() -> ts::Node;
};
class Else{
    std::vector<ts::Node> else_body;
public:
    Else(ts::Node);
    auto body() -> std::vector<ts::Node>;
};

class Return{
    std::vector<ts::Node> expression_list;
public:
    Return(ts::Node node);
    auto explist() -> std::vector<ts::Node>;
};
class VariableDeclaration{
    std::vector<ts::Node> variable_declarators;
    std::vector<ts::Node> variable_declarations;
public:
    VariableDeclaration(ts::Node node);
    auto declarators() -> std::vector<ts::Node>;
    auto declarations() -> std::vector<ts::Node>;
};
class VariableDeclarator{
    ts::Node variable;
public:
    VariableDeclarator(ts::Node node);
    auto var() -> ts::Node;
};
class LocalVariableDeclaration{
    ts::Node variable_declarator;
    std::vector<ts::Node> variable_declarations;
public:
    LocalVariableDeclaration(ts::Node node);
    auto declarations() -> std::vector<ts::Node>;
};
class LocalVariableDeclarator{
    std::vector<ts::Node> variables;
public:
    LocalVariableDeclarator(ts::Node node);
    auto vars() -> std::vector<ts::Node>;
};
class FieldExpression{
    ts::Node table_identifier;
    ts::Node property_identifier;
public:
    FieldExpression(ts::Node);
    auto table_id() -> ts::Node;
    auto property_id() -> ts::Node;
};
class DoStatement{
    std::vector<ts::Node> do_body;
public:
    DoStatement(ts::Node);
    auto body() -> std::vector<ts::Node>;
};
class GoTo{
    ts::Node identifier;
public:
    GoTo(ts::Node node);
    auto id() -> ts::Node;
};
class Label{
    ts::Node identifier;
public:
    Label(ts::Node node);
    auto id() -> ts::Node;
};
class Function {
    ts::Node function_name;
    std::vector<ts::Node> parameters;
    std::vector<ts::Node> function_body;
};
class FunctionName{
    std::vector<ts::Node> table_references;
    ts::Node name;
    bool method;
};
class LocalFunction {
    ts::Node local_function_name;
    std::vector<ts::Node> parameters;
    std::vector<ts::Node> function_body;
};
class FunctionCall{
    std::optional<ts::Node> table_reference;
    ts::Node function_name;
    bool method;
    std::vector<ts::Node> arguments;

};
enum GV {_G,_VERSION};
class GlobalVariable{
    GV type;
};
class Table{
    std::vector<ts::Node> fields;
};
} // namespace rt
} // namespace lua

#endif // MINILUA_TREE_SITTER_AST_HPP
