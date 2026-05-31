#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    system("echo \"A1=1\nA2=2\nA3=3\nB1=SUM(A1:A3)\nq\" | ./sheet 3 2 > tests/output_sum_function.txt");

    FILE *f = fopen("tests/output_sum_function.txt", "r");
    if (!f) {
        printf("❌ test_sum_function: cannot open output\n");
        return 1;
    }

    char buf[8192] = {0};
    fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);

    const char *expected =
        "        A     B\n"
        "1       0     0\n"
        "2       0     0\n"
        "3       0     0\n"
        "[0.0] (ok) >         A     B\n"
        "1       1     0\n"
        "2       0     0\n"
        "3       0     0\n"
        "[0.0] (ok) >         A     B\n"
        "1       1     0\n"
        "2       2     0\n"
        "3       0     0\n"
        "[0.0] (ok) >         A     B\n"
        "1       1     0\n"
        "2       2     0\n"
        "3       3     0\n"
        "[0.0] (ok) >         A     B\n"
        "1       1     6\n"
        "2       2     0\n"
        "3       3     0\n"
        "[0.0] (ok) > ";

    if (strcmp(buf, expected) != 0) {
        printf("❌ test_sum_function failed\n");
        printf("Expected:\n%s\n", expected);
        printf("Got:\n%s\n", buf);
        return 1;
    }

    printf("✅ test_sum_function passed\n");
    return 0;
}