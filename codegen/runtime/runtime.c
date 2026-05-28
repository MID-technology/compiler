#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void* ol_alloc(int64_t size) {
    void* p = calloc(1, (size_t)size);
    return p;
}

const char* ol_str_new(const char* s) {
    if (!s) return "";
    size_t n = strlen(s);
    char* r = (char*)malloc(n + 1);
    memcpy(r, s, n + 1);
    return r;
}

const char* ol_str_empty(void) {
    char* r = (char*)malloc(1);
    r[0] = '\0';
    return r;
}

const char* ol_str_concat(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    size_t la = strlen(a), lb = strlen(b);
    char* r = (char*)malloc(la + lb + 1);
    memcpy(r, a, la);
    memcpy(r + la, b, lb);
    r[la + lb] = '\0';
    return r;
}

const char* ol_str_at(const char* s, int64_t i) {
    if (!s) return ol_str_empty();
    size_t n = strlen(s);
    if (i < 0 || (size_t)i >= n) return ol_str_empty();
    char* r = (char*)malloc(2);
    r[0] = s[i];
    r[1] = '\0';
    return r;
}

int64_t ol_str_length(const char* s) {
    if (!s) return 0;
    return (int64_t)strlen(s);
}

int ol_str_equal(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    return strcmp(a, b) == 0;
}

const char* ol_str_substring(const char* s, int64_t from, int64_t to) {
    if (!s) return ol_str_empty();
    int64_t n = (int64_t)strlen(s);
    if (from < 0) from = 0;
    if (to > n) to = n;
    if (to < from) to = from;
    size_t len = (size_t)(to - from);
    char* r = (char*)malloc(len + 1);
    memcpy(r, s + from, len);
    r[len] = '\0';
    return r;
}

const char* ol_int_to_string(int64_t v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)v);
    return ol_str_new(buf);
}

const char* ol_real_to_string(double v) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", v);
    return ol_str_new(buf);
}

const char* ol_bool_to_string(int v) {
    return ol_str_new(v ? "true" : "false");
}

void ol_io_write(const char* s) {
    if (s) fputs(s, stdout);
}

void ol_io_writeln(const char* s) {
    if (s) fputs(s, stdout);
    fputc('\n', stdout);
}

const char* ol_io_read(void) {
    char buf[1024];
    if (!fgets(buf, sizeof(buf), stdin)) return ol_str_empty();
    size_t n = strlen(buf);
    if (n && buf[n-1] == '\n') buf[n-1] = '\0';
    return ol_str_new(buf);
}

const char* ol_io_readline(void) {
    return ol_io_read();
}
