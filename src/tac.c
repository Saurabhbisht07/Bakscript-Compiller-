#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/tac.h"

static int temp_counter = 0;
static int label_counter = 0;

// Create a new TAC instruction
TAC *tac_create(TACOpType op, char *result, char *arg1, char *arg2, int line)
{
    TAC *tac = (TAC *)malloc(sizeof(TAC));
    if (!tac)
    {
        fprintf(stderr, "Error: Memory allocation failed for TAC\n");
        exit(1);
    }

    tac->op = op;
    tac->result = result ? strdup(result) : NULL;
    tac->arg1 = arg1 ? strdup(arg1) : NULL;
    tac->arg2 = arg2 ? strdup(arg2) : NULL;
    tac->next = NULL;
    tac->line = line;
    return tac;
}

// Free a TAC instruction
void tac_free(TAC *tac)
{
    if (!tac)
        return;

    free(tac->result);
    free(tac->arg1);
    free(tac->arg2);
    free(tac);
}

void tac_free_list(TAC *head)
{
    while (head)
    {
        TAC *next = head->next;
        free(head->result);
        free(head->arg1);
        free(head->arg2);
        free(head);
        head = next;
    }
}

// Join two TAC lists
TAC *tac_join(TAC *tac1, TAC *tac2)
{
    if (!tac1)
        return tac2;
    if (!tac2)
        return tac1;

    TAC *current = tac1;
    while (current->next)
    {
        current = current->next;
    }
    current->next = tac2;
    return tac1;
}

// Generate a unique temporary variable name
char *generate_temp_var(void)
{
    char *temp = (char *)malloc(64);
    if (!temp)
    {
        fprintf(stderr, "Error: Memory allocation failed for temp var\n");
        exit(1);
    }
    sprintf(temp, "t%d", temp_counter++);
    return temp;
}

// Generate a unique label
char *generate_label(void)
{
    char *label = (char *)malloc(32);
    if (!label)
    {
        fprintf(stderr, "Error: Memory allocation failed for label\n");
        exit(1);
    }
    sprintf(label, "L%d", label_counter++);
    return label;
}

void reset_temp_counter(void)
{
    temp_counter = 0;
}
void reset_label_counter(void)
{
    label_counter = 0;
}

void print_tac(TAC *tac)
{
    if (!tac)
        return;

    switch (tac->op)
    {
    case TAC_ASSIGN:
        printf("%s = %s\n", tac->result, tac->arg1);
        break;
    case TAC_ADD:
        printf("%s = %s + %s\n", tac->result, tac->arg1, tac->arg2);
        break;
    case TAC_SUB:
        printf("%s = %s - %s\n", tac->result, tac->arg1, tac->arg2);
        break;
    case TAC_MUL:
        printf("%s = %s * %s\n", tac->result, tac->arg1, tac->arg2);
        break;
    case TAC_DIV:
        printf("%s = %s / %s\n", tac->result, tac->arg1, tac->arg2);
        break;
    case TAC_MOD:
        printf("%s = %s %% %s\n", tac->result, tac->arg1, tac->arg2);
        break;
    case TAC_NEG:
        printf("%s = -%s\n", tac->result, tac->arg1);
        break;
    case TAC_LABEL:
        printf("%s:\n", tac->result);
        break;
    case TAC_IF:
        printf("if %s goto %s\n", tac->arg1, tac->result);
        break;
    case TAC_GOTO:
        printf("goto %s\n", tac->result);
        break;
    case TAC_RETURN:
        printf("return %s\n", tac->result);
        break;
    case TAC_FUNC_START:
        printf("function %s start\n", tac->result);
        break;
    case TAC_FUNC_END:
        printf("function %s end\n", tac->result);
        break;
    case TAC_PARAM:
        printf("param %s\n", tac->result);
        break;
    case TAC_CALL:
        printf("%s = call %s, %s\n", tac->result, tac->arg1, tac->arg2);
        break;
    case TAC_ARG:
        printf("arg %s\n", tac->result);
        break;
    case TAC_VAR:
        printf("var %s\n", tac->result);
        break;
    case TAC_ARRAY:
        printf("%s = %s[%s]\n", tac->result, tac->arg1, tac->arg2);
        break;
    case TAC_LOAD:
        printf("%s = *%s\n", tac->result, tac->arg1);
        break;
    case TAC_STORE:
        printf("*%s = %s\n", tac->result, tac->arg1);
        break;
    case TAC_LESS:
        printf("%s = %s < %s\n", tac->result, tac->arg1, tac->arg2);
        break;
    case TAC_GREATER:
        printf("%s = %s > %s\n", tac->result, tac->arg1, tac->arg2);
        break;
    case TAC_LESS_EQ:
        printf("%s = %s <= %s\n", tac->result, tac->arg1, tac->arg2);
        break;
    case TAC_GREATER_EQ:
        printf("%s = %s >= %s\n", tac->result, tac->arg1, tac->arg2);
        break;
    case TAC_EQ:
        printf("%s = %s == %s\n", tac->result, tac->arg1, tac->arg2);
        break;
    case TAC_NEQ:
        printf("%s = %s != %s\n", tac->result, tac->arg1, tac->arg2);
        break;
    default:
        printf("; (unknown TAC op %d)\n", (int)tac->op);
        break;
    }
}
void print_tac_list(TAC *tac)
{
    while (tac)
    {
        print_tac(tac);
        tac = tac->next;
    }
}
TAC *generate_tac_for_expr(Node *node)
{
    if (!node)
        return NULL;

    switch (node->type)
    {
    case NODE_NUMBER:
    {
        char value_str[32];
        sprintf(value_str, "%d", node->number.value);
        return tac_create(TAC_ASSIGN, generate_temp_var(), strdup(value_str), NULL, node->number.info.line);
    }

    case NODE_STRING:
    {
        char *quoted_str = (char *)malloc(strlen(node->string.value) + 3);
        if (!quoted_str)
        {
            fprintf(stderr, "Error: Memory allocation failed for quoted string\n");
            exit(1);
        }
        sprintf(quoted_str, "\"%s\"", node->string.value);
        TAC *result = tac_create(TAC_ASSIGN, generate_temp_var(), quoted_str, NULL, node->string.info.line);
        free(quoted_str);
        return result;
    }

    case NODE_BINARY_OP:
    {
        TAC *left = generate_tac_for_expr(node->binary_op.left);
        TAC *right = generate_tac_for_expr(node->binary_op.right);

        if (!left || !right)
            return NULL;

        TAC *last_left = left;
        while (last_left->next)
            last_left = last_left->next;

        TAC *last_right = right;
        while (last_right->next)
            last_right = last_right->next;

        TAC *result = NULL;
        switch (node->binary_op.op)
        {
        case OP_ADD:
        case OP_SUBTRACT:
        case OP_MULTIPLY:
        case OP_DIVIDE:
        case OP_LESS:
        case OP_GREATER:
        {
            char *temp = generate_temp_var();
            switch (node->binary_op.op)
            {
            case OP_ADD:
                result = tac_create(TAC_ADD, temp, last_left->result, last_right->result, node->binary_op.info.line);
                break;
            case OP_SUBTRACT:
                result = tac_create(TAC_SUB, temp, last_left->result, last_right->result, node->binary_op.info.line);
                break;
            case OP_MULTIPLY:
                result = tac_create(TAC_MUL, temp, last_left->result, last_right->result, node->binary_op.info.line);
                break;
            case OP_DIVIDE:
                result = tac_create(TAC_DIV, temp, last_left->result, last_right->result, node->binary_op.info.line);
                break;
            case OP_LESS:
                result = tac_create(TAC_LESS, temp, last_left->result, last_right->result, node->binary_op.info.line);
                break;
            case OP_GREATER:
                result = tac_create(TAC_GREATER, temp, last_left->result, last_right->result, node->binary_op.info.line);
                break;
            default:
                free(temp);
                return NULL;
            }
            break;
        }
        case OP_ASSIGN:
            if (node->binary_op.left->type == NODE_IDENTIFIER)
            {
                result = tac_create(TAC_ASSIGN, strdup(node->binary_op.left->identifier.name), last_right->result, NULL, node->binary_op.info.line);
            }
            break;
        default:
            return NULL;
        }

        TAC *joined = tac_join(left, right);
        if (result)
        {
            joined = tac_join(joined, result);
        }
        return joined;
    }

    case NODE_IDENTIFIER:
        return tac_create(TAC_ASSIGN, generate_temp_var(), strdup(node->identifier.name), NULL, node->identifier.info.line);

    default:
        return NULL;
    }
}

// Generate TAC for statements
TAC *generate_tac_for_stmt(Node *node)
{
    if (!node)
        return NULL;

    switch (node->type)
    {
    case NODE_VARIABLE_DECLARATION:
    {
        if (node->var_decl.initializer)
        {
            TAC *init = NULL;
            if (node->var_decl.initializer->type == NODE_FUNCTION_CALL)
            {
                init = generate_tac_for_stmt(node->var_decl.initializer);
            }
            else
            {
                init = generate_tac_for_expr(node->var_decl.initializer);
            }

            if (init)
            {
                TAC *last_tac = init;
                while (last_tac->next)
                {
                    last_tac = last_tac->next;
                }
                TAC *assign = tac_create(TAC_ASSIGN, strdup(node->var_decl.name), last_tac->result, NULL, node->var_decl.info.line);

                TAC *result = tac_join(init, assign);
                return result;
            }
        }
        return NULL;
    }

    case NODE_FOR_LOOP:
    {
        TAC *init = generate_tac_for_stmt(node->for_loop.initializer);
        if (!init)
            return NULL;

        char *start_label = generate_label();
        char *body_label = generate_label();
        char *end_label = generate_label();

        TAC *condition = generate_tac_for_expr(node->for_loop.condition);
        if (!condition)
        {
            free(start_label);
            free(body_label);
            free(end_label);
            return NULL;
        }

        TAC *last_tac = condition;
        while (last_tac->next)
        {
            last_tac = last_tac->next;
        }

        TAC *start_label_tac = tac_create(TAC_LABEL, start_label, NULL, NULL, node->for_loop.info.line);
        TAC *if_body = tac_create(TAC_IF, body_label, last_tac->result, NULL, node->for_loop.info.line);
        TAC *goto_end = tac_create(TAC_GOTO, end_label, NULL, NULL, node->for_loop.info.line);
        TAC *body_label_tac = tac_create(TAC_LABEL, body_label, NULL, NULL, node->for_loop.info.line);

        TAC *body = generate_tac_for_stmt(node->for_loop.body);
        if (!body)
        {
            tac_free(init);
            tac_free(condition);
            tac_free(start_label_tac);
            tac_free(if_body);
            tac_free(goto_end);
            tac_free(body_label_tac);
            free(start_label);
            free(body_label);
            free(end_label);
            return NULL;
        }

        TAC *increment = generate_tac_for_expr(node->for_loop.increment);
        if (!increment)
        {
            tac_free(init);
            tac_free(condition);
            tac_free(start_label_tac);
            tac_free(if_body);
            tac_free(goto_end);
            tac_free(body_label_tac);
            tac_free(body);
            free(start_label);
            free(body_label);
            free(end_label);
            return NULL;
        }

        TAC *goto_start = tac_create(TAC_GOTO, start_label, NULL, NULL, node->for_loop.info.line);
        TAC *end_label_tac = tac_create(TAC_LABEL, end_label, NULL, NULL, node->for_loop.info.line);

        TAC *result = init;
        result = tac_join(result, start_label_tac);
        result = tac_join(result, condition);
        result = tac_join(result, if_body);
        result = tac_join(result, goto_end);
        result = tac_join(result, body_label_tac);
        result = tac_join(result, body);
        result = tac_join(result, increment);
        result = tac_join(result, goto_start);
        result = tac_join(result, end_label_tac);
        return result;
    }

    case NODE_IF_STATEMENT:
    {
        TAC *condition = generate_tac_for_expr(node->if_stmt.condition);
        if (!condition)
            return NULL;

        char *true_label = generate_label();
        char *false_label = generate_label();
        char *end_label = generate_label();
        TAC *last_tac = condition;
        while (last_tac->next)
        {
            last_tac = last_tac->next;
        }

        TAC *if_tac = tac_create(TAC_IF, true_label, last_tac->result, NULL, node->if_stmt.info.line);
        TAC *goto_false = tac_create(TAC_GOTO, false_label, NULL, NULL, node->if_stmt.info.line);
        TAC *true_label_tac = tac_create(TAC_LABEL, true_label, NULL, NULL, node->if_stmt.info.line);
        TAC *body = generate_tac_for_stmt(node->if_stmt.if_body);
        if (!body)
        {
            tac_free(condition);
            tac_free(if_tac);
            tac_free(goto_false);
            tac_free(true_label_tac);
            free(true_label);
            free(false_label);
            free(end_label);
            return NULL;
        }

        TAC *goto_end = tac_create(TAC_GOTO, end_label, NULL, NULL, node->if_stmt.info.line);
        TAC *false_label_tac = tac_create(TAC_LABEL, false_label, NULL, NULL, node->if_stmt.info.line);
        TAC *else_body = NULL;
        if (node->if_stmt.else_body)
        {
            else_body = generate_tac_for_stmt(node->if_stmt.else_body);
            if (!else_body)
            {
                tac_free(condition);
                tac_free(if_tac);
                tac_free(goto_false);
                tac_free(true_label_tac);
                tac_free(body);
                tac_free(goto_end);
                tac_free(false_label_tac);
                free(true_label);
                free(false_label);
                free(end_label);
                return NULL;
            }
        }

        TAC *end_label_tac = tac_create(TAC_LABEL, end_label, NULL, NULL, node->if_stmt.info.line);
        TAC *result = condition;
        result = tac_join(result, if_tac);
        result = tac_join(result, goto_false);
        result = tac_join(result, true_label_tac);
        result = tac_join(result, body);
        result = tac_join(result, goto_end);
        result = tac_join(result, false_label_tac);
        if (else_body)
        {
            result = tac_join(result, else_body);
        }
        result = tac_join(result, end_label_tac);
        return result;
    }

    case NODE_BLOCK:
    case NODE_PROGRAM:
    {
        TAC *result = NULL;
        for (int i = 0; i < node->block.count; i++)
        {
            TAC *stmt = generate_tac_for_stmt(node->block.statements[i]);
            if (stmt)
            {
                if (!result)
                {
                    result = stmt;
                }
                else
                {
                    result = tac_join(result, stmt);
                }
            }
        }
        return result;
    }

    case NODE_FUNCTION_CALL:
    {
        TAC *args = NULL;
        TAC *last_arg = NULL;
        for (int i = 0; i < node->function_call.arg_count; i++)
        {
            TAC *arg = generate_tac_for_expr(node->function_call.arguments[i]);
            if (arg)
            {
                if (!args)
                {
                    args = arg;
                    last_arg = arg;
                }
                else
                {
                    TAC *new_last = tac_join(last_arg, arg);
                    if (new_last != last_arg)
                    {
                        tac_free(last_arg);
                    }
                    last_arg = new_last;
                }
            }
        }
        char *arg_result = NULL;
        if (last_arg)
        {
            TAC *temp = last_arg;
            while (temp->next)
            {
                temp = temp->next;
            }
            arg_result = temp->result;
        }
        if (strcmp(node->function_call.name, "show") == 0)
        {
            TAC *call = tac_create(TAC_CALL, NULL, strdup(node->function_call.name),
                                   arg_result, node->function_call.info.line);
            if (!call)
            {
                tac_free(args);
                return NULL;
            }
            return tac_join(args, call);
        }
        else if (strcmp(node->function_call.name, "ask") == 0)
        {
            char *temp = generate_temp_var();
            TAC *call = tac_create(TAC_CALL, temp, strdup(node->function_call.name),
                                   arg_result, node->function_call.info.line);
            if (!call)
            {
                free(temp);
                tac_free(args);
                return NULL;
            }
            return tac_join(args, call);
        }
        else
        {
            char *temp = generate_temp_var();
            TAC *call = tac_create(TAC_CALL, temp, strdup(node->function_call.name),
                                   arg_result, node->function_call.info.line);
            if (!call)
            {
                free(temp);
                tac_free(args);
                return NULL;
            }
            return tac_join(args, call);
        }
    }

    default:
        return generate_tac_for_expr(node);
    }
}

// Main AST to TAC conversion function
TAC *ast_to_tac(Node *node)
{
    if (!node)
        return NULL;

    reset_temp_counter();
    reset_label_counter();
    TAC *tac = generate_tac_for_stmt(node);
    return tac;
}

// Test function to demonstrate TAC generation and printing
void test_tac_generation(void)
{
    Node *num1 = create_number_node(5, 1, 1);
    Node *num2 = create_number_node(3, 1, 5);
    Node *add = create_binary_op_node(OP_ADD, num1, num2, 1, 3);
    Node *var = create_identifier_node("x", 1, 7);
    Node *assign = create_binary_op_node(OP_ASSIGN, var, add, 1, 1);
    TAC *tac = ast_to_tac(assign);
    printf("\nGenerated Three-Address Code:\n");
    printf("----------------------------\n");
    print_tac_list(tac);
    free_node(assign);
    tac_free_list(tac);
}