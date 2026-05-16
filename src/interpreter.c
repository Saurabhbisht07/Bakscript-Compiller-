#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/interpreter.h"
#include "../include/lexer.h"
#include "../include/parser.h"

static Value value_clone(Value v);

// ------------------------------
// Internal helpers: hashing/env
// ------------------------------

static unsigned int env_hash(const char *str, int size)
{
    unsigned int h = 0;
    while (*str)
    {
        h = (h * 31u + (unsigned char)*str) % (unsigned int)size;
        str++;
    }
    return h;
}

Env *env_create(Env *parent)
{
    Env *env = (Env *)malloc(sizeof(Env));
    if (!env)
    {
        fprintf(stderr, "Interpreter: failed to allocate environment\n");
        exit(1);
    }
    env->bucket_count = 256;
    env->buckets = (EnvEntry **)calloc((size_t)env->bucket_count, sizeof(EnvEntry *));
    if (!env->buckets)
    {
        fprintf(stderr, "Interpreter: failed to allocate environment buckets\n");
        free(env);
        exit(1);
    }
    env->parent = parent;
    return env;
}

static void env_entry_free(EnvEntry *entry)
{
    while (entry)
    {
        EnvEntry *next = entry->next;
        free(entry->name);
        value_free(&entry->value);
        free(entry);
        entry = next;
    }
}

void env_free(Env *env)
{
    if (!env)
        return;
    for (int i = 0; i < env->bucket_count; i++)
    {
        env_entry_free(env->buckets[i]);
    }
    free(env->buckets);
    free(env);
}

bool env_define(Env *env, const char *name, Value value)
{
    if (!env || !name)
        return false;
    unsigned int idx = env_hash(name, env->bucket_count);
    EnvEntry *head = env->buckets[idx];
    for (EnvEntry *e = head; e; e = e->next)
    {
        if (strcmp(e->name, name) == 0)
        {
            // Shadowing in same env is treated as assignment; keeps semantics simple.
            value_free(&e->value);
            e->value = value_clone(value);
            return true;
        }
    }
    EnvEntry *entry = (EnvEntry *)malloc(sizeof(EnvEntry));
    if (!entry)
    {
        fprintf(stderr, "Interpreter: failed to allocate EnvEntry\n");
        exit(1);
    }
    entry->name = strdup(name);
    // Deep-copy into the environment so callers retain ownership.
    entry->value = value_clone(value);
    entry->next = head;
    env->buckets[idx] = entry;
    return true;
}

static EnvEntry *env_lookup_entry(Env *env, const char *name)
{
    for (Env *cur = env; cur; cur = cur->parent)
    {
        unsigned int idx = env_hash(name, cur->bucket_count);
        for (EnvEntry *e = cur->buckets[idx]; e; e = e->next)
        {
            if (strcmp(e->name, name) == 0)
            {
                return e;
            }
        }
    }
    return NULL;
}

bool env_assign(Env *env, const char *name, Value value, RuntimeError *err)
{
    EnvEntry *entry = env_lookup_entry(env, name);
    if (!entry)
    {
        if (err && !err->has_error)
        {
            err->has_error = true;
            err->line = 0;
            err->column = 0;
            char buf[256];
            snprintf(buf, sizeof(buf), "Runtime error: assignment to undefined variable '%s'", name);
            err->message = strdup(buf);
        }
        value_free(&value);
        return false;
    }

    value_free(&entry->value);

    // Deep-copy into the environment so callers retain ownership.
    entry->value = value_clone(value);
    return true;
}

bool env_get(Env *env, const char *name, Value *out, RuntimeError *err, int line, int column)
{
    EnvEntry *entry = env_lookup_entry(env, name);
    if (!entry)
    {
        if (err && !err->has_error)
        {
            err->has_error = true;
            err->line = line;
            err->column = column;
            char buf[256];
            snprintf(buf, sizeof(buf), "Runtime error: use of undefined variable '%s'", name);
            err->message = strdup(buf);
        }
        return false;
    }
    *out = entry->value;
    // Shallow copy; caller must not free the value itself.
    return true;
}

// ------------------------------
// Function table (user-defined)
// ------------------------------

typedef struct FunctionEntry
{
    Node *decl; // NODE_FUNCTION_DECLARATION
    struct FunctionEntry *next;
} FunctionEntry;

static FunctionEntry *g_functions = NULL;
static bool g_trace_enabled = false;
static long g_trace_step = 0;

static const char *node_type_name(NodeType t)
{
    switch (t)
    {
    case NODE_NUMBER: return "Number";
    case NODE_BOOL: return "Bool";
    case NODE_STRING: return "String";
    case NODE_IDENTIFIER: return "Identifier";
    case NODE_ARRAY_LITERAL: return "ArrayLiteral";
    case NODE_INDEX_ACCESS: return "IndexAccess";
    case NODE_BINARY_OP: return "BinaryOp";
    case NODE_UNARY_OP: return "UnaryOp";
    case NODE_FUNCTION_CALL: return "FunctionCall";
    case NODE_VARIABLE_DECLARATION: return "VarDecl";
    case NODE_IF_STATEMENT: return "IfStatement";
    case NODE_FOR_LOOP: return "ForLoop";
    case NODE_WHILE_LOOP: return "WhileLoop";
    case NODE_FUNCTION_DECLARATION: return "FunctionDecl";
    case NODE_RETURN_STATEMENT: return "Return";
    case NODE_BLOCK: return "Block";
    case NODE_PROGRAM: return "Program";
    default: return "UnknownNode";
    }
}

static void functions_clear(void)
{
    FunctionEntry *cur = g_functions;
    while (cur)
    {
        FunctionEntry *next = cur->next;
        free(cur);
        cur = next;
    }
    g_functions = NULL;
}

static void functions_register(Node *decl)
{
    if (!decl || decl->type != NODE_FUNCTION_DECLARATION)
        return;
    const char *name = decl->function_decl.name;

    for (FunctionEntry *cur = g_functions; cur; cur = cur->next)
    {
        if (strcmp(cur->decl->function_decl.name, name) == 0)
        {
            cur->decl = decl;
            return;
        }
    }

    FunctionEntry *entry = (FunctionEntry *)malloc(sizeof(FunctionEntry));
    if (!entry)
    {
        fprintf(stderr, "Interpreter: failed to allocate FunctionEntry\n");
        exit(1);
    }
    entry->decl = decl;
    entry->next = g_functions;
    g_functions = entry;
}

static Node *functions_lookup(const char *name)
{
    for (FunctionEntry *cur = g_functions; cur; cur = cur->next)
    {
        if (strcmp(cur->decl->function_decl.name, name) == 0)
            return cur->decl;
    }
    return NULL;
}

// ------------------------------
// Value helpers
// ------------------------------

Value value_num(long n)
{
    Value v;
    v.type = VAL_NUM;
    v.as.num = n;
    return v;
}

Value value_str_owned(char *s)
{
    Value v;
    v.type = VAL_STR;
    v.as.str = s;
    return v;
}

Value value_str_copy(const char *s)
{
    if (!s)
        return value_nil();
    char *copy = strdup(s);
    if (!copy)
    {
        fprintf(stderr, "Interpreter: failed to allocate string value\n");
        exit(1);
    }
    return value_str_owned(copy);
}

Value value_bool(bool b)
{
    Value v;
    v.type = VAL_BOOL;
    v.as.boolean = b ? 1 : 0;
    return v;
}

Value value_nil(void)
{
    Value v;
    v.type = VAL_NIL;
    v.as.num = 0;
    return v;
}

static Value value_clone(Value v)
{
    switch (v.type)
    {
    case VAL_STR:
        return value_str_copy(v.as.str ? v.as.str : "");
    case VAL_ARRAY:
    {
        RuntimeArray *src = v.as.array;
        RuntimeArray *arr = (RuntimeArray *)malloc(sizeof(RuntimeArray));
        if (!arr)
        {
            fprintf(stderr, "Interpreter: failed to allocate array clone\n");
            exit(1);
        }
        arr->count = src ? src->count : 0;
        if (arr->count > 0)
        {
            arr->items = (Value *)malloc(sizeof(Value) * (size_t)arr->count);
            if (!arr->items)
            {
                free(arr);
                fprintf(stderr, "Interpreter: failed to allocate array clone items\n");
                exit(1);
            }
            for (int i = 0; i < arr->count; i++)
            {
                arr->items[i] = value_clone(src->items[i]);
            }
        }
        else
        {
            arr->items = NULL;
        }
        Value out;
        out.type = VAL_ARRAY;
        out.as.array = arr;
        return out;
    }
    case VAL_NUM:
        return value_num(v.as.num);
    case VAL_BOOL:
        return value_bool(v.as.boolean != 0);
    case VAL_NIL:
    default:
        return value_nil();
    }
}

void value_free(Value *v)
{
    if (!v)
        return;
    if (v->type == VAL_STR && v->as.str)
    {
        free(v->as.str);
    }
    else if (v->type == VAL_ARRAY && v->as.array)
    {
        for (int i = 0; i < v->as.array->count; i++)
        {
            value_free(&v->as.array->items[i]);
        }
        free(v->as.array->items);
        free(v->as.array);
    }
    v->type = VAL_NIL;
    v->as.num = 0;
}

// ------------------------------
// Expression evaluation
// ------------------------------

static Value eval_expr(Env *env, Node *expr, RuntimeError *err);
static void print_value(Value v);

static long as_long(Value v)
{
    if (v.type == VAL_NUM)
        return v.as.num;
    if (v.type == VAL_BOOL)
        return v.as.boolean ? 1 : 0;
    return 0;
}

static void print_value(Value v)
{
    switch (v.type)
    {
    case VAL_STR:
        printf("%s", v.as.str ? v.as.str : "");
        break;
    case VAL_NUM:
        printf("%ld", v.as.num);
        break;
    case VAL_BOOL:
        printf("%s", v.as.boolean ? "true" : "false");
        break;
    case VAL_ARRAY:
        printf("[");
        if (v.as.array)
        {
            for (int i = 0; i < v.as.array->count; i++)
            {
                if (i > 0)
                    printf(", ");
                print_value(v.as.array->items[i]);
            }
        }
        printf("]");
        break;
    case VAL_NIL:
    default:
        printf("nil");
        break;
    }
}

static void print_env_state(Env *env)
{
    printf("state { ");
    int printed = 0;
    if (env)
    {
        for (int i = 0; i < env->bucket_count; i++)
        {
            for (EnvEntry *e = env->buckets[i]; e; e = e->next)
            {
                if (printed > 0)
                    printf(", ");
                printf("%s=", e->name);
                print_value(e->value);
                printed++;
            }
        }
    }
    printf(" }\n");
}

static void trace_before(Node *node)
{
    if (!g_trace_enabled || !node)
        return;
    g_trace_step++;
    printf("[TRACE %ld] %s\n", g_trace_step, node_type_name(node->type));
}

static void trace_after(Node *node, Env *env, int success)
{
    if (!g_trace_enabled || !node)
        return;
    printf("[TRACE %ld] %s -> %s\n", g_trace_step, node_type_name(node->type), success ? "ok" : "error");
    print_env_state(env);
}

static bool as_bool(Value v)
{
    switch (v.type)
    {
    case VAL_BOOL:
        return v.as.boolean != 0;
    case VAL_NUM:
        return v.as.num != 0;
    case VAL_STR:
        return v.as.str && v.as.str[0] != '\0';
    case VAL_ARRAY:
        return v.as.array && v.as.array->count > 0;
    case VAL_NIL:
    default:
        return false;
    }
}

static Value eval_binary_op(Env *env, Node *node, RuntimeError *err)
{
    switch (node->binary_op.op)
    {
    case OP_ASSIGN:
    {
        // Assignment evaluates only the RHS.
        Value right = eval_expr(env, node->binary_op.right, err);
        if (err && err->has_error)
            return value_nil();

        if (node->binary_op.left->type == NODE_IDENTIFIER)
        {
            // env_assign takes ownership of `right` (including heap memory).
            if (!env_assign(env, node->binary_op.left->identifier.name, right, err))
                return value_nil();
            return right;
        }
        else if (node->binary_op.left->type == NODE_INDEX_ACCESS)
        {
            Node *index_left = node->binary_op.left;
            if (index_left->index_access.array->type != NODE_IDENTIFIER)
            {
                if (err && !err->has_error)
                {
                    err->has_error = true;
                    err->line = index_left->index_access.info.line;
                    err->column = index_left->index_access.info.column;
                    err->message = strdup("Runtime error: indexed assignment requires an array variable on the left side");
                }
                value_free(&right);
                return value_nil();
            }

            const char *arr_name = index_left->index_access.array->identifier.name;
            EnvEntry *entry = env_lookup_entry(env, arr_name);
            if (!entry)
            {
                if (err && !err->has_error)
                {
                    err->has_error = true;
                    err->line = index_left->index_access.info.line;
                    err->column = index_left->index_access.info.column;
                    err->message = strdup("Runtime error: assignment to index of undefined variable");
                }
                value_free(&right);
                return value_nil();
            }
            if (entry->value.type != VAL_ARRAY || !entry->value.as.array)
            {
                if (err && !err->has_error)
                {
                    err->has_error = true;
                    err->line = index_left->index_access.info.line;
                    err->column = index_left->index_access.info.column;
                    err->message = strdup("Runtime error: indexed assignment requires array value");
                }
                value_free(&right);
                return value_nil();
            }

            Value idx_val = eval_expr(env, index_left->index_access.index, err);
            if (err && err->has_error)
            {
                value_free(&right);
                return value_nil();
            }
            if (idx_val.type != VAL_NUM)
            {
                if (err && !err->has_error)
                {
                    err->has_error = true;
                    err->line = index_left->index_access.info.line;
                    err->column = index_left->index_access.info.column;
                    err->message = strdup("Runtime error: array index must be numeric");
                }
                value_free(&idx_val);
                value_free(&right);
                return value_nil();
            }

            long idx = idx_val.as.num;
            value_free(&idx_val);
            if (idx < 0 || idx >= entry->value.as.array->count)
            {
                if (err && !err->has_error)
                {
                    err->has_error = true;
                    err->line = index_left->index_access.info.line;
                    err->column = index_left->index_access.info.column;
                    err->message = strdup("Runtime error: array index out of bounds");
                }
                value_free(&right);
                return value_nil();
            }

            value_free(&entry->value.as.array->items[idx]);
            entry->value.as.array->items[idx] = value_clone(right);
            return right;
        }

        if (err && !err->has_error)
        {
            err->has_error = true;
            err->line = node->binary_op.info.line;
            err->column = node->binary_op.info.column;
            err->message = strdup("Runtime error: invalid assignment target");
        }
        value_free(&right);
        return value_nil();
    }

    case OP_AND:
    {
        // Short-circuit: evaluate left, and only if truthy evaluate right.
        Value left = eval_expr(env, node->binary_op.left, err);
        if (err && err->has_error)
            return value_nil();
        if (!as_bool(left))
        {
            value_free(&left);
            return value_bool(false);
        }
        Value right = eval_expr(env, node->binary_op.right, err);
        if (err && err->has_error)
        {
            value_free(&left);
            return value_nil();
        }
        bool result = as_bool(right);
        value_free(&left);
        value_free(&right);
        return value_bool(result);
    }

    case OP_OR:
    {
        Value left = eval_expr(env, node->binary_op.left, err);
        if (err && err->has_error)
            return value_nil();
        if (as_bool(left))
        {
            value_free(&left);
            return value_bool(true);
        }
        Value right = eval_expr(env, node->binary_op.right, err);
        if (err && err->has_error)
        {
            value_free(&left);
            return value_nil();
        }
        bool result = as_bool(right);
        value_free(&left);
        value_free(&right);
        return value_bool(result);
    }

    case OP_ADD:
    case OP_SUBTRACT:
    case OP_MULTIPLY:
    case OP_DIVIDE:
    {
        Value left = eval_expr(env, node->binary_op.left, err);
        if (err && err->has_error)
            return value_nil();
        Value right = eval_expr(env, node->binary_op.right, err);
        if (err && err->has_error)
        {
            value_free(&left);
            return value_nil();
        }

        if (node->binary_op.op == OP_ADD && (left.type == VAL_STR || right.type == VAL_STR))
        {
            // String concatenation: allow string with num/bool by converting other side.
            const char *ls = NULL;
            const char *rs = NULL;
            char num_buf[64];
            char *tmp_ls = NULL;
            char *tmp_rs = NULL;

            if (left.type == VAL_STR)
                ls = left.as.str;
            else if (left.type == VAL_NUM)
            {
                snprintf(num_buf, sizeof(num_buf), "%ld", left.as.num);
                tmp_ls = strdup(num_buf);
                ls = tmp_ls;
            }
            else if (left.type == VAL_BOOL)
            {
                tmp_ls = strdup(left.as.boolean ? "true" : "false");
                ls = tmp_ls;
            }
            else
            {
                tmp_ls = strdup("nil");
                ls = tmp_ls;
            }

            if (right.type == VAL_STR)
                rs = right.as.str;
            else if (right.type == VAL_NUM)
            {
                snprintf(num_buf, sizeof(num_buf), "%ld", right.as.num);
                tmp_rs = strdup(num_buf);
                rs = tmp_rs;
            }
            else if (right.type == VAL_BOOL)
            {
                tmp_rs = strdup(right.as.boolean ? "true" : "false");
                rs = tmp_rs;
            }
            else
            {
                tmp_rs = strdup("nil");
                rs = tmp_rs;
            }

            size_t out_len = (ls ? strlen(ls) : 0) + (rs ? strlen(rs) : 0) + 1;
            char *out = (char *)malloc(out_len);
            if (!out)
            {
                fprintf(stderr, "Runtime: out-of-memory in string concatenation\n");
                exit(1);
            }
            out[0] = '\0';
            if (ls)
                strcat(out, ls);
            if (rs)
                strcat(out, rs);

            if (tmp_ls)
                free(tmp_ls);
            if (tmp_rs)
                free(tmp_rs);
            value_free(&left);
            value_free(&right);
            return value_str_owned(out);
        }

        // For numeric arithmetic, require numeric operands.
        if (left.type != VAL_NUM || right.type != VAL_NUM)
        {
            if (err && !err->has_error)
            {
                err->has_error = true;
                err->line = node->binary_op.info.line;
                err->column = node->binary_op.info.column;
                err->message = strdup("Runtime error: arithmetic operators require numeric operands");
            }
            value_free(&left);
            value_free(&right);
            return value_nil();
        }

        long a = left.as.num;
        long b = right.as.num;
        long result_num = 0;
        switch (node->binary_op.op)
        {
        case OP_ADD:
            result_num = a + b;
            break;
        case OP_SUBTRACT:
            result_num = a - b;
            break;
        case OP_MULTIPLY:
            result_num = a * b;
            break;
        case OP_DIVIDE:
            if (b == 0)
            {
                if (err && !err->has_error)
                {
                    err->has_error = true;
                    err->line = node->binary_op.info.line;
                    err->column = node->binary_op.info.column;
                    err->message = strdup("Runtime error: division by zero");
                }
                value_free(&left);
                value_free(&right);
                return value_nil();
            }
            result_num = a / b;
            break;
        default:
            break;
        }
        value_free(&left);
        value_free(&right);
        return value_num(result_num);
    }

    case OP_LESS:
    case OP_GREATER:
    case OP_LESS_EQ:
    case OP_GREATER_EQ:
    {
        Value left = eval_expr(env, node->binary_op.left, err);
        if (err && err->has_error)
            return value_nil();
        Value right = eval_expr(env, node->binary_op.right, err);
        if (err && err->has_error)
        {
            value_free(&left);
            return value_nil();
        }

        if (left.type != VAL_NUM || right.type != VAL_NUM)
        {
            if (err && !err->has_error)
            {
                err->has_error = true;
                err->line = node->binary_op.info.line;
                err->column = node->binary_op.info.column;
                err->message = strdup("Runtime error: relational operators require numeric operands");
            }
            value_free(&left);
            value_free(&right);
            return value_nil();
        }

        long a = left.as.num;
        long b = right.as.num;
        bool result = false;
        switch (node->binary_op.op)
        {
        case OP_LESS:
            result = a < b;
            break;
        case OP_GREATER:
            result = a > b;
            break;
        case OP_LESS_EQ:
            result = a <= b;
            break;
        case OP_GREATER_EQ:
            result = a >= b;
            break;
        default:
            break;
        }
        value_free(&left);
        value_free(&right);
        return value_bool(result);
    }

    case OP_EQUAL:
    case OP_NOT_EQUAL:
    {
        Value left = eval_expr(env, node->binary_op.left, err);
        if (err && err->has_error)
            return value_nil();
        Value right = eval_expr(env, node->binary_op.right, err);
        if (err && err->has_error)
        {
            value_free(&left);
            return value_nil();
        }

        bool result = false;
        if (left.type != right.type)
        {
            if (err && !err->has_error)
            {
                err->has_error = true;
                err->line = node->binary_op.info.line;
                err->column = node->binary_op.info.column;
                err->message = strdup("Runtime error: equality operators require operands of the same type");
            }
            value_free(&left);
            value_free(&right);
            return value_nil();
        }

        switch (left.type)
        {
        case VAL_NUM:
            result = (left.as.num == right.as.num);
            break;
        case VAL_STR:
            result = (strcmp(left.as.str ? left.as.str : "", right.as.str ? right.as.str : "") == 0);
            break;
        case VAL_BOOL:
            result = (left.as.boolean == right.as.boolean);
            break;
        case VAL_NIL:
            result = true;
            break;
        default:
            result = false;
            break;
        }

        value_free(&left);
        value_free(&right);
        if (node->binary_op.op == OP_NOT_EQUAL)
            result = !result;
        return value_bool(result);
    }

    default:
        return value_nil();
    }
}

// Execute inside a function and propagate `return`.
// `did_return` becomes true when a return statement is executed; `out_return`
// holds the returned value (caller owns it).
static bool exec_node_with_return(Env *env, Node *node, RuntimeError *err, bool *did_return, Value *out_return);

static Value eval_function_call(Env *env, Node *node, RuntimeError *err)
{
    // Currently we only support built-ins: show(expr) and ask(prompt).
    const char *name = node->function_call.name;
    if (strcmp(name, "show") == 0)
    {
        if (node->function_call.arg_count != 1)
        {
            if (err && !err->has_error)
            {
                err->has_error = true;
                err->line = node->function_call.info.line;
                err->column = node->function_call.info.column;
                err->message = strdup("Runtime error: show() expects exactly one argument");
            }
            return value_nil();
        }
        Value v = eval_expr(env, node->function_call.arguments[0], err);
        if (err && err->has_error)
            return value_nil();
        print_value(v);
        printf("\n");
        // `v` ownership belongs to eval_expr; free string temps after printing.
        value_free(&v);
        return value_nil();
    }
    else if (strcmp(name, "ask") == 0)
    {
        if (node->function_call.arg_count != 1)
        {
            if (err && !err->has_error)
            {
                err->has_error = true;
                err->line = node->function_call.info.line;
                err->column = node->function_call.info.column;
                err->message = strdup("Runtime error: ask() expects exactly one argument");
            }
            return value_nil();
        }
        Value prompt = eval_expr(env, node->function_call.arguments[0], err);
        if (err && err->has_error)
            return value_nil();
        if (prompt.type == VAL_STR && prompt.as.str)
        {
            printf("%s", prompt.as.str);
            fflush(stdout);
        }
        // Prompt is an evaluation result; free any string temp.
        value_free(&prompt);
        char buffer[1024];
        if (!fgets(buffer, sizeof(buffer), stdin))
        {
            return value_str_copy("");
        }
        size_t len = strlen(buffer);
        if (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
        {
            buffer[len - 1] = '\0';
        }
        return value_str_copy(buffer);
    }

    // User-defined functions
    Node *fn_decl = functions_lookup(name);
    if (fn_decl)
    {
        int expected = fn_decl->function_decl.param_count;
        if (node->function_call.arg_count != expected)
        {
            if (err && !err->has_error)
            {
                err->has_error = true;
                err->line = node->function_call.info.line;
                err->column = node->function_call.info.column;
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Runtime error: function '%s' expects %d args but got %d",
                         name, expected, node->function_call.arg_count);
                err->message = strdup(buf);
            }
            return value_nil();
        }

        Env *call_env = env_create(env);

        for (int i = 0; i < expected; i++)
        {
            Value arg = eval_expr(env, node->function_call.arguments[i], err);
            if (err && err->has_error)
            {
                value_free(&arg);
                env_free(call_env);
                return value_nil();
            }

            env_define(call_env, fn_decl->function_decl.params[i], arg);
            // env_define deep-copies; free our temporary.
            value_free(&arg);
        }

        bool did_return = false;
        Value ret = value_nil();
        bool ok = exec_node_with_return(call_env, fn_decl->function_decl.body, err,
                                          &did_return, &ret);
        env_free(call_env);

        if (!ok)
        {
            if (did_return)
                value_free(&ret);
            return value_nil();
        }

        if (!did_return)
            return value_nil();

        return ret; // transfer ownership to caller
    }

    if (err && !err->has_error)
    {
        err->has_error = true;
        err->line = node->function_call.info.line;
        err->column = node->function_call.info.column;
        char buf[256];
        snprintf(buf, sizeof(buf), "Runtime error: unknown function '%s'", name);
        err->message = strdup(buf);
    }
    return value_nil();
}

static Value eval_expr(Env *env, Node *expr, RuntimeError *err)
{
    if (!expr)
        return value_nil();

    switch (expr->type)
    {
    case NODE_NUMBER:
        return value_num(expr->number.value);
    case NODE_BOOL:
        return value_bool(expr->boolean.value != 0);
    case NODE_STRING:
        return value_str_copy(expr->string.value);
    case NODE_ARRAY_LITERAL:
    {
        RuntimeArray *arr = (RuntimeArray *)malloc(sizeof(RuntimeArray));
        if (!arr)
        {
            fprintf(stderr, "Interpreter: failed to allocate array value\n");
            exit(1);
        }
        arr->count = expr->array_literal.count;
        if (arr->count > 0)
        {
            arr->items = (Value *)malloc(sizeof(Value) * (size_t)arr->count);
            if (!arr->items)
            {
                free(arr);
                fprintf(stderr, "Interpreter: failed to allocate array elements\n");
                exit(1);
            }
            for (int i = 0; i < arr->count; i++)
            {
                arr->items[i] = eval_expr(env, expr->array_literal.elements[i], err);
                if (err && err->has_error)
                {
                    for (int j = 0; j <= i; j++)
                        value_free(&arr->items[j]);
                    free(arr->items);
                    free(arr);
                    return value_nil();
                }
            }
        }
        else
        {
            arr->items = NULL;
        }
        Value out;
        out.type = VAL_ARRAY;
        out.as.array = arr;
        return out;
    }
    case NODE_INDEX_ACCESS:
    {
        Value arr_v = eval_expr(env, expr->index_access.array, err);
        if (err && err->has_error)
            return value_nil();
        if (arr_v.type != VAL_ARRAY || !arr_v.as.array)
        {
            if (err && !err->has_error)
            {
                err->has_error = true;
                err->line = expr->index_access.info.line;
                err->column = expr->index_access.info.column;
                err->message = strdup("Runtime error: index access requires array value");
            }
            value_free(&arr_v);
            return value_nil();
        }
        Value idx_v = eval_expr(env, expr->index_access.index, err);
        if (err && err->has_error)
        {
            value_free(&arr_v);
            return value_nil();
        }
        if (idx_v.type != VAL_NUM)
        {
            if (err && !err->has_error)
            {
                err->has_error = true;
                err->line = expr->index_access.info.line;
                err->column = expr->index_access.info.column;
                err->message = strdup("Runtime error: array index must be numeric");
            }
            value_free(&idx_v);
            value_free(&arr_v);
            return value_nil();
        }
        long idx = idx_v.as.num;
        value_free(&idx_v);
        if (idx < 0 || idx >= arr_v.as.array->count)
        {
            if (err && !err->has_error)
            {
                err->has_error = true;
                err->line = expr->index_access.info.line;
                err->column = expr->index_access.info.column;
                err->message = strdup("Runtime error: array index out of bounds");
            }
            value_free(&arr_v);
            return value_nil();
        }
        Value out = value_clone(arr_v.as.array->items[idx]);
        value_free(&arr_v);
        return out;
    }
    case NODE_IDENTIFIER:
    {
        Value v;
        if (!env_get(env, expr->identifier.name, &v, err,
                     expr->identifier.info.line, expr->identifier.info.column))
        {
            return value_nil();
        }
        // Return a deep copy so caller can safely own/finalize value.
        return value_clone(v);
    }
    case NODE_BINARY_OP:
        return eval_binary_op(env, expr, err);
    case NODE_UNARY_OP:
        if (expr->unary_op.op == UOP_NOT)
        {
            Value v = eval_expr(env, expr->unary_op.operand, err);
            if (err && err->has_error)
                return value_nil();
            bool result = !as_bool(v);
            value_free(&v);
            return value_bool(result);
        }
        return value_nil();
    case NODE_FUNCTION_CALL:
        return eval_function_call(env, expr, err);
    default:
        return value_nil();
    }
}

// ------------------------------
// Statement execution
// ------------------------------

static bool exec_node(Env *env, Node *node, RuntimeError *err);

static bool exec_block(Env *parent_env, Node *block_node, RuntimeError *err)
{
    Env *local = env_create(parent_env);
    for (int i = 0; i < block_node->block.count; i++)
    {
        if (!exec_node(local, block_node->block.statements[i], err))
        {
            env_free(local);
            return false;
        }
    }
    env_free(local);
    return true;
}

static bool exec_if(Env *env, Node *node, RuntimeError *err)
{
    Value cond = eval_expr(env, node->if_stmt.condition, err);
    if (err && err->has_error)
        return false;
    bool truthy = as_bool(cond);
    value_free(&cond);
    if (truthy)
    {
        return exec_block(env, node->if_stmt.if_body, err);
    }
    else if (node->if_stmt.else_body)
    {
        return exec_block(env, node->if_stmt.else_body, err);
    }
    return true;
}

static bool exec_for(Env *env, Node *node, RuntimeError *err)
{
    Env *loop_env = env_create(env);

    if (node->for_loop.initializer)
    {
        if (!exec_node(loop_env, node->for_loop.initializer, err))
        {
            env_free(loop_env);
            return false;
        }
    }

    while (1)
    {
        if (node->for_loop.condition)
        {
            Value cond = eval_expr(loop_env, node->for_loop.condition, err);
            if (err && err->has_error)
            {
                env_free(loop_env);
                return false;
            }
            bool truthy = as_bool(cond);
            value_free(&cond);
            if (!truthy)
                break;
        }

        if (!exec_node(loop_env, node->for_loop.body, err))
        {
            env_free(loop_env);
            return false;
        }

        if (node->for_loop.increment)
        {
            Value inc = eval_expr(loop_env, node->for_loop.increment, err);
            value_free(&inc);
            if (err && err->has_error)
            {
                env_free(loop_env);
                return false;
            }
        }
    }

    env_free(loop_env);
    return true;
}

static bool exec_while(Env *env, Node *node, RuntimeError *err)
{
    while (1)
    {
        Value cond = eval_expr(env, node->while_loop.condition, err);
        if (err && err->has_error)
        {
            value_free(&cond);
            return false;
        }

        bool truthy = as_bool(cond);
        value_free(&cond);

        if (!truthy)
            break;

        if (!exec_block(env, node->while_loop.body, err))
            return false;
    }
    return true;
}

static bool exec_var_decl(Env *env, Node *node, RuntimeError *err)
{
    Value initial = value_nil();
    if (node->var_decl.initializer)
    {
        initial = eval_expr(env, node->var_decl.initializer, err);
        if (err && err->has_error)
            return false;
    }
    // For now, rely on semantic analysis for type correctness; interpreter
    // simply stores the evaluated value.
    if (!env_define(env, node->var_decl.name, initial))
    {
        if (err && !err->has_error)
        {
            err->has_error = true;
            err->line = node->var_decl.info.line;
            err->column = node->var_decl.info.column;
            char buf[256];
            snprintf(buf, sizeof(buf), "Runtime error: failed to declare variable '%s'", node->var_decl.name);
            err->message = strdup(buf);
        }
        value_free(&initial);
        return false;
    }
    // Environment stores a deep copy, so we can free our temporary.
    value_free(&initial);
    return true;
}

static bool exec_node_with_return(Env *env, Node *node, RuntimeError *err, bool *did_return, Value *out_return)
{
    if (!node)
        return true;
    if (did_return && *did_return)
        return true;

    switch (node->type)
    {
    case NODE_VARIABLE_DECLARATION:
        return exec_var_decl(env, node, err);

    case NODE_IF_STATEMENT:
    {
        Value cond = eval_expr(env, node->if_stmt.condition, err);
        if (err && err->has_error)
        {
            value_free(&cond);
            return false;
        }
        bool truthy = as_bool(cond);
        value_free(&cond);

        if (truthy)
            return exec_node_with_return(env, node->if_stmt.if_body, err, did_return, out_return);
        if (node->if_stmt.else_body)
            return exec_node_with_return(env, node->if_stmt.else_body, err, did_return, out_return);
        return true;
    }

    case NODE_FOR_LOOP:
    {
        Env *loop_env = env_create(env);

        if (node->for_loop.initializer)
        {
            if (!exec_node_with_return(loop_env, node->for_loop.initializer, err, did_return, out_return))
            {
                env_free(loop_env);
                return false;
            }
        }

        while (1)
        {
            if (node->for_loop.condition)
            {
                Value cond = eval_expr(loop_env, node->for_loop.condition, err);
                if (err && err->has_error)
                {
                    value_free(&cond);
                    env_free(loop_env);
                    return false;
                }
                bool truthy = as_bool(cond);
                value_free(&cond);
                if (!truthy)
                    break;
            }

            if (!exec_node_with_return(loop_env, node->for_loop.body, err, did_return, out_return))
            {
                env_free(loop_env);
                return false;
            }
            if (did_return && *did_return)
            {
                env_free(loop_env);
                return true;
            }

            if (node->for_loop.increment)
            {
                Value inc = eval_expr(loop_env, node->for_loop.increment, err);
                value_free(&inc);
                if (err && err->has_error)
                {
                    env_free(loop_env);
                    return false;
                }
            }
        }

        env_free(loop_env);
        return true;
    }

    case NODE_WHILE_LOOP:
    {
        while (1)
        {
            Value cond = eval_expr(env, node->while_loop.condition, err);
            if (err && err->has_error)
            {
                value_free(&cond);
                return false;
            }
            bool truthy = as_bool(cond);
            value_free(&cond);

            if (!truthy)
                break;

            if (!exec_node_with_return(env, node->while_loop.body, err, did_return, out_return))
                return false;
            if (did_return && *did_return)
                return true;
        }
        return true;
    }

    case NODE_BLOCK:
    {
        Env *local = env_create(env);
        for (int i = 0; i < node->block.count; i++)
        {
            if (did_return && *did_return)
                break;
            if (!exec_node_with_return(local, node->block.statements[i], err, did_return, out_return))
            {
                env_free(local);
                return false;
            }
        }
        env_free(local);
        return true;
    }

    case NODE_FUNCTION_DECLARATION:
        functions_register(node);
        return true;

    case NODE_RETURN_STATEMENT:
    {
        Value v = eval_expr(env, node->return_stmt.value, err);
        if (err && err->has_error)
        {
            value_free(&v);
            return false;
        }
        if (did_return)
            *did_return = true;
        if (out_return)
            *out_return = v; // transfer ownership
        return true;
    }

    case NODE_FUNCTION_CALL:
    {
        Value ret = eval_function_call(env, node, err);
        value_free(&ret);
        return !(err && err->has_error);
    }

    case NODE_BINARY_OP:
    {
        Value ret = eval_binary_op(env, node, err);
        value_free(&ret);
        return !(err && err->has_error);
    }

    default:
    {
        Value ret = eval_expr(env, node, err);
        value_free(&ret);
        return !(err && err->has_error);
    }
    }
}

static bool exec_node(Env *env, Node *node, RuntimeError *err)
{
    if (!node)
        return true;

    trace_before(node);
    bool ok = true;

    switch (node->type)
    {
    case NODE_VARIABLE_DECLARATION:
        ok = exec_var_decl(env, node, err);
        break;
    case NODE_IF_STATEMENT:
        ok = exec_if(env, node, err);
        break;
    case NODE_FOR_LOOP:
        ok = exec_for(env, node, err);
        break;
    case NODE_WHILE_LOOP:
        ok = exec_while(env, node, err);
        break;
    case NODE_FUNCTION_DECLARATION:
        functions_register(node);
        ok = true;
        break;
    case NODE_RETURN_STATEMENT:
        if (err && !err->has_error)
        {
            err->has_error = true;
            err->line = node->return_stmt.info.line;
            err->column = node->return_stmt.info.column;
            err->message = strdup("Runtime error: return outside function");
        }
        ok = false;
        break;
    case NODE_BLOCK:
        ok = exec_block(env, node, err);
        break;
    case NODE_FUNCTION_CALL:
    {
        Value ret = eval_function_call(env, node, err);
        // If the call produced a string (e.g., ask()), it is owned by us.
        value_free(&ret);
        ok = !(err && err->has_error);
        break;
    }
    case NODE_BINARY_OP:
    {
        Value ret = eval_binary_op(env, node, err);
        value_free(&ret);
        ok = !(err && err->has_error);
        break;
    }
    default:
        // Expression as statement: evaluate and ignore result
    {
        Value ret = eval_expr(env, node, err);
        value_free(&ret);
        ok = !(err && err->has_error);
    }
        break;
    }

    trace_after(node, env, ok);
    return ok;
}

// ------------------------------
// Public entrypoints
// ------------------------------

bool interpret_program_with_trace(Node *program, RuntimeError *err, bool trace_enabled)
{
    functions_clear();
    g_trace_enabled = trace_enabled;
    g_trace_step = 0;

    if (err)
    {
        err->has_error = false;
        err->message = NULL;
        err->line = 0;
        err->column = 0;
    }
    if (!program)
        return true;

    Env *global = env_create(NULL);
    bool ok = true;

    if (program->type == NODE_PROGRAM)
    {
        for (int i = 0; i < program->program.count; i++)
        {
            if (!exec_node(global, program->program.statements[i], err))
            {
                ok = false;
                break;
            }
        }
    }
    else
    {
        ok = exec_node(global, program, err);
    }

    env_free(global);

    g_trace_enabled = false;
    functions_clear();
    return ok;
}

bool interpret_program(Node *program, RuntimeError *err)
{
    return interpret_program_with_trace(program, err, false);
}

// Simple REPL using existing lexer/parser frontend.
void repl_start(void)
{
    printf("BakScript REPL (type 'exit' to quit)\n");
    Env *global = env_create(NULL);
    functions_clear();
    Node **repl_programs = NULL;
    int repl_program_count = 0;
    int repl_program_capacity = 0;

    while (1)
    {
        printf(">>> ");
        fflush(stdout);

        char line[2048];
        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }

        // Allow user to quit with the word "exit"
        char *p = line;
        while (isspace((unsigned char)*p))
            p++;
        if (strncmp(p, "exit", 4) == 0 && (p[4] == '\0' || isspace((unsigned char)p[4])))
        {
            break;
        }

        // Reuse existing front-end to parse this line as a mini program.
        Lexer *lexer = create_lexer(line);
        Parser *parser = create_parser(lexer);
        Node *program = parse_program(parser);

        if (!program)
        {
            printf("Parse error in REPL input.\n");
            free_parser(parser);
            free_lexer(lexer);
            continue;
        }

        RuntimeError err;
        err.has_error = false;
        err.message = NULL;
        err.line = 0;
        err.column = 0;

        if (program->type == NODE_PROGRAM)
        {
            for (int i = 0; i < program->program.count; i++)
            {
                if (!exec_node(global, program->program.statements[i], &err))
                    break;
            }
        }
        else
        {
            (void)exec_node(global, program, &err);
        }

        if (err.has_error && err.message)
        {
            printf("Runtime error at line %d, column %d: %s\n",
                   err.line, err.column, err.message);
            free(err.message);
        }

        // Keep parsed program chunks alive for REPL session so function
        // declarations stored in function table remain valid.
        if (repl_program_count >= repl_program_capacity)
        {
            int new_cap = repl_program_capacity == 0 ? 16 : repl_program_capacity * 2;
            Node **new_arr = (Node **)realloc(repl_programs, (size_t)new_cap * sizeof(Node *));
            if (!new_arr)
            {
                // Fallback: free current program to avoid leak if allocation fails.
                free_node(program);
            }
            else
            {
                repl_programs = new_arr;
                repl_program_capacity = new_cap;
                repl_programs[repl_program_count++] = program;
            }
        }
        else
        {
            repl_programs[repl_program_count++] = program;
        }
        free_parser(parser);
        free_lexer(lexer);
    }

    for (int i = 0; i < repl_program_count; i++)
    {
        free_node(repl_programs[i]);
    }
    free(repl_programs);
    env_free(global);
    functions_clear();
}

