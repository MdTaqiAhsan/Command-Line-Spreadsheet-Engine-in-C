#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    system("echo \"A1=5\nB1=A1+3\nq\" | ./sheet 2 2 > tests/output_arith.txt");

    FILE *f = fopen("tests/output_arith.txt", "r");
    if (!f) {
        printf("❌ test_arithmetic: cannot open output\n");
        return 1;
    }

    char buf[4096] = {0};
    fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);

    const char *expected = "        A     B\n"
                           "1       0     0\n"
                           "2       0     0\n"
                           "[0.0] (ok) >         A     B\n"
                           "1       5     0\n"
                           "2       0     0\n"
                           "[0.0] (ok) >         A     B\n"
                           "1       5     8\n"
                           "2       0     0\n"
                           "[0.0] (ok) > ";

    if (strcmp(buf, expected) != 0) {
        printf("❌ test_arithmetic failed\n");
        printf("Expected:\n%s\n", expected);
        printf("Got:\n%s\n", buf);
        return 1;
    }

    printf("✅ test_arithmetic passed\n");
    return 0;
}