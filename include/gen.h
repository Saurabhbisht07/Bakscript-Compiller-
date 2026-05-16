#ifndef GEN_H
#define GEN_H

#include "tac.h"
typedef struct
{
    char *name;     
    char *variable; 
    bool is_free;   
} Register;

// Code generation context
typedef struct
{
    Register *registers; 
    int reg_count;       
    int current_reg;     
    char *output;        
    int output_size;     
    int output_pos;      
} GenContext;

GenContext *create_gen_context(void);
void free_gen_context(GenContext *context);
char *generate_code(TAC *tac);
void append_code(GenContext *context, const char *format, ...);
Register *allocate_register(GenContext *context, const char *variable);
void free_register(GenContext *context, Register *reg);

#endif 