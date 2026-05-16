#ifndef TAC_H
#define TAC_H

#include <stdbool.h>
#include "ast.h"

typedef enum
{
    TAC_ASSIGN,     // x = y
    TAC_ADD,        // x = y + z
    TAC_SUB,        // x = y - z
    TAC_MUL,        // x = y * z
    TAC_DIV,        // x = y / z
    TAC_MOD,        // x = y % z
    TAC_NEG,        // x = -y
    TAC_LABEL,      // L1:
    TAC_IF,         // if x goto L1
    TAC_GOTO,       // goto L1
    TAC_RETURN,     // return x
    TAC_FUNC_START, // function start
    TAC_FUNC_END,   // function end
    TAC_PARAM,      // param x
    TAC_CALL,       // x = call f, n
    TAC_ARG,        // arg x
    TAC_VAR,        // variable declaration
    TAC_ARRAY,      // array access
    TAC_LOAD,       // load from memory
    TAC_STORE,      // store to memory
    TAC_LESS,       // Less than comparison
    TAC_LESS_EQ,    // Less than or equal
    TAC_GREATER,    // Greater than
    TAC_GREATER_EQ, // Greater than or equal
    TAC_EQ,         // Equal
    TAC_NEQ         // Not equal
} TACOpType;

typedef struct TAC
{
    TACOpType op;
    char *result; 
    char *arg1;   
    char *arg2;   
    struct TAC *next;
    int line; 
} TAC;

TAC *tac_create(TACOpType op, char *result, char *arg1, char *arg2, int line);
void tac_free(TAC *tac);
TAC *tac_join(TAC *tac1, TAC *tac2);
TAC *ast_to_tac(Node *node);
TAC *generate_tac_for_expr(Node *node);
TAC *generate_tac_for_stmt(Node *node);
TAC *generate_tac_for_decl(Node *node);
char *generate_temp_var(void);
char *generate_label(void);
void reset_temp_counter(void);
void reset_label_counter(void);
void print_tac(TAC *tac);
void print_tac_list(TAC *tac);
void tac_free_list(TAC *head);
void test_tac_generation(void);

#endif