#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    system("echo \"A1=1\nB1=1/A1\nA2=B1+2\nA1=0\nq\" | ./sheet 2 2 > tests/output_div_zero_propagation.txt");

    FILE *f = fopen("tests/output_div_zero_propagation.txt", "r");
    if (!f) {
        printf("❌ test_div_zero_propagation: cannot open output\n");
        return 1;
    }

    char buf[8192] = {0};
    fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);

    const char *expected =
        "        A     B\n"
        "1       0     0\n"
        "2       0     0\n"
        "[0.0] (ok) >         A     B\n"
        "1       1     0\n"
        "2       0     0\n"
        "[0.0] (ok) >         A     B\n"
        "1       1     1\n"
        "2       0     0\n"
        "[0.0] (ok) >         A     B\n"
        "1       1     1\n"
        "2       3     0\n"
        "[0.0] (ok) >         A     B\n"
        "1       0   ERR\n"
        "2     ERR     0\n"
        "[0.0] (ok) > ";

    if (strcmp(buf, expected) != 0) {
        printf("❌ test_div_zero_propagation failed\n");
        printf("Expected:\n%s\n", expected);
        printf("Got:\n%s\n", buf);
        return 1;
    }

    printf("✅ test_div_zero_propagation passed\n");
    return 0;
}