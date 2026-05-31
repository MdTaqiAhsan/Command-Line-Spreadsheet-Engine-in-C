#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    system("echo \"hello\nq\" | ./sheet 2 2 > tests/output_basic.txt");

    FILE *f = fopen("tests/output_basic.txt", "r");
    if (!f) {
        printf("❌ test_basic: cannot open output\n");
        return 1;
    }

    char buf[4096] = {0};
    fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);

    const char *expected = "        A     B\n"
                           "1       0     0\n"
                           "2       0     0\n"
                           "[0.0] (ok) >         A     B\n"
                           "1       0     0\n"
                           "2       0     0\n"
                           "[0.0] (unrecognized cmd) > ";

    if (strcmp(buf, expected) != 0) {
        printf("❌ test_basic failed\n");
        printf("Expected:\n%s\n", expected);
        printf("Got:\n%s\n", buf);
        return 1;
    }

    printf("✅ test_basic passed\n");
    return 0;
}