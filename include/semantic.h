#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdbool.h>
#include "ast.h"
#include "symbol_table.h"

typedef enum
{
    ERROR_UNDEFINED_VARIABLE,
    ERROR_DUPLICATE_VARIABLE,
    ERROR_TYPE_MISMATCH,
    ERROR_UNINITIALIZED_VARIABLE,
    ERROR_INVALID_OPERATION,
    ERROR_DIVISION_BY_ZERO
} SemanticErrorType;

typedef struct
{
    SemanticErrorType type;
    char *message;
    int line;
    int column;
} SemanticError;

typedef struct
{
    SymbolTable *symbol_table;
    SemanticError *errors;
    int error_count;
    int error_capacity;
    bool strict_types; // when true, enforce extended type rules (bool/array/object/auto)
} SemanticContext;

SemanticContext *create_semantic_context(bool strict_types);
void free_semantic_context(SemanticContext *context);
bool analyze_program(SemanticContext *context, Node *program);
DataType get_expression_type(SemanticContext *context, Node *expr);
void add_semantic_error(SemanticContext *context, SemanticErrorType type,
                        const char *message, int line, int column);
const char *get_error_type_string(SemanticErrorType type);
void semantic_print_errors(const SemanticContext *context);

#endif