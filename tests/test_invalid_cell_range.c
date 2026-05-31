#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    system("echo \"A1=SUM(A2:A10)\nq\" | ./sheet 5 1 > tests/output_invalid_cell_range.txt");

    FILE *f = fopen("tests/output_invalid_cell_range.txt", "r");
    if (!f) {
        printf("❌ test_invalid_cell_range: cannot open output\n");
        return 1;
    }

    char buf[4096] = {0};
    fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);

    const char *expected =
        "        A\n"
        "1       0\n"
        "2       0\n"
        "3       0\n"
        "4       0\n"
        "5       0\n"
        "[0.0] (ok) >         A\n"
        "1       0\n"
        "2       0\n"
        "3       0\n"
        "4       0\n"
        "5       0\n"
        "[0.0] (Invalid cell) > ";

    if (strcmp(buf, expected) != 0) {
        printf("❌ test_invalid_cell_range failed\n");
        printf("Expected:\n%s\n", expected);
        printf("Got:\n%s\n", buf);
        return 1;
    }

    printf("✅ test_invalid_cell_range passed\n");
    return 0;
}
