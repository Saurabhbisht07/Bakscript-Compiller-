#include <stdio.h>
#include <stdlib.h>

void show_num(long x) {
    printf("%ld\n", x);
}

void show_str(char* s) {
    printf("%s\n", s);
}

void process_exit(long code) {
    exit(code);
}