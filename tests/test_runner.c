#include <stdio.h>
#include <stdlib.h>

int run(const char *cmd) {
    int ret = system(cmd);
    return (ret == 0);
}

int main() {
    int ok = 1;

    ok &= run("./tests/test_basic");
    ok &= run("./tests/test_scroll_to");
    ok &= run("./tests/test_arithmetic");
    ok &= run("./tests/test_sum_function");
    ok &= run("./tests/test_invalid_range");
    ok &= run("./tests/test_invalid_cell_range");
    ok &= run("./tests/test_recalc");
    ok &= run("./tests/test_div_zero_propagation");

    if (ok) {
        printf("✅ All C tests passed\n");
        return 0;
    } else {
        printf("❌ Some tests failed\n");
        return 1;
    }
}
