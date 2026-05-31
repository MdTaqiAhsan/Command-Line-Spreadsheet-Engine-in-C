#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    system("echo \"scroll_to B2\nq\" | ./sheet 3 3 > tests/output_scroll.txt");

    FILE *f = fopen("tests/output_scroll.txt", "r");
    if (!f) {
        printf("❌ test_scroll_to: cannot open output\n");
        return 1;
    }

    char buf[4096] = {0};
    fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);

    const char *expected = "        A     B     C\n"
                           "1       0     0     0\n"
                           "2       0     0     0\n"
                           "3       0     0     0\n"
                           "[0.0] (ok) >         B     C\n"
                           "2       0     0\n"
                           "3       0     0\n"
                           "[0.0] (ok) > ";

    if (strcmp(buf, expected) != 0) {
        printf("❌ test_scroll_to failed\n");
        printf("Expected:\n%s\n", expected);
        printf("Got:\n%s\n", buf);
        return 1;
    }

    printf("✅ test_scroll_to passed\n");
    return 0;
}