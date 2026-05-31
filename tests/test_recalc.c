#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    system("echo \"A1=2\nB1=A1+1\nA1=5\nq\" | ./sheet 2 2 > tests/output_recalc.txt");

    FILE *f = fopen("tests/output_recalc.txt", "r");
    if (!f) {
        printf("❌ test_recalc: cannot open output\n");
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
        "1       2     0\n"
        "2       0     0\n"
        "[0.0] (ok) >         A     B\n"
        "1       2     3\n"
        "2       0     0\n"
        "[0.0] (ok) >         A     B\n"
        "1       5     6\n"
        "2       0     0\n"
        "[0.0] (ok) > ";

    if (strcmp(buf, expected) != 0) {
        printf("❌ test_recalc failed\n");
        printf("Expected:\n%s\n", expected);
        printf("Got:\n%s\n", buf);
        return 1;
    }

    printf("✅ test_recalc passed\n");
    return 0;
}