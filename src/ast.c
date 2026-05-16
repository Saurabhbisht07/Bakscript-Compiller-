#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ast.h"

// Helper function to allocate a new node
static Node *create_node(NodeType type)
{
    Node *node = (Node *)malloc(sizeof(Node));
    node->type = type;
    return node;
}

Node *create_number_node(int value, int line, int column)
{
    Node *node = create_node(NODE_NUMBER);
    node->number.value = value;
    node->number.info.line = line;
    node->number.info.column = column;
    return node;
}

Node *create_bool_node(int value, int line, int column)
{
    Node *node = create_node(NODE_BOOL);
    node->boolean.value = value ? 1 : 0;
    node->boolean.info.line = line;
    node->boolean.info.column = column;
    return node;
}

Node *create_string_node(const char *value, int line, int column)
{
    Node *node = create_node(NODE_STRING);
    node->string.value = strdup(value);
    node->string.info.line = line;
    node->string.info.column = column;
    return node;
}

Node *create_identifier_node(const char *name, int line, int column)
{
    Node *node = create_node(NODE_IDENTIFIER);
    node->identifier.name = strdup(name);
    node->identifier.info.line = line;
    node->identifier.info.column = column;
    return node;
}

Node *create_array_literal_node(Node **elements, int count, int line, int column)
{
    Node *node = create_node(NODE_ARRAY_LITERAL);
    node->array_literal.count = count;
    node->array_literal.info.line = line;
    node->array_literal.info.column = column;
    if (count > 0)
    {
        node->array_literal.elements = (Node **)malloc(sizeof(Node *) * (size_t)count);
        memcpy(node->array_literal.elements, elements, sizeof(Node *) * (size_t)count);
    }
    else
    {
        node->array_literal.elements = NULL;
    }
    return node;
}

Node *create_index_access_node(Node *array, Node *index, int line, int column)
{
    Node *node = create_node(NODE_INDEX_ACCESS);
    node->index_access.array = array;
    node->index_access.index = index;
    node->index_access.info.line = line;
    node->index_access.info.column = column;
    return node;
}

Node *create_binary_op_node(BinaryOpType op, Node *left, Node *right, int line, int column)
{
    Node *node = create_node(NODE_BINARY_OP);
    node->binary_op.op = op;
    node->binary_op.left = left;
    node->binary_op.right = right;
    node->binary_op.info.line = line;
    node->binary_op.info.column = column;
    return node;
}

Node *create_unary_op_node(UnaryOpType op, Node *operand, int line, int column)
{
    Node *node = create_node(NODE_UNARY_OP);
    node->unary_op.op = op;
    node->unary_op.operand = operand;
    node->unary_op.info.line = line;
    node->unary_op.info.column = column;
    return node;
}

Node *create_function_call_node(const char *name, Node **arguments, int arg_count, int line, int column)
{
    Node *node = create_node(NODE_FUNCTION_CALL);
    node->function_call.name = strdup(name);
    node->function_call.arg_count = arg_count;
    node->function_call.info.line = line;
    node->function_call.info.column = column;

    if (arg_count > 0)
    {
        node->function_call.arguments = (Node **)malloc(sizeof(Node *) * arg_count);
        memcpy(node->function_call.arguments, arguments, sizeof(Node *) * arg_count);
    }
    else
    {
        node->function_call.arguments = NULL;
    }

    return node;
}

Node *create_var_decl_node(const char *type, const char *name, Node *initializer, int line, int column)
{
    Node *node = create_node(NODE_VARIABLE_DECLARATION);
    node->var_decl.type = strdup(type);
    node->var_decl.name = strdup(name);
    node->var_decl.initializer = initializer;
    node->var_decl.info.line = line;
    node->var_decl.info.column = column;
    return node;
}

Node *create_if_node(Node *condition, Node *if_body, Node *else_body, int line, int column)
{
    Node *node = create_node(NODE_IF_STATEMENT);
    node->if_stmt.condition = condition;
    node->if_stmt.if_body = if_body;
    node->if_stmt.else_body = else_body;
    node->if_stmt.info.line = line;
    node->if_stmt.info.column = column;
    return node;
}

Node *create_for_node(Node *initializer, Node *condition, Node *increment, Node *body, int line, int column)
{
    Node *node = create_node(NODE_FOR_LOOP);
    node->for_loop.initializer = initializer;
    node->for_loop.condition = condition;
    node->for_loop.increment = increment;
    node->for_loop.body = body;
    node->for_loop.info.line = line;
    node->for_loop.info.column = column;
    return node;
}

Node *create_while_node(Node *condition, Node *body, int line, int column)
{
    Node *node = create_node(NODE_WHILE_LOOP);
    node->while_loop.condition = condition;
    node->while_loop.body = body;
    node->while_loop.info.line = line;
    node->while_loop.info.column = column;
    return node;
}

Node *create_function_decl_node(const char *name, char **params, int param_count, Node *body, int line, int column)
{
    Node *node = create_node(NODE_FUNCTION_DECLARATION);
    node->function_decl.name = strdup(name);
    node->function_decl.param_count = param_count;
    node->function_decl.body = body;
    node->function_decl.info.line = line;
    node->function_decl.info.column = column;

    if (param_count > 0)
    {
        node->function_decl.params = (char **)malloc(sizeof(char *) * (size_t)param_count);
        for (int i = 0; i < param_count; i++)
        {
            node->function_decl.params[i] = strdup(params[i]);
        }
    }
    else
    {
        node->function_decl.params = NULL;
    }

    return node;
}

Node *create_return_node(Node *value, int line, int column)
{
    Node *node = create_node(NODE_RETURN_STATEMENT);
    node->return_stmt.value = value;
    node->return_stmt.info.line = line;
    node->return_stmt.info.column = column;
    return node;
}

Node *create_block_node(Node **statements, int count, int line, int column)
{
    Node *node = create_node(NODE_BLOCK);
    node->block.count = count;
    node->block.info.line = line;
    node->block.info.column = column;

    if (count > 0)
    {
        node->block.statements = (Node **)malloc(sizeof(Node *) * count);
        memcpy(node->block.statements, statements, sizeof(Node *) * count);
    }
    else
    {
        node->block.statements = NULL;
    }

    return node;
}

Node *create_program_node(Node **statements, int count)
{
    Node *node = create_node(NODE_PROGRAM);
    node->program.count = count;
    node->program.info.line = 1;
    node->program.info.column = 1;

    if (count > 0)
    {
        node->program.statements = (Node **)malloc(sizeof(Node *) * count);
        memcpy(node->program.statements, statements, sizeof(Node *) * count);
    }
    else
    {
        node->program.statements = NULL;
    }

    return node;
}

void free_node(Node *node)
{
    if (!node)
        return;

    switch (node->type)
    {
    case NODE_BOOL:
        // Bool has no heap members.
        break;

    case NODE_STRING:
        free(node->string.value);
        break;

    case NODE_IDENTIFIER:
        free(node->identifier.name);
        break;

    case NODE_ARRAY_LITERAL:
        for (int i = 0; i < node->array_literal.count; i++)
        {
            free_node(node->array_literal.elements[i]);
        }
        free(node->array_literal.elements);
        break;

    case NODE_INDEX_ACCESS:
        free_node(node->index_access.array);
        free_node(node->index_access.index);
        break;

    case NODE_BINARY_OP:
        free_node(node->binary_op.left);
        free_node(node->binary_op.right);
        break;

    case NODE_UNARY_OP:
        free_node(node->unary_op.operand);
        break;

    case NODE_FUNCTION_CALL:
        free(node->function_call.name);
        for (int i = 0; i < node->function_call.arg_count; i++)
        {
            free_node(node->function_call.arguments[i]);
        }
        free(node->function_call.arguments);
        break;

    case NODE_VARIABLE_DECLARATION:
        free(node->var_decl.type);
        free(node->var_decl.name);
        free_node(node->var_decl.initializer);
        break;

    case NODE_IF_STATEMENT:
        free_node(node->if_stmt.condition);
        free_node(node->if_stmt.if_body);
        if (node->if_stmt.else_body)
        {
            free_node(node->if_stmt.else_body);
        }
        break;

    case NODE_FOR_LOOP:
        free_node(node->for_loop.initializer);
        free_node(node->for_loop.condition);
        free_node(node->for_loop.increment);
        free_node(node->for_loop.body);
        break;

    case NODE_WHILE_LOOP:
        free_node(node->while_loop.condition);
        free_node(node->while_loop.body);
        break;

    case NODE_FUNCTION_DECLARATION:
        free(node->function_decl.name);
        for (int i = 0; i < node->function_decl.param_count; i++)
        {
            free(node->function_decl.params[i]);
        }
        free(node->function_decl.params);
        free_node(node->function_decl.body);
        break;

    case NODE_RETURN_STATEMENT:
        free_node(node->return_stmt.value);
        break;

    case NODE_BLOCK:
    case NODE_PROGRAM:
        for (int i = 0; i < node->block.count; i++)
        {
            free_node(node->block.statements[i]);
        }
        free(node->block.statements);
        break;

    default:
        break;
    }

    free(node);
}