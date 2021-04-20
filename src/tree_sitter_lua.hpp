#ifndef TREE_SITTE_LUA_HPP
#define TREE_SITTE_LUA_HPP

#include <tree_sitter/tree_sitter.hpp>

extern "C" const TSLanguage* tree_sitter_lua();

namespace ts {

/**
 * @brief Lua language.
 */
const Language LUA_LANGUAGE = Language(tree_sitter_lua());

// TODO document them
// TODO we should really generate this automatically
// TODO how to deal with unnamed (e.g. operators "+", etc. are still useful)
const TypeId NODE_BREAK_STATEMENT = LUA_LANGUAGE.node_type_id("break_statement", true);
const TypeId NODE_SPREAD = LUA_LANGUAGE.node_type_id("spread", true);
const TypeId NODE_SELF = LUA_LANGUAGE.node_type_id("self", true);
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
const TypeId NODE_LOCAL_VARIABLE_DECLARATOR =
    LUA_LANGUAGE.node_type_id("local_variable_declarator", true);
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
// binary operators
const TypeId NODE_BIN_OP_LOGICAL_OR = LUA_LANGUAGE.node_type_id("lo_or", true);
const TypeId NODE_BIN_OP_LOGICAL_AND = LUA_LANGUAGE.node_type_id("lo_and", true);
const TypeId NODE_BIN_OP_LT = LUA_LANGUAGE.node_type_id("lt", true);
const TypeId NODE_BIN_OP_LEQ = LUA_LANGUAGE.node_type_id("leq", true);
const TypeId NODE_BIN_OP_EQ = LUA_LANGUAGE.node_type_id("eq", true);
const TypeId NODE_BIN_OP_NEQ = LUA_LANGUAGE.node_type_id("neq", true);
const TypeId NODE_BIN_OP_GEQ = LUA_LANGUAGE.node_type_id("geq", true);
const TypeId NODE_BIN_OP_GT = LUA_LANGUAGE.node_type_id("gt", true);
const TypeId NODE_BIN_OP_BITWISE_OR = LUA_LANGUAGE.node_type_id("bit_or", true);
const TypeId NODE_BIN_OP_BITWISE_XOR = LUA_LANGUAGE.node_type_id("bit_xor", true);
const TypeId NODE_BIN_OP_BITWISE_AND = LUA_LANGUAGE.node_type_id("bit_and", true);
const TypeId NODE_BIN_OP_SHIFT_LEFT = LUA_LANGUAGE.node_type_id("shift_left", true);
const TypeId NODE_BIN_OP_SHIFT_RIGHT = LUA_LANGUAGE.node_type_id("shift_right", true);
const TypeId NODE_BIN_OP_ADDITION = LUA_LANGUAGE.node_type_id("add", true);
const TypeId NODE_BIN_OP_SUBTRACTION = LUA_LANGUAGE.node_type_id("sub", true);
const TypeId NODE_BIN_OP_MULTIPLICATION = LUA_LANGUAGE.node_type_id("mul", true);
const TypeId NODE_BIN_OP_DIVISION = LUA_LANGUAGE.node_type_id("div", true);
const TypeId NODE_BIN_OP_INTEGER_DIVISION = LUA_LANGUAGE.node_type_id("int_div", true);
const TypeId NODE_BIN_OP_MODULO = LUA_LANGUAGE.node_type_id("mod", true);
const TypeId NODE_BIN_OP_CONCAT = LUA_LANGUAGE.node_type_id("concat", true);
const TypeId NODE_BIN_OP_POWER = LUA_LANGUAGE.node_type_id("pow", true);
// unary operators
const TypeId NODE_UN_OP_LOGICAL_NOT = LUA_LANGUAGE.node_type_id("lo_not", true);
const TypeId NODE_UN_OP_LENGTH = LUA_LANGUAGE.node_type_id("length", true);
const TypeId NODE_UN_OP_BITWISE_NOT = LUA_LANGUAGE.node_type_id("bit_not", true);
const TypeId NODE_UN_OP_NEGATIVE = LUA_LANGUAGE.node_type_id("negative", true);

} // namespace ts

#endif
