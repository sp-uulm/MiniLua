#include "MiniLua/tree_sitter_ast.hpp"
#include <boost/fusion/include/for_each.hpp>
#include <iostream>
#include <tree_sitter/tree_sitter.hpp>
using namespace std::string_literals;
namespace minilua {
namespace details {
//Body
Body::Body(std::vector<ts::Node> node_vec) : nodes(node_vec) {

}
auto Body::ret() -> std::optional<Return> {
    if(nodes[nodes.size()-1].type_id()==ts::NODE_RETURN_STATEMENT){
        return Return(nodes[nodes.size()-1]);
    }else{
        return {};
    }
}
auto Body::statements() -> std::vector<Statement> {
    std::vector<ts::Node>::iterator end;
    std::vector<Statement> res;
    if((nodes.end()-1) -> type_id() == ts::NODE_RETURN_STATEMENT){
        res.reserve(nodes.size()-1);
        end = nodes.end()-1;
    }else{
        res.reserve(nodes.size());
        end = nodes.end();
    }
    std::transform(nodes.begin(),end,std::back_inserter(res),[](ts::Node node){return Statement(node);});
    return res;
}
// BinOpEnum
std::ostream& operator<<(std::ostream& os, BinOpEnum& binOpEnum) {
    switch (binOpEnum) {
    case BinOpEnum::ADD:
        return os << '+';
    case BinOpEnum::SUB:
        return os << '-';
    case BinOpEnum::MUL:
        return os << '*';
    case BinOpEnum::DIV:
        return os << '/';
    case BinOpEnum::POW:
        return os << '^';
    case BinOpEnum::MOD:
        return os << '%';
    case BinOpEnum::LEQ:
        return os << "<=";
    case BinOpEnum::GEQ:
        return os << ">=";
    case BinOpEnum::EQ:
        return os << "==";
    case BinOpEnum::LT:
        return os << '<';
    case BinOpEnum::GT:
        return os << '>';
    case BinOpEnum::NEQ:
        return os << "~=";
    case BinOpEnum::AND:
        return os << "and";
    case BinOpEnum::OR:
        return os << "or";
    case BinOpEnum::CONCAT:
        return os << "..";
    }
};
// BinaryOperation
BinaryOperation::BinaryOperation(ts::Node node) : bin_op(node) {
    if (node.type_id() != ts::NODE_BINARY_OPERATION || node.child_count() != 3) {
        throw std::runtime_error("not a binary_operation node");
    }
}
// TODO
auto BinaryOperation::left() -> Expression { return Expression(bin_op.child(0).value()); }
auto BinaryOperation::right() -> Expression { return Expression(bin_op.child(2).value()); }
auto BinaryOperation::op() -> BinOpEnum {
    std::string op_str = bin_op.child(1)->text();
    if (op_str == "+"s) {
        return BinOpEnum::ADD;
    } else if (op_str == "-"s) {
        return BinOpEnum::SUB;
    } else if (op_str == "/"s) {
        return BinOpEnum::DIV;
    } else if (op_str == "*"s) {
        return BinOpEnum::MUL;
    } else if (op_str == "%"s) {
        return BinOpEnum::MOD;
    } else if (op_str == "^"s) {
        return BinOpEnum::POW;
    } else if (op_str == "<"s) {
        return BinOpEnum::LT;
    } else if (op_str == ">"s) {
        return BinOpEnum::GT;
    } else if (op_str == "<="s) {
        return BinOpEnum::LEQ;
    } else if (op_str == ">="s) {
        return BinOpEnum::GEQ;
    } else if (op_str == "==") {
        return BinOpEnum::EQ;
    } else if (op_str == "~="s) {
        return BinOpEnum::NEQ;
    } else if (op_str == ".."s) {
        return BinOpEnum::CONCAT;
    } else if (op_str == "and"s) {
        return BinOpEnum::AND;
    } else if (op_str == "or"s) {
        return BinOpEnum::OR;
    } else if (op_str == "<<"s) {
        return BinOpEnum::BSL;
    } else if (op_str == ">>"s) {
        return BinOpEnum::BSR;
    } else if (op_str == "//"s) {
        return BinOpEnum::INTDIV;
    } else if (op_str == "|"s) {
        return BinOpEnum::BWOR;
    } else if (op_str == "&"s) {
        return BinOpEnum::BWAND;
    } else if (op_str == "~"s) {
        return BinOpEnum::BWNOT;
    } else {
        throw std::runtime_error("Unknown Binary Operator: " + op_str);
    }
}
/*std::ostream& operator<<(std::ostream& os, BinaryOperation& binaryOperation){
    return os << binaryOperation.left().type() << (int)binaryOperation.op() <<
binaryOperation.right().type();
}*/
// UnaryOperation
UnaryOperation::UnaryOperation(ts::Node node) : un_op(node) {
    if (node.type_id() != ts::NODE_UNARY_OPERATION || node.child_count() != 2) {
        throw std::runtime_error("not an unary_operation node");
    }
}

auto UnaryOperation::op() -> UnOpEnum {
    if (this->un_op.child(0)->text() == "not"s) {
        return UnOpEnum::NOT;
    } else if (this->un_op.child(0)->text() == "-"s) {
        return UnOpEnum::NEG;
    } else if (this->un_op.child(0)->text() == "~"s) {
        return UnOpEnum::BWNOT;
    } else if (this->un_op.child(0)->text() == "#"s) {
        return UnOpEnum::LEN;
    } else {
        throw std::runtime_error("unknown Unary Operator: " + this->un_op.child(0)->text());
    }
}

auto UnaryOperation::exp() -> Expression { return Expression(un_op.child(0).value()); }

/*std::ostream& operator<<(std::ostream& os, UnaryOperation& unaryOperation){
    return os << (int)unaryOperation.op() << unaryOperation.exp().type();
}*/

// ForStatement
ForStatement::ForStatement(ts::Node node) : for_statement(node) {
    if (node.type_id() != ts::NODE_FOR_STATEMENT || !node.named_child(0).has_value() || node.named_child(0).value().type_id() != ts::NODE_LOOP_EXPRESSION) {
        throw std::runtime_error("not a for_statement node");
    }
}
auto ForStatement::body() -> Body {
    std::vector<ts::Node> body = for_statement.named_children();
    body.erase(body.begin());
    return Body(body);
}

auto ForStatement::loop_exp() -> ts::Node { return for_statement.named_child(0).value(); }

LoopExpression::LoopExpression(ts::Node node) : loop_exp(node) {
    if (node.type_id() != ts::NODE_LOOP_EXPRESSION || node.child_count()<3) {
        throw std::runtime_error("not a loop_expression node");
    }
}
auto LoopExpression::variable() -> Identifier {return Identifier(loop_exp.named_child(0).value());}
auto LoopExpression::end() -> Expression {
    if(loop_exp.child_count()==4){
        return Expression(loop_exp.named_child(3).value());
    }else{
        return Expression(loop_exp.named_child(2).value());
    }
}
auto LoopExpression::start() -> Expression { return Expression(loop_exp.named_child(0).value()); }
auto LoopExpression::step() -> std::optional<Expression> {
    if(loop_exp.child_count()==4){
        return Expression(loop_exp.named_child(2).value());
    }else{
        return {};
    }
}
//TODO find way to split childern
InLoopExpression::InLoopExpression(ts::Node node) : loop_exp(node){
    if(node.type_id()!=ts::NODE_LOOP_EXPRESSION){
        throw std::runtime_error("not a loop_expression node");
    }

}
auto InLoopExpression::loop_exps() -> std::vector<Expression> {
    std::vector<ts::Node> body = loop_exp.named_children();
    for(int i=0;i<body.size();i++){
        if(body[i].type_id()!=ts::NODE_IDENTIFIER){
            std::vector<Expression> res;
            std::transform(body.begin()+i,body.end(),std::back_inserter(res),[](ts::Node node){return Expression(node);});
            return res;
        }
    }
}
auto InLoopExpression::loop_vars() -> std::vector<Identifier> {
    std::vector<ts::Node> body = loop_exp.named_children();
    for(int i=0;i<body.size();i++){
        if(body[i].type_id()!=ts::NODE_IDENTIFIER){
            std::vector<Identifier> res;
            std::transform(body.begin(),body.begin()+i,std::back_inserter(res),[](ts::Node node){return Identifier(node);});
            return res;
        }
    }
}
WhileStatement::WhileStatement(ts::Node node) : while_statement(node){
    if (node.type_id() != ts::NODE_WHILE_STATEMENT || !node.named_child(0).has_value() ||node.named_child(0).value().type_id()!=ts::NODE_CONDITION_EXPRESSION) {
        throw std::runtime_error("not a while_statement node");
    }
}

auto WhileStatement::body() -> Body {
    std::vector<ts::Node> body = while_statement.named_children();
    body.erase(body.begin());
    return Body(body);
}
auto WhileStatement::exit_cond() -> Expression {
    return Expression(while_statement.named_child(0).value());
}
RepeatStatement::RepeatStatement(ts::Node node) : repeat_statement(node){
    if (node.type_id() != ts::NODE_REPEAT_STATEMENT || (node.named_children().end()-1)->type_id()!=ts::NODE_CONDITION_EXPRESSION) {
        throw std::runtime_error("not a repeat_statement node");
    }
}
auto RepeatStatement::body() -> Body {
    std::vector<ts::Node> body = repeat_statement.named_children();
    body.pop_back();
    return Body(body);
}
auto RepeatStatement::until_cond() -> Expression {
    return Expression(repeat_statement.named_child(repeat_statement.named_child_count()-1).value());
}
//If
IfStatement::IfStatement(ts::Node node) : if_statement(node){
    if (node.type_id() != ts::NODE_IF_STATEMENT || !node.named_child(0).has_value() || node.named_child(0).value().type_id()!=ts::NODE_CONDITION_EXPRESSION) {
        throw std::runtime_error("not an if_statement node");
    }
}
auto IfStatement::cond() -> Expression {
    return Expression(if_statement.named_child(0).value());
}
auto IfStatement::else_() -> std::optional<Else> {
    if(if_statement.named_child(if_statement.named_child_count()-1).has_value() && if_statement.named_child(if_statement.named_child_count()-1)->type_id()==ts::NODE_ELSE){
        return Else(if_statement.named_child(if_statement.named_child_count()-1).value());
    }else{
        return {};
    }
}
auto IfStatement::elseifs() -> std::vector<ElseIf> {
    std::vector<ts::Node> if_children = if_statement.named_children();
    std::vector<ts::Node>::iterator end;
    std::vector<ElseIf> res;
    if((if_children.end()-1)->type_id()==ts::NODE_ELSE){
        end = if_children.end()-1;
    }else{
        end = if_children.end();
    }
    std::vector<ts::Node>::iterator begin = end;
    if(begin->type_id()!=ts::NODE_ELSEIF){
        return res;
    }
    while((begin--)->type_id()==ts::NODE_ELSEIF){
        begin--;
    }
    std::transform(begin,end,std::back_inserter(res),[](ts::Node node){return ElseIf(node);});
}
auto IfStatement::body() -> Body {
    std::vector<ts::Node> if_children = if_statement.named_children();
    std::vector<ts::Node>::iterator end;
    if((if_children.end()-1)->type_id()==ts::NODE_ELSE){
        end = if_children.end()-1;
    }else{
        end = if_children.end();
    }
    if(end->type_id()!=ts::NODE_ELSEIF){
        return Body(std::vector<ts::Node>(if_children.begin()+1,end));
    }
    while((end--)->type_id()==ts::NODE_ELSEIF){
        end--;
    }
    return Body(std::vector<ts::Node>(if_children.begin()+1,end));
}
//Else
Else::Else(ts::Node node) : else_statement(node){
    if(node.type_id()!=ts::NODE_ELSE){
        throw std::runtime_error("Not an else_statement node");
    }
}
auto Else::body() -> Body {
    return Body(else_statement.named_children());
}
//ElseIf
ElseIf::ElseIf(ts::Node node) : else_if(node) {
    if (node.type_id() != ts::NODE_ELSEIF || !node.named_child(0).has_value() ||node.named_child(0).value().type_id()!=ts::NODE_CONDITION_EXPRESSION) {
        throw std::runtime_error("not a else_if node");
    }
}
auto ElseIf::body() -> Body {
    std::vector<ts::Node> body = else_if.named_children();
    body.erase(body.begin());
    return Body(body);
}
auto ElseIf::cond() -> Expression {
    return Expression(else_if.named_child(0).value());
}
//Return
Return::Return(ts::Node node) : expressions(node){

}
auto Return::explist() -> std::vector<Expression> {
    std::vector<ts::Node> exps = expressions.named_children();
    std::vector<ts::Node>::iterator end;
    std::vector<Expression> res;
    if(exps.size()>1 && exps[exps.size()-1].text()==";"s){
        end = exps.end()-1;
        res.reserve(exps.size()-1);
    }else{
        end = exps.end();
        res.reserve(exps.size());
    }
    std::transform(exps.begin(),end,std::back_inserter(res),[](ts::Node node){return Expression(node);});
    return res;
}
//VariableDeclaration
VariableDeclaration::VariableDeclaration(ts::Node node) : var_dec(node){
    if(node.type_id()!=ts::NODE_VARIABLE_DECLARATION || node.named_child_count()<2 || node.named_child(0).value().type_id() != ts::NODE_VARIABLE_DECLARATOR){
        throw std::runtime_error("not a variable declaration node");
    }
}
auto VariableDeclaration::declarations() -> std::vector<Expression> {
    std::vector<ts::Node> nodes = var_dec.named_children();
    std::vector<Expression> res;
    std::vector<ts::Node>::iterator it = nodes.begin();
    while(it->type_id() == ts::NODE_VARIABLE_DECLARATOR){
        it++;
    }
    res.reserve(nodes.size()-(it-nodes.begin()));
    std::transform(it,nodes.end(),std::back_inserter(res),[](ts::Node node){return Expression(node);});
    return res;
}
auto VariableDeclaration::declarators() -> std::vector<VariableDeclarator> {
    std::vector<ts::Node> nodes = var_dec.named_children();
    std::vector<VariableDeclarator> res;
    std::vector<ts::Node>::iterator it = nodes.begin();
    while(it->type_id() == ts::NODE_VARIABLE_DECLARATOR){
        it++;
    }
    res.reserve(it-nodes.begin());
    std::transform(nodes.begin(),it,std::back_inserter(res),[](ts::Node node){return VariableDeclarator(node);});
    return res;
}
//VariableDeclarator
VariableDeclarator::VariableDeclarator(ts::Node node) : dec(node){
    if(node.type_id() != ts::NODE_VARIABLE_DECLARATOR){
        throw std::runtime_error("not a variable declarator");
    }
}
auto VariableDeclarator::var() -> std::variant<Identifier,FieldExpression,TableIndex> {
    if(dec.named_child_count()==1){
        if(dec.named_child(0) -> type_id() == ts::NODE_IDENTIFIER){
            return Identifier(dec.named_child(0).value());
        }else if(dec.named_child(0) -> type_id() == ts::NODE_FIELD_EXPRESSION){
            return FieldExpression(dec.named_child(0).value());
        }
    }else{
        return TableIndex();//TODO find good constructor for tableindex
    }
}
//LocalVariableDeclaration
LocalVariableDeclaration::LocalVariableDeclaration(ts::Node node) : local_var_dec(node){
    if(node.type_id() != ts::NODE_LOCAL_VARIABLE_DECLARATION || !node.named_child(0).has_value() || node.named_child(0) -> type_id() != ts::NODE_VARIABLE_DECLARATOR){
        throw std::runtime_error("not a local_variable_declaration node");
    }
}
auto LocalVariableDeclaration::declarator() -> LocalVariableDeclarator {
    return LocalVariableDeclarator(local_var_dec.named_child(0).value());
}
auto LocalVariableDeclaration::declarations() -> std::vector<Expression> {
    std::vector<ts::Node> nodes = local_var_dec.named_children();
    std::vector<Expression> res;
    res.reserve(nodes.size()-1);
    std::transform(nodes.begin()+1,nodes.end(),std::back_inserter(res),[](ts::Node node){return Expression(node);});
    return res;
}
//LocalVariableDeclarator
LocalVariableDeclarator::LocalVariableDeclarator(ts::Node node) : var_dec(node){
    if(node.type_id() != ts::NODE_VARIABLE_DECLARATOR){
        throw std::runtime_error("not a local_variable_declarator node");
    }
}
auto LocalVariableDeclarator::vars() -> std::vector<Identifier> {
    std::vector<ts::Node> nodes = var_dec.named_children();
    std::vector<Identifier> res;
    res.reserve(nodes.size());
    std::transform(nodes.begin(),nodes.end(),std::back_inserter(res),[](ts::Node node){return Identifier(node);});
    return res;
}

// Prefix
Prefix::Prefix(ts::Node node) : prefix(node) {
    if (!(node.type_id() == ts::NODE_SELF || node.type_id() == ts::NODE_GLOBAL_VARIABLE ||
          node.type_id() == ts::NODE_FUNCTION_CALL || node.child(0)->text() == "(")) {
        throw std::runtime_error("Not a prefix-node");
    }
}
auto Prefix::options() -> std::variant<Self, GlobalVariable, VariableDeclarator, FunctionCall,
Expression> { if(prefix.type_id() == ts::NODE_SELF){ return Self(); }else if(prefix.type_id() ==
ts::NODE_GLOBAL_VARIABLE){ return GlobalVariable(prefix); }else if(prefix.type_id() ==
ts::NODE_FUNCTION_CALL){ return FunctionCall(prefix); }else if(prefix.child(0)->text() == "("){
        return Expression(prefix.child(1).value());
    }else{
        throw std::runtime_error("Not a prefix-node");
    }
}
//Expression
Expression::Expression(ts::Node node) : exp(node){
    if(!(node.type_id() == ts::NODE_SPREAD || node.type_id() == ts::NODE_NEXT || node.type_id() ==
ts::NODE_FUNCTION_DEFINITION || node.type_id() == ts::NODE_TABLE || node.type_id() ==
ts::NODE_BINARY_OPERATION || node.type_id() == ts::NODE_UNARY_OPERATION || node.type_id() ==
ts::NODE_STRING || node.type_id() == ts::NODE_NUMBER || node.type_id() == ts::NODE_NIL ||
node.type_id() == ts::NODE_FALSE || node.type_id() == ts::NODE_TRUE || node.type_id() ==
ts::NODE_IDENTIFIER || (node.type_id() == ts::NODE_SELF || node.type_id() ==
ts::NODE_GLOBAL_VARIABLE || node.type_id() == ts::NODE_FUNCTION_CALL || node.child(0)->text() ==
"("))){ throw std::runtime_error("Not an expression-node");
    }
}
auto Expression::options() -> std::variant<
    Spread, Prefix, Next, FunctionDefinition, Table, BinaryOperation, UnaryOperation,
    minilua::Value, Identifier> {
    if (exp.type_id() == ts::NODE_SPREAD) {
        return Spread();
    } else if (exp.type_id() == ts::NODE_NEXT) {
        return Next();
    } else if (exp.type_id() == ts::NODE_FUNCTION_DEFINITION) {
        return FunctionDefinition(exp);
    } else if (exp.type_id() == ts::NODE_TABLE) {
        return Table(exp);
    } else if (exp.type_id() == ts::NODE_BINARY_OPERATION) {
        return BinaryOperation(exp);
    } else if (exp.type_id() == ts::NODE_UNARY_OPERATION) {
        return UnaryOperation(exp);
    } else if (exp.type_id() == ts::NODE_STRING) {
        return minilua::Value(exp.text());
    } else if (exp.type_id() == ts::NODE_NUMBER) {
        return minilua::Value();//TODO parse number and put into value
    } else if (exp.type_id() == ts::NODE_NIL) {
        return minilua::Value(Nil());
    } else if (exp.type_id() == ts::NODE_TRUE) {
        return minilua::Value(true);
    } else if (exp.type_id() == ts::NODE_FALSE) {
        return minilua::Value(false);
    } else if (exp.type_id() == ts::NODE_IDENTIFIER) {
        return Identifier(exp);
    } else if (
        exp.type_id() == ts::NODE_SELF || exp.type_id() == ts::NODE_FUNCTION_CALL ||
        exp.type_id() == ts::NODE_GLOBAL_VARIABLE ||
        (exp.child(0).has_value() && exp.child(0)->text() == "(")) {
        return Prefix(exp);
    } else {
        throw std::runtime_error("Not an expression-node");
    }
}
//Statement
Statement::Statement(ts::Node node) : statement(node){
    if(!(node.type_id() == ts::NODE_EXPRESSION || node.type_id() == ts::NODE_VARIABLE_DECLARATION ||
node.type_id() == ts::NODE_LOCAL_VARIABLE_DECLARATION || node.type_id() == ts::NODE_DO_STATEMENT ||
node.type_id() == ts::NODE_IF_STATEMENT || node.type_id() == ts::NODE_WHILE_STATEMENT ||
node.type_id() == ts::NODE_REPEAT_STATEMENT || node.type_id() == ts::NODE_FOR_STATEMENT ||
node.type_id() == ts::NODE_FOR_IN_STATEMENT || node.type_id() == ts::NODE_GOTO_STATEMENT ||
node.type_id() == ts::NODE_BREAK_STATEMENT || node.type_id() == ts::NODE_LABEL_STATEMENT ||
node.type_id() == ts::NODE_FUNCTION || node.type_id() == ts::NODE_LOCAL_FUNCTION || node.type_id()
== ts::NODE_FUNCTION_CALL || (node.child(0).has_value() && node.child(0) -> text() == ";"))){ throw
std::runtime_error("Not a statement-node");
    }
}
auto Statement::options() -> std::variant<VariableDeclaration, LocalVariableDeclaration,
DoStatement, IfStatement, WhileStatement, RepeatStatement, ForStatement, ForInStatement, GoTo,
Break, Label, Empty, FunctionStatement, LocalFunctionStatement, FunctionCall, Expression> { if(statement.type_id() ==
ts::NODE_EXPRESSION){ return Expression(statement.named_child(0).value()); }else
if(statement.type_id() == ts::NODE_VARIABLE_DECLARATION){ return VariableDeclaration(statement);
    }else if(statement.type_id() == ts::NODE_LOCAL_VARIABLE_DECLARATION){
        return LocalVariableDeclaration(statement);
    }else if(statement.type_id() == ts::NODE_DO_STATEMENT){
        return DoStatement(statement);
    }else if(statement.type_id() == ts::NODE_IF_STATEMENT){
        return IfStatement(statement);
    }else if(statement.type_id() == ts::NODE_WHILE_STATEMENT){
        return WhileStatement(statement);
    }else if(statement.type_id() == ts::NODE_REPEAT_STATEMENT){
        return RepeatStatement(statement);
    }else if(statement.type_id() == ts::NODE_FOR_STATEMENT){
        return ForStatement(statement);
    }else if(statement.type_id() == ts::NODE_FOR_IN_STATEMENT){
        return ForInStatement(statement);
    }else if(statement.type_id() == ts::NODE_GOTO_STATEMENT){
        return GoTo(statement);
    }else if(statement.type_id() == ts::NODE_BREAK_STATEMENT){
        return Break();
    }else if(statement.type_id() == ts::NODE_LABEL_STATEMENT){
        return Label(statement);
    }else if(statement.type_id() == ts::NODE_FUNCTION){
        return FunctionStatement(statement);
    }else if(statement.type_id() == ts::NODE_LOCAL_FUNCTION){
        return LocalFunctionStatement(statement);
    }else if(statement.type_id() == ts::NODE_FUNCTION_CALL){
        return FunctionCall(statement);
    }else if((statement.child(0).has_value() && statement.child(0) -> text() == ";")){
        return Empty();
    }else{
        throw std::runtime_error("Not a statement-node");
    }
}
}
}