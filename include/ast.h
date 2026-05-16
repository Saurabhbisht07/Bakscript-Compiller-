#ifndef AST_H
#define AST_H

typedef enum
{
    // Expressions
    NODE_NUMBER,
    NODE_BOOL,
    NODE_STRING,
    NODE_IDENTIFIER,
    NODE_ARRAY_LITERAL,
    NODE_INDEX_ACCESS,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_FUNCTION_CALL,

    // Statements
    NODE_VARIABLE_DECLARATION,
    NODE_IF_STATEMENT,
    NODE_FOR_LOOP,
    NODE_WHILE_LOOP,
    NODE_FUNCTION_DECLARATION,
    NODE_RETURN_STATEMENT,
    NODE_BLOCK,
    NODE_PROGRAM
} NodeType;
typedef struct Node Node;
typedef enum
{
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_LESS,
    OP_GREATER,
    OP_LESS_EQ,
    OP_GREATER_EQ,
    OP_AND,
    OP_OR,
    OP_ASSIGN
} BinaryOpType;

typedef enum
{
    UOP_NOT
} UnaryOpType;
typedef struct
{
    int line;
    int column;
} NodeInfo;
typedef struct
{
    BinaryOpType op;
    Node *left;
    Node *right;
    NodeInfo info;
} BinaryOpNode;
typedef struct
{
    UnaryOpType op;
    Node *operand;
    NodeInfo info;
} UnaryOpNode;
typedef struct
{
    int value;
    NodeInfo info;
} NumberNode;
typedef struct
{
    int value; // 0/1
    NodeInfo info;
} BoolNode;
typedef struct
{
    char *value;
    NodeInfo info;
} StringNode;
typedef struct
{
    char *name;
    NodeInfo info;
} IdentifierNode;
typedef struct
{
    Node **elements;
    int count;
    NodeInfo info;
} ArrayLiteralNode;
typedef struct
{
    Node *array;
    Node *index;
    NodeInfo info;
} IndexAccessNode;
typedef struct
{
    char *name;
    Node **arguments;
    int arg_count;
    NodeInfo info;
} FunctionCallNode;
typedef struct
{
    char *type;
    char *name;
    Node *initializer;
    NodeInfo info;
} VariableDeclarationNode;
typedef struct
{
    Node *condition;
    Node *if_body;
    Node *else_body;
    NodeInfo info;
} IfStatementNode;
typedef struct
{
    Node *initializer;
    Node *condition;
    Node *increment;
    Node *body;
    NodeInfo info;
} ForLoopNode;
typedef struct
{
    Node *condition;
    Node *body;
    NodeInfo info;
} WhileLoopNode;

typedef struct
{
    char *name;
    char **params;
    int param_count;
    Node *body; // NODE_BLOCK
    NodeInfo info;
} FunctionDeclarationNode;

typedef struct
{
    Node *value; // expression
    NodeInfo info;
} ReturnStatementNode;
typedef struct
{
    Node **statements;
    int count;
    NodeInfo info;
} BlockNode;
typedef struct
{
    Node **statements;
    int count;
    NodeInfo info;
} ProgramNode;
struct Node
{
    NodeType type;
    union
    {
        NumberNode number;
        BoolNode boolean;
        StringNode string;
        IdentifierNode identifier;
        ArrayLiteralNode array_literal;
        IndexAccessNode index_access;
        BinaryOpNode binary_op;
        UnaryOpNode unary_op;
        FunctionCallNode function_call;
        VariableDeclarationNode var_decl;
        IfStatementNode if_stmt;
        ForLoopNode for_loop;
        WhileLoopNode while_loop;
        FunctionDeclarationNode function_decl;
        ReturnStatementNode return_stmt;
        BlockNode block;
        ProgramNode program;
    };
};

// Function declarations for creating AST nodes
Node *create_number_node(int value, int line, int column);
Node *create_bool_node(int value, int line, int column);
Node *create_string_node(const char *value, int line, int column);
Node *create_identifier_node(const char *name, int line, int column);
Node *create_array_literal_node(Node **elements, int count, int line, int column);
Node *create_index_access_node(Node *array, Node *index, int line, int column);
Node *create_binary_op_node(BinaryOpType op, Node *left, Node *right, int line, int column);
Node *create_unary_op_node(UnaryOpType op, Node *operand, int line, int column);
Node *create_function_call_node(const char *name, Node **arguments, int arg_count, int line, int column);
Node *create_var_decl_node(const char *type, const char *name, Node *initializer, int line, int column);
Node *create_if_node(Node *condition, Node *if_body, Node *else_body, int line, int column);
Node *create_for_node(Node *initializer, Node *condition, Node *increment, Node *body, int line, int column);
Node *create_while_node(Node *condition, Node *body, int line, int column);
Node *create_function_decl_node(const char *name, char **params, int param_count, Node *body, int line, int column);
Node *create_return_node(Node *value, int line, int column);
Node *create_block_node(Node **statements, int count, int line, int column);
Node *create_program_node(Node **statements, int count);
void free_node(Node *node);

#endif
