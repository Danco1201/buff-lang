#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} StringStream;

void ssinit(StringStream *ss) {
    ss->capacity = 128;
    ss->length = 0;
    ss->data = malloc(ss->capacity);
    if (ss->data) ss->data[0] = '\0';
}

void ssfree(StringStream *ss) {
    free(ss->data);
    ss->data = NULL;
    ss->length = 0;
    ss->capacity = 0;
}

void expandss(StringStream *ss, size_t new_len) {
    if (new_len >= ss->capacity) {
        while (new_len >= ss->capacity)
            ss->capacity *= 2;
        ss->data = realloc(ss->data, ss->capacity);
    }
}

void appendss(StringStream *ss, const char *text) {
    size_t len = strlen(text);
    expandss(ss, ss->length + len + 1);
    memcpy(ss->data + ss->length, text, len + 1);
    ss->length += len;
}

void appendf(StringStream *ss, const char *fmt, ...) {
    char temp[1024];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(temp, sizeof(temp), fmt, args);
    va_end(args);
    if (n > 0)
        appendss(ss, temp);
}

char *ssfile(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = (char *)malloc(size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    fread(buffer, 1, size, fp);
    buffer[size] = '\0';

    fclose(fp);
    return buffer;
}
void appendssf(StringStream *ss, const char *format, ...) {
    char buffer[1024];  // ajustá según tu necesidad
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    appendss(ss, buffer);
}
