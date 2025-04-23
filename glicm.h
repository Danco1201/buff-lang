#include <stdio.h>
#include "sslib.h"
#include "opsstream.h"
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#define DEBUG(...) fprintf(stderr, "DEBUG [%s:%d] ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__)
#define ERREXIT(...) do { \
    fprintf(stderr, "ERROR [%s:%d]: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    exit(1); \
} while (0)
typedef enum {
    _print,
    intlit,
    scolon
} TokenT;

typedef struct {
    TokenT type;
    optstr value;
    int line;
    int chr;
} token;
typedef struct {
    token* data;
    size_t size;
    size_t capc;
} tkVec;
void pushbk(char **str, char c) {
    size_t len = *str ? strlen(*str) : 0;
    char *temp = realloc(*str, len + 2);  
    if (!temp) {
        perror("realloc falló");
        exit(EXIT_FAILURE);
    }
    temp[len] = c;
    temp[len + 1] = '\0';
    *str = temp;
}
void pushtk(tkVec* vec, token t) {
    if (vec->size >= vec->capc) {
        size_t ncapc = vec->capc ? vec->capc * 2 : 4;
        token* temp = realloc(vec->data, ncapc * sizeof(token));
        if (!temp) {
            perror("realloc failed in func pushtk, line 23");
            exit(EXIT_FAILURE);
        }
        vec->data = temp;
        vec->capc = ncapc;
    }

    vec->data[vec->size++] = t;
}

tkVec tokenize(const char* value) {
    tkVec tokens = {NULL, 0, 0};
    char* buf = NULL;
    int line = 1, col = 1;

    for (int i = 0; i < strlen(value); i++, col++) {
        char p = value[i];

        if (p == '\n') {
            line++;
            col = 0;
            continue;
        }

        if (isalpha(p)) {
            int strt = col;
            pushbk(&buf, p);
            i++; col++;
            while (isalnum(value[i])) {
                pushbk(&buf, value[i]);
                i++; col++;
            }
            i--; col--;

            if (strcmp(buf, "print") == 0) {
                token t = {.type = _print, .line = line, .chr = strt};
                optstr opt = {.has_value = false, .value = NULL};
                t.value = opt;
                free(buf); buf = NULL;
                pushtk(&tokens, t);
                continue;
            } else {
                fprintf(stderr, "ERROR [line %d, char %d]: unknown command '%s'\n", line, strt, buf);
                exit(1);
            }
        } else if (isdigit(p)) {
            int strt = col;
            pushbk(&buf, p);
            i++; col++;
            while (isdigit(value[i])) {
                pushbk(&buf, value[i]);
                i++; col++;
            }
            i--; col--;

            token t = {.type = intlit, .line = line, .chr = strt};
            optstr opt = {.has_value = true, .value = buf};
            t.value = opt;
            buf = NULL;
            pushtk(&tokens, t);
            continue;

        } else if (p == ';') {
            token s = {.type = scolon, .line = line, .chr = col};
            optstr opt = {.has_value = false, .value = NULL};
            s.value = opt;
            pushtk(&tokens, s);
            continue;

        } else if (isspace(p)) {
            continue;
        } else {
            fprintf(stderr, "ERROR [line %d, col %d]: invalid character '%c'\n", line, col, p);
            exit(1);
        }
    }
    return tokens;
}

char* toasm(const tkVec* tokens) {
    StringStream info, text;
    info.data = strdup("section .data\n");
    text.data = strdup("section .text\nglobal _start\n_start:\n");

    int countmsg = 0;

    for (int i = 0; i < tokens->size; i++) {
        const token* tk = &tokens->data[i];
        if (tk->type == _print) {
            if (i + 1 < tokens->size && tokens->data[i + 1].type == intlit &&
                i + 2 < tokens->size && tokens->data[i + 2].type == scolon) {

                char msg[32];
                sprintf(msg, "msg_%d", countmsg);

                appendssf(&info, "    %s db \"%s\", 10\n", msg, tokens->data[i + 1].value.value);
                appendssf(&info, "    len_%d equ $ - %s\n", countmsg, msg);

                appendssf(&text, "    mov rax, 1\n");
                appendssf(&text, "    mov rdi, 1\n");
                appendssf(&text, "    mov rsi, %s\n", msg);
                appendssf(&text, "    mov rdx, len_%d\n", countmsg);
                appendssf(&text, "    syscall\n");

                countmsg++;
                i += 2;  
            }
        }
    }

    appendssf(&text, "    mov rax, 60\n");
    appendssf(&text, "    xor rdi, rdi\n");
    appendssf(&text, "    syscall\n");

    size_t full = strlen(info.data) + strlen(text.data) + 1;
    char* res = malloc(full);
    snprintf(res, full, "%s%s", info.data, text.data);

    free(info.data);
    free(text.data);
    return res;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso incorrecto del lenguaje\n");
        fprintf(stderr, "Uso: glicm <archivo.gcm>\n");
        return 1;
    }

    FILE *input = fopen(argv[1], "r");
    if (!input) {
        perror("No se pudo abrir el archivo");
        return 1;
    }

    char *contents = ssfile(argv[1]);
    fclose(input);

    if (!contents) {
        fprintf(stderr, "Error al leer el archivo\n");
        return 1;
    }

    tkVec tks = tokenize(contents);
    char *asmCode = toasm(&tks);

    FILE *file = fopen("out.asm", "w");
    if (!file) {
        perror("No se pudo crear el archivo ASM");
        free(asmCode);
        free(contents);
        return 1;
    }

    fprintf(file, "%s", asmCode);
    fclose(file);

    if (system("nasm -f win64 out.asm -o out.obj") != 0) {
        fprintf(stderr, "Error al ensamblar con NASM\n");
        free(asmCode);
        free(contents);
        return 1;
    }

    if (system("gcc -o out.exe out.obj") != 0) {
        fprintf(stderr, "Error al compilar con GCC\n");
        free(asmCode);
        free(contents);
        return 1;
    }

    system("out.exe");
    DEBUG("Contenido de ASM generado:\n%s\n", asmCode);

    free(asmCode);
    free(contents);
    return 0;
}
