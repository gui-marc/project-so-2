#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct my_s {
    int a;
} my_s;

typedef struct common_s {
    struct my_s base;
} common_s;

typedef struct some_s {
    struct my_s base;
    int b;
    char string[10];
} some_s;

typedef struct another_s {
    struct my_s base;
    int c;
} another_s;

void *create_some(int a, int b, char *str) {
    some_s *s = malloc(sizeof(some_s));
    s->b = b;
    s->base.a = a;
    strcpy(s->string, str);
    return s;
}

int main() {
    common_s *common = (common_s *)create_some(1, 2, "STRING");

    some_s *some = (some_s *)common;

    printf("%s\n", some->string);
    printf("%d\n", common->base.a);
}