#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/semantic.h"

#define INITIAL_ERROR_CAPACITY 16

static DataType resolve_declared_type(const char *type_keyword, bool strict_types)
{
    if (!type_keyword)
        return TYPE_VOID;

    if (strcmp(type_keyword, "num") == 0)
        return TYPE_NUM;
    if (strcmp(type_keyword, "str") == 0)
        return TYPE_STR;
    if (strcmp(type_keyword, "bool") == 0)
    {
        // In non-strict mode, keep backwards-compatible behaviour by
        // treating bool as a numeric value (0/1) in the static layer.
        return strict_types ? TYPE_BOOL : TYPE_NUM;
    }
    if (strcmp(type_keyword, "array") == 0)
        return TYPE_ARRAY;
    if (strcmp(type_keyword, "object") == 0)
        return TYPE_OBJECT;
    if (strcmp(type_keyword, "auto") == 0)
        return TYPE_AUTO;

    // Fallback to string to avoid crashing on unknown type keywords.
    return TYPE_STR;
}

SemanticContext *create_semantic_context(bool strict_types)
{
    SemanticContext *context = (SemanticContext *)malloc(sizeof(SemanticContext));
    context->symbol_table = create_symbol_table(256);
    context->errors = (SemanticError *)malloc(INITIAL_ERROR_CAPACITY * sizeof(SemanticError));
    context->error_count = 0;
    context->error_capacity = INITIAL_ERROR_CAPACITY;
    context->strict_types = strict_types;
    return context;
}

void add_semantic_error(SemanticContext *context, SemanticErrorType type,
                        const char *message, int line, int column)
{
    if (context->error_count >= context->error_capacity)
    {
        context->error_capacity *= 2;
        context->errors = (SemanticError *)realloc(context->errors,
                                                   context->error_capacity * sizeof(SemanticError));
    }
    SemanticError *error = &context->errors[context->error_count++];
    error->type = type;
    error->message = strdup(message);
    error->line = line;
    error->column = column;
}

void semantic_print_errors(const SemanticContext *context)
{
    if (!context)
        return;
    for (int i = 0; i < context->error_count; i++)
    {
        SemanticError *e = &context->errors[i];
        fprintf(stderr, "Semantic error [%s] at line %d, column %d: %s\n",
                get_error_type_string(e->type), e->line, e->column, e->message);
    }
}

const char *get_error_type_string(SemanticErrorType type)
{
    switch (type)
    {
    case ERROR_UNDEFINED_VARIABLE:
        return "Undefined variable";
    case ERROR_DUPLICATE_VARIABLE:
        return "Duplicate variable declaration";
    case ERROR_TYPE_MISMATCH:
        return "Type mismatch";
    case ERROR_UNINITIALIZED_VARIABLE:
        return "Use of uninitialized variable";
    case ERROR_INVALID_OPERATION:
        return "Invalid operation";
    case ERROR_DIVISION_BY_ZERO:
        return "Division by zero";
    default:
        return "Unknown error";
    }
}

static DataType analyze_binary_op(SemanticContext *context, Node *node)
{
    DataType left_type = get_expression_type(context, node->binary_op.left);
    DataType right_type = get_expression_type(context, node->binary_op.right);

    switch (node->binary_op.op)
    {
    case OP_ADD:
    {
        if (left_type == TYPE_NUM && right_type == TYPE_NUM)
        {
            if (node->binary_op.right->type == NODE_NUMBER)
            {
                double divisor = (double)node->binary_op.right->number.value;
                if (divisor == 0.0)
                {
                    add_semantic_error(context, ERROR_INVALID_OPERATION,
                                       "Division by zero detected",
                                       node->binary_op.info.line,
                                       node->binary_op.info.column);
                }
            }
            return TYPE_NUM;
        }
        if (left_type == TYPE_STR && right_type == TYPE_STR)
            return TYPE_STR;
        if (left_type == TYPE_STR && right_type == TYPE_NUM)
            return TYPE_STR;
        if (left_type == TYPE_NUM && right_type == TYPE_STR)
            return TYPE_STR;
        add_semantic_error(context, ERROR_TYPE_MISMATCH,
                           "Invalid '+' operands (expect num+num or string concatenation)",
                           node->binary_op.info.line,
                           node->binary_op.info.column);
        return TYPE_NUM;
    }

    case OP_SUBTRACT:
    case OP_MULTIPLY:
    case OP_DIVIDE:
        if (left_type != TYPE_NUM || right_type != TYPE_NUM)
        {
            add_semantic_error(context, ERROR_TYPE_MISMATCH,
                               "Arithmetic operations require numeric operands",
                               node->binary_op.info.line,
                               node->binary_op.info.column);
            return TYPE_NUM; 
        }
        if (node->binary_op.op == OP_DIVIDE && node->binary_op.right->type == NODE_NUMBER)
        {
            double divisor = (double)node->binary_op.right->number.value;
            if (divisor == 0.0)
            {
                add_semantic_error(context, ERROR_INVALID_OPERATION,
                                   "Division by zero detected",
                                   node->binary_op.info.line,
                                   node->binary_op.info.column);
            }
        }
        return TYPE_NUM;

    case OP_LESS:
    case OP_GREATER:
    case OP_LESS_EQ:
    case OP_GREATER_EQ:
    case OP_EQUAL:
    case OP_NOT_EQUAL:
        if (left_type != right_type)
        {
            add_semantic_error(context, ERROR_TYPE_MISMATCH,
                               "Comparison operators require operands of the same type",
                               node->binary_op.info.line,
                               node->binary_op.info.column);
        }
        return TYPE_NUM;

    case OP_AND:
    case OP_OR:
        if (left_type != TYPE_NUM || right_type != TYPE_NUM)
        {
            add_semantic_error(context, ERROR_TYPE_MISMATCH,
                               "Logical operators require numeric (boolean-style) operands",
                               node->binary_op.info.line,
                               node->binary_op.info.column);
        }
        return TYPE_NUM;

    case OP_ASSIGN:
        // Allow assignments to array elements without enforcing a static element type.
        // Arrays are treated as dynamically typed (TYPE_VOID / TYPE_ARRAY) in the
        // semantic layer, so an expression like `nums[1] = 99;` should not trigger
        // a type mismatch solely because of the element type.
        if (node->binary_op.left->type == NODE_INDEX_ACCESS)
        {
            (void)get_expression_type(context, node->binary_op.left);
            (void)get_expression_type(context, node->binary_op.right);
            return TYPE_VOID;
        }

        if (left_type != right_type && right_type != TYPE_VOID)
        {
            add_semantic_error(context, ERROR_TYPE_MISMATCH,
                               "Cannot assign value of different type",
                               node->binary_op.info.line,
                               node->binary_op.info.column);
        }
        return left_type;

    default:
        add_semantic_error(context, ERROR_INVALID_OPERATION,
                           "Unknown binary operator",
                           node->binary_op.info.line,
                           node->binary_op.info.column);
        return TYPE_NUM;
    }
}

static DataType get_function_return_type(const char *func_name)
{
    if (strcmp(func_name, "ask") == 0)
        return TYPE_STR;
    else if (strcmp(func_name, "show") == 0)
        return TYPE_VOID;
    return TYPE_VOID;
}

DataType get_expression_type(SemanticContext *context, Node *expr)
{
    if (!expr)
        return TYPE_VOID;

    switch (expr->type)
    {
    case NODE_NUMBER:
        return TYPE_NUM;

    case NODE_STRING:
        return TYPE_STR;

    case NODE_BOOL:
        // In strict mode, track booleans as a dedicated type; otherwise they
        // behave like numbers in the static layer (as they always have).
        return context->strict_types ? TYPE_BOOL : TYPE_NUM;

    case NODE_ARRAY_LITERAL:
        // Arrays are dynamically typed at runtime. In non-strict mode we treat
        // them as an untyped value so existing programs keep working; in strict
        // mode we expose a dedicated TYPE_ARRAY.
        for (int i = 0; i < expr->array_literal.count; i++)
        {
            (void)get_expression_type(context, expr->array_literal.elements[i]);
        }
        return context->strict_types ? TYPE_ARRAY : TYPE_VOID;

    case NODE_INDEX_ACCESS:
        // Indexing produces a runtime value (element type depends on the array).
        // Validate subexpressions but do not enforce a static element type yet.
        (void)get_expression_type(context, expr->index_access.array);
        (void)get_expression_type(context, expr->index_access.index);
        return TYPE_VOID;

    case NODE_IDENTIFIER:
    {
        Symbol *symbol = symbol_table_lookup(context->symbol_table, expr->identifier.name);
        if (!symbol)
        {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Use of undefined variable '%s'", expr->identifier.name);
            add_semantic_error(context, ERROR_UNDEFINED_VARIABLE,
                               error_msg, expr->identifier.info.line, expr->identifier.info.column);
            return TYPE_VOID;
        }
        if (!symbol->is_initialized)
        {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Variable '%s' is used before being initialized", expr->identifier.name);
            add_semantic_error(context, ERROR_UNINITIALIZED_VARIABLE,
                               error_msg, expr->identifier.info.line, expr->identifier.info.column);
        }
        return symbol->data_type;
    }

    case NODE_BINARY_OP:
        return analyze_binary_op(context, expr);

    case NODE_UNARY_OP:
    {
        DataType t = get_expression_type(context, expr->unary_op.operand);
        if (expr->unary_op.op == UOP_NOT)
        {
            if (context->strict_types)
            {
                if (t != TYPE_BOOL && t != TYPE_NUM)
                {
                    add_semantic_error(context, ERROR_TYPE_MISMATCH,
                                       "'!' expects a bool or numeric operand",
                                       expr->unary_op.info.line,
                                       expr->unary_op.info.column);
                }
            }
            else if (t != TYPE_NUM && t != TYPE_VOID)
            {
                add_semantic_error(context, ERROR_TYPE_MISMATCH,
                                   "'!' expects a numeric operand",
                                   expr->unary_op.info.line,
                                   expr->unary_op.info.column);
            }
        }
        return TYPE_NUM;
    }

    case NODE_FUNCTION_CALL:
        return get_function_return_type(expr->function_call.name);

    default:
        return TYPE_VOID;
    }
}

static void analyze_variable_declaration(SemanticContext *context, Node *node)
{
    DataType var_type = resolve_declared_type(node->var_decl.type, context->strict_types);

    if (!symbol_table_insert(context->symbol_table, node->var_decl.name,
                             SYMBOL_VARIABLE, var_type))
    {
        add_semantic_error(context, ERROR_DUPLICATE_VARIABLE,
                           "Variable already declared in this scope",
                           node->var_decl.info.line,
                           node->var_decl.info.column);
        return;
    }

    if (node->var_decl.initializer)
    {
        // Allow arrays to be assigned into variables even though the surface syntax
        // originally only had `num` / `str` declarations. Runtime values support
        // arrays and objects, and in non-strict mode we stay permissive.
        if (node->var_decl.initializer->type == NODE_ARRAY_LITERAL &&
            (var_type == TYPE_ARRAY || !context->strict_types))
        {
            for (int i = 0; i < node->var_decl.initializer->array_literal.count; i++)
            {
                (void)get_expression_type(context, node->var_decl.initializer->array_literal.elements[i]);
            }
            symbol_table_set_initialized(context->symbol_table, node->var_decl.name);
            return;
        }

        // Type inference (`auto` keyword)
        if (var_type == TYPE_AUTO)
        {
            DataType inferred = get_expression_type(context, node->var_decl.initializer);
            if (!context->strict_types && node->var_decl.initializer->type == NODE_ARRAY_LITERAL)
            {
                inferred = TYPE_ARRAY;
            }
            if (inferred == TYPE_VOID)
            {
                add_semantic_error(context, ERROR_TYPE_MISMATCH,
                                   "Could not infer type for 'auto' variable",
                                   node->var_decl.info.line,
                                   node->var_decl.info.column);
                return;
            }
            var_type = inferred;
            // Update symbol table entry with the inferred type.
            Symbol *sym = symbol_table_lookup(context->symbol_table, node->var_decl.name);
            if (sym)
            {
                sym->data_type = var_type;
            }
        }

        DataType init_type = get_expression_type(context, node->var_decl.initializer);
        if (init_type != var_type && init_type != TYPE_VOID)
        {
            add_semantic_error(context, ERROR_TYPE_MISMATCH,
                               "Initializer type does not match variable type",
                               node->var_decl.info.line,
                               node->var_decl.info.column);
        }
        else if (node->var_decl.initializer->type == NODE_FUNCTION_CALL ||
                 init_type == var_type)
        {
            symbol_table_set_initialized(context->symbol_table, node->var_decl.name);
        }
    }
}

static void analyze_if_statement(SemanticContext *context, Node *node)
{
    DataType cond_type = get_expression_type(context, node->if_stmt.condition);
    if (context->strict_types)
    {
        if (cond_type != TYPE_BOOL && cond_type != TYPE_NUM)
        {
            add_semantic_error(context, ERROR_TYPE_MISMATCH,
                               "If condition must be a bool or numeric expression",
                               node->if_stmt.info.line,
                               node->if_stmt.info.column);
        }
    }
    else if (cond_type != TYPE_NUM)
    {
        add_semantic_error(context, ERROR_TYPE_MISMATCH,
                           "If condition must be a numeric expression",
                           node->if_stmt.info.line,
                           node->if_stmt.info.column);
    }
    symbol_table_enter_scope(context->symbol_table);
    analyze_program(context, node->if_stmt.if_body);
    symbol_table_exit_scope(context->symbol_table);

    if (node->if_stmt.else_body)
    {
        symbol_table_enter_scope(context->symbol_table);
        analyze_program(context, node->if_stmt.else_body);
        symbol_table_exit_scope(context->symbol_table);
    }
}

static void analyze_while_loop(SemanticContext *context, Node *node)
{
    DataType cond_type = get_expression_type(context, node->while_loop.condition);
    if (context->strict_types)
    {
        if (cond_type != TYPE_BOOL && cond_type != TYPE_NUM && cond_type != TYPE_VOID)
        {
            add_semantic_error(context, ERROR_TYPE_MISMATCH,
                               "While condition must be a bool or numeric expression",
                               node->while_loop.info.line,
                               node->while_loop.info.column);
        }
    }
    else if (cond_type != TYPE_NUM && cond_type != TYPE_VOID)
    {
        add_semantic_error(context, ERROR_TYPE_MISMATCH,
                           "While condition must be a numeric expression",
                           node->while_loop.info.line,
                           node->while_loop.info.column);
    }
    analyze_program(context, node->while_loop.body);
}

static void analyze_function_declaration(SemanticContext *context, Node *node)
{
    if (!symbol_table_insert(context->symbol_table, node->function_decl.name,
                             SYMBOL_FUNCTION, TYPE_VOID))
    {
        add_semantic_error(context, ERROR_DUPLICATE_VARIABLE,
                           "Function or variable already declared in this scope",
                           node->function_decl.info.line,
                           node->function_decl.info.column);
        return;
    }

    symbol_table_enter_scope(context->symbol_table);
    for (int i = 0; i < node->function_decl.param_count; i++)
    {
        const char *param = node->function_decl.params[i];
        if (!symbol_table_insert(context->symbol_table, param, SYMBOL_VARIABLE, TYPE_NUM))
        {
            add_semantic_error(context, ERROR_DUPLICATE_VARIABLE,
                               "Duplicate parameter name",
                               node->function_decl.info.line,
                               node->function_decl.info.column);
            symbol_table_exit_scope(context->symbol_table);
            return;
        }
        symbol_table_set_initialized(context->symbol_table, param);
    }
    analyze_program(context, node->function_decl.body);
    symbol_table_exit_scope(context->symbol_table);
}

static void analyze_for_loop(SemanticContext *context, Node *node)
{
    symbol_table_enter_scope(context->symbol_table);

    if (node->for_loop.initializer)
    {
        analyze_program(context, node->for_loop.initializer);
    }

    if (node->for_loop.condition)
    {
        DataType cond_type = get_expression_type(context, node->for_loop.condition);
        if (context->strict_types)
        {
            if (cond_type != TYPE_BOOL && cond_type != TYPE_NUM)
            {
                add_semantic_error(context, ERROR_TYPE_MISMATCH,
                                   "For loop condition must be a bool or numeric expression",
                                   node->for_loop.info.line,
                                   node->for_loop.info.column);
            }
        }
        else if (cond_type != TYPE_NUM)
        {
            add_semantic_error(context, ERROR_TYPE_MISMATCH,
                               "For loop condition must be a numeric expression",
                               node->for_loop.info.line,
                               node->for_loop.info.column);
        }
    }

    if (node->for_loop.increment)
    {
        analyze_program(context, node->for_loop.increment);
    }

    analyze_program(context, node->for_loop.body);
    symbol_table_exit_scope(context->symbol_table);
}

static void analyze_function_call(SemanticContext *context, Node *node)
{
    for (int i = 0; i < node->function_call.arg_count; i++)
    {
        Node *arg = node->function_call.arguments[i];
        DataType arg_type = get_expression_type(context, arg);
    }
}

bool analyze_program(SemanticContext *context, Node *node)
{
    if (!node)
        return true;
    switch (node->type)
    {
    case NODE_PROGRAM:
    case NODE_BLOCK:
        for (int i = 0; i < node->block.count; i++)
        {
            analyze_program(context, node->block.statements[i]);
        }
        break;

    case NODE_VARIABLE_DECLARATION:
        analyze_variable_declaration(context, node);
        break;

    case NODE_IF_STATEMENT:
        analyze_if_statement(context, node);
        break;

    case NODE_FOR_LOOP:
        analyze_for_loop(context, node);
        break;

    case NODE_WHILE_LOOP:
        analyze_while_loop(context, node);
        break;

    case NODE_FUNCTION_DECLARATION:
        analyze_function_declaration(context, node);
        break;

    case NODE_RETURN_STATEMENT:
        if (node->return_stmt.value)
            get_expression_type(context, node->return_stmt.value);
        break;

    case NODE_BINARY_OP:
        get_expression_type(context, node);
        break;

    case NODE_UNARY_OP:
        get_expression_type(context, node);
        break;

    case NODE_FUNCTION_CALL:
        analyze_function_call(context, node);
        break;

    default:
        break;
    }

    return context->error_count == 0;
}

void free_semantic_context(SemanticContext *context)
{
    if (!context)
        return;

    free_symbol_table(context->symbol_table);

    for (int i = 0; i < context->error_count; i++)
    {
        free(context->errors[i].message);
    }
    free(context->errors);
    free(context);
}