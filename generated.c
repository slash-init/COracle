#include <stdio.h>

int main(void) {
    int v0 = (79 - -89);
    printf("%d\n", v0);
    int v1 = ((-39 + (-47 - -42)) - v0);
    int v2 = v1;
    v1 = v2;
    v2 = (-72 + (v1 - v1));
    return 0;
}

// OK: reproducibility check passed.
