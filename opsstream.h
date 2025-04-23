#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
typedef struct {
    bool has_value;  
    char* value;    
} optstr;

optstr opsmake(const char* str) {
    optstr opt;
    opt.has_value = true;
    opt.value = strdup(str);
    return opt;
}

optstr opsnull() {
    optstr opt;
    opt.has_value = false;
    opt.value = NULL;
    return opt;
}

void opsfree(optstr* opt) {
    if (opt->has_value && opt->value != NULL) {
        free(opt->value);
        opt->value = NULL;
    }
    opt->has_value = 0;
}

void opsprint(const optstr* opt) {
    if (opt->has_value && opt->value != NULL) {
        printf("\"%s\"\n", opt->value);
    } else {
        printf("");
    }
}
