#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include "sheet.h"
#include "parser.h"

/* -----------------------------BASE 26 COL indexing------------------------------------- */
int col_name_to_index(const char *s, int len) {
    if (len <= 0 || len > 3) return -1;
    int val = 0;
    for (int i = 0; i < len; i++) {
        char ch = s[i];
        if (ch < 'A' || ch > 'Z') return -1;
        val = val * 26 + (ch - 'A' + 1);
    }
    return val - 1;  /* 0-based */
}

/*-----------CELL PARSER --------------------*/
int parse_cell_name(const char **p, int *r, int *c) {
    const char *s = *p;

    /* collect letters */
    int col_len = 0;
    while (s[col_len] && isupper((unsigned char)s[col_len])) col_len++;
    if (col_len == 0 || col_len > 3) return 0;

    /* collect digits */
    int row_len = 0;
    while (s[col_len + row_len] && isdigit((unsigned char)s[col_len + row_len]))
        row_len++;
    if (row_len == 0) return 0;

    int col = col_name_to_index(s, col_len);
    if (col < 0) return 0;

    int row = atoi(s + col_len) - 1;  /* 0-based */
    if (row < 0) return 0;

    *r = row;
    *c = col;
    *p = s + col_len + row_len;
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Expression evaluator                                               */
/* ------------------------------------------------------------------ */

/* We carry a tiny result type that knows about error kind. */
typedef enum {
    EVAL_OK = 0,
    EVAL_ERR,
    EVAL_SYNTAX,
    EVAL_INVALID_CELL,
    EVAL_DIV_BY_ZERO,
} EvalErr;

typedef struct {
    int value;
    EvalErr err;
} Eval;

static Eval make_val(int v)                { Eval e; e.value = v; e.err = EVAL_OK; return e; }
static Eval make_err(EvalErr err)          { Eval e; e.value = 0; e.err = err; return e; }
static Eval make_err_default(void)         { return make_err(EVAL_ERR); }
static Eval make_err_syntax(void)          { return make_err(EVAL_SYNTAX); }

/* Forward declaration */
static Eval eval_expr(Sheet *sheet, const char *s, const char **end);

/*
 * eval_value:
 *   Parses and evaluates a Value (constant integer or cell reference).
 */
static Eval eval_value(Sheet *sheet, const char *s, const char **end) {
    /* Negative constant: handle unary minus */
    if (*s == '-') {
        const char *p = s + 1;
        Eval e = eval_value(sheet, p, end);
        if (e.err != EVAL_OK) return e;
        return make_val(-e.value);
    }

    /* Positive integer constant */
    if (isdigit((unsigned char)*s)) {
        char *endp;
        long v = strtol(s, &endp, 10);
        *end = endp;
        return make_val((int)v);
    }

    /* Cell reference */
    if (isupper((unsigned char)*s)) {
        int r, c;
        const char *p = s;
        if (!parse_cell_name(&p, &r, &c)) {
            *end = s;
            return make_err_syntax();
        }
        *end = p;
        if (r < 0 || r >= sheet->rows || c < 0 || c >= sheet->cols)
            return make_err(EVAL_INVALID_CELL);
        Cell *cell = get_cell(sheet, r, c);
        if (!cell || cell->is_error) return make_err(EVAL_ERR);
        return make_val(cell->value);
    }

    *end = s;
    return make_err_syntax();
}

/*
 * eval_function:
 *   Parses and evaluates FUNC(Range) or SLEEP(Value).
 *   *p should point to the character after the opening '('.
 */
static Eval eval_function(Sheet *sheet, const char *fname,
                          const char *p, const char **end) {

    /* SLEEP(Value) */
    if (strcmp(fname, "SLEEP") == 0) {
        Eval arg = eval_value(sheet, p, end);
        if ((*end)[0] != ')') return make_err_syntax();
        *end = *end + 1;
        if (arg.err != EVAL_OK) return arg;
        if (arg.value > 0) sleep((unsigned int)arg.value);
        return make_val(arg.value);
    }

    /* Range functions: MIN MAX AVG SUM STDEV */
    int is_min   = strcmp(fname, "MIN")   == 0;
    int is_max   = strcmp(fname, "MAX")   == 0;
    int is_avg   = strcmp(fname, "AVG")   == 0;
    int is_sum   = strcmp(fname, "SUM")   == 0;
    int is_stdev = strcmp(fname, "STDEV") == 0;

    if (!is_min && !is_max && !is_avg && !is_sum && !is_stdev)
        return make_err_default();

    /* Parse first cell of range */
    int r1, c1, r2, c2;
    if (!parse_cell_name(&p, &r1, &c1)) return make_err_syntax();
    if (*p != ':') return make_err_syntax();
    p++;
    if (!parse_cell_name(&p, &r2, &c2)) return make_err_syntax();
    if (*p != ')') return make_err_syntax();
    *end = p + 1;

    /* Validate range direction */
    if (r1 > r2 || c1 > c2) return make_err_default();  /* caller returns "Invalid range" */

    /* Bounds check */
    if (r1 < 0 || r2 >= sheet->rows || c1 < 0 || c2 >= sheet->cols)
        return make_err(EVAL_INVALID_CELL);

    int count = 0;
    long sum = 0;
    int min_val = 0;
    int max_val = 0;
    double mean = 0.0;
    double m2 = 0.0;
    Eval result = make_val(0);

    for (int r = r1; r <= r2; r++) {
        for (int c = c1; c <= c2; c++) {
            Cell *cell = get_cell(sheet, r, c);
            if (!cell) return make_err(EVAL_INVALID_CELL);
            if (cell->is_error) return make_err(EVAL_ERR);

            int value = cell->value;
            if (count == 0) {
                min_val = max_val = value;
                mean = value;
                m2 = 0.0;
            } else {
                if (value < min_val) min_val = value;
                if (value > max_val) max_val = value;
                double delta = value - mean;
                mean += delta / (count + 1);
                m2 += delta * (value - mean);
            }
            sum += value;
            count++;
        }
    }

    if (count == 0) return make_err(EVAL_ERR);

    if (is_sum) {
        result = make_val((int)sum);
    } else if (is_avg) {
        result = make_val((int)(sum / count));
    } else if (is_min) {
        result = make_val(min_val);
    } else if (is_max) {
        result = make_val(max_val);
    } else if (is_stdev) {
        double sd = sqrt(m2 / count);
        result = make_val((int)sd);
    }

    return result;
}

/*
 * eval_expr:
 *   Parses Value [op Value] or FUNC(Range/Value).
 *   Returns ERR on any failure.
 */
static Eval eval_expr(Sheet *sheet, const char *s, const char **end) {
    /* Function call? */
    if (isupper((unsigned char)*s)) {
        /* Peek ahead: if we see letters followed by '(' it's a function */
        int flen = 0;
        while (s[flen] && isupper((unsigned char)s[flen])) flen++;
        if (s[flen] == '(') {
            char fname[16];
            if (flen >= (int)sizeof(fname)) return make_err_default();
            memcpy(fname, s, flen);
            fname[flen] = '\0';
            const char *p = s + flen + 1;  /* skip '(' */
            return eval_function(sheet, fname, p, end);
        }
        /* Otherwise fall through to value (cell reference or arithmetic) */
    }

    /* LHS value */
    const char *p;
    Eval lhs = eval_value(sheet, s, &p);

    /* Optional binary operator */
    char op = *p;
    if (op == '+' || op == '-' || op == '*' || op == '/') {
        p++;
        const char *rhs_end = NULL;
        Eval rhs = eval_value(sheet, p, &rhs_end);
        if (rhs.err != EVAL_OK) {
            *end = rhs_end;
            return lhs.err != EVAL_OK ? lhs : rhs;
        }
        if (lhs.err != EVAL_OK) {
            *end = rhs_end;
            return lhs;
        }
        *end = rhs_end;
        if (op == '+') return make_val(lhs.value + rhs.value);
        if (op == '-') return make_val(lhs.value - rhs.value);
        if (op == '*') return make_val(lhs.value * rhs.value);
        if (op == '/') {
            if (rhs.value == 0) return make_err(EVAL_DIV_BY_ZERO);
            return make_val(lhs.value / rhs.value);
        }
    }

    *end = p;
    return lhs;
}

/* ------------------------------------------------------------------ */
/*  Range direction validation helper                                  */
/* ------------------------------------------------------------------ */

/*
 * check_range_direction:
 *   Re-parses a range inside a function argument to see if it was
 *   specified in the wrong direction (r1>r2 or c1>c2).
 *   Returns 1 if invalid direction, 0 otherwise.
 */
static int range_is_backwards(const char *expr) {
    /* Find the opening '(' */
    const char *p = strchr(expr, '(');
    if (!p) return 0;
    p++;
    int r1, c1, r2, c2;
    if (!parse_cell_name(&p, &r1, &c1)) return 0;
    if (*p != ':') return 0;
    p++;
    if (!parse_cell_name(&p, &r2, &c2)) return 0;
    return (r1 > r2 || c1 > c2);
}

/* ------------------------------------------------------------------ */
/*  Dependency collection                                              */
/* ------------------------------------------------------------------ */

/*
 * has_cell_ref:
 *   Returns 1 if the expression string contains any cell reference or
 *   function call (i.e. it's not a plain integer constant).
 *   Used to set cell->has_formula correctly.
 */
static int has_cell_ref(const char *expr) {
    for (const char *p = expr; *p; p++)
        if (isupper((unsigned char)*p)) return 1;
    return 0;
}

/*
 * collect_deps:
 *   Scans an expression string and registers a dependency edge from every
 *   cell reference found in it to the destination cell (dest_r, dest_c).
 *   Also handles range syntax (A1:B3) by registering all cells in the range.
 */
static int collect_deps(Sheet *sheet, const char *expr, int dest_r, int dest_c) {
    const char *p = expr;
    while (*p) {
        if (isupper((unsigned char)*p)) {
            /* Try to parse a cell name */
            int r, c;
            const char *after = p;
            if (parse_cell_name(&after, &r, &c)) {
                /* Check if it's a range (cell:cell) */
                if (*after == ':') {
                    const char *after2 = after + 1;
                    int r2, c2;
                    if (parse_cell_name(&after2, &r2, &c2) && r2 >= r && c2 >= c) {
                        /* Validate the range bounds */
                        if (r < 0 || r2 >= sheet->rows || c < 0 || c2 >= sheet->cols)
                            return 1;
                        /* Register the whole cell range as a single dependency edge */
                        sheet_add_dep_range(sheet, r, c, r2, c2, dest_r, dest_c);
                        p = after2;
                        continue;
                    }
                }
                /* Validate single-cell bounds */
                if (r < 0 || r >= sheet->rows || c < 0 || c >= sheet->cols)
                    return 1;
                sheet_add_dep(sheet, r, c, dest_r, dest_c);
                p = after;
                continue;
            }
        }
        p++;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Circular dependency detection                                      */
/* ------------------------------------------------------------------ */

/*
 * has_cycle:
 *   DFS from (start_r, start_c) following "depends-on" edges (i.e. from_r/from_c
 *   direction — what does start depend on, transitively?).
 *   Returns 1 if (target_r, target_c) is reachable, meaning a cycle exists.
 *
 *   We walk: if cell X depends on cell Y (edge Y->X in the dep graph),
 *   then from X we want to find what X depends on, i.e. look for edges
 *   where to_r==X meaning from_r is a dependency of X.
 *
 *   Concretely: starting from dest cell, follow edges where to_r/to_c == current,
 *   visiting from_r/from_c — i.e. "what cells does dest (transitively) pull from?"
 *   If we hit dest itself, there's a cycle.
 */
static int has_cycle(Sheet *sheet, int dest_r, int dest_c) {
    int total = sheet->rows * sheet->cols;
    unsigned char *visited = calloc(total, sizeof(unsigned char));
    if (!visited) return 0;

    /* DFS stack */
    int *stack = malloc(total * sizeof(int));
    if (!stack) { free(visited); return 0; }
    int top = 0;

    /* Seed: all cells that dest directly depends on */
    for (int i = 0; i < sheet->dep_count; i++) {
        DepEdge *e = &sheet->deps[i];
        if (e->to_r == dest_r && e->to_c == dest_c) {
            if (e->is_range) {
                for (int rr = e->from_r; rr <= e->from_r2; rr++) {
                    for (int cc = e->from_c; cc <= e->from_c2; cc++) {
                        int idx = rr * sheet->cols + cc;
                        if (!visited[idx]) {
                            visited[idx] = 1;
                            stack[top++] = idx;
                        }
                    }
                }
            } else {
                int idx = e->from_r * sheet->cols + e->from_c;
                if (!visited[idx]) {
                    visited[idx] = 1;
                    stack[top++] = idx;
                }
            }
        }
    }

    int found = 0;
    while (top > 0 && !found) {
        int idx = stack[--top];
        int r = idx / sheet->cols;
        int c = idx % sheet->cols;

        /* If we've reached dest itself, cycle detected */
        if (r == dest_r && c == dest_c) { found = 1; break; }

        /* Push everything that (r,c) depends on */
        for (int i = 0; i < sheet->dep_count; i++) {
            DepEdge *e = &sheet->deps[i];
            if (e->to_r == r && e->to_c == c) {
                if (e->is_range) {
                    for (int rr = e->from_r; rr <= e->from_r2; rr++) {
                        for (int cc = e->from_c; cc <= e->from_c2; cc++) {
                            int nidx = rr * sheet->cols + cc;
                            if (!visited[nidx]) {
                                visited[nidx] = 1;
                                stack[top++] = nidx;
                            }
                        }
                    }
                } else {
                    int nidx = e->from_r * sheet->cols + e->from_c;
                    if (!visited[nidx]) {
                        visited[nidx] = 1;
                        stack[top++] = nidx;
                    }
                }
            }
        }
    }

    free(visited);
    free(stack);
    return found;
}

static DepEdge *save_deps_for(Sheet *sheet, int to_r, int to_c, int *out_count) {
    int count = 0;
    DepEdge *saved = malloc(sheet->dep_count * sizeof(DepEdge));
    if (!saved) { *out_count = 0; return NULL; }
    for (int i = 0; i < sheet->dep_count; i++) {
        DepEdge *e = &sheet->deps[i];
        if (e->to_r == to_r && e->to_c == to_c)
            saved[count++] = *e;
    }
    *out_count = count;
    return saved;
}

static void restore_deps_for(Sheet *sheet, DepEdge *saved, int count) {
    if (!saved || count == 0) {
        free(saved);
        return;
    }
    if (sheet->dep_count + count > sheet->dep_cap) {
        while (sheet->dep_count + count > sheet->dep_cap)
            sheet->dep_cap *= 2;
        sheet->deps = realloc(sheet->deps, sheet->dep_cap * sizeof(DepEdge));
    }
    for (int i = 0; i < count; i++)
        sheet->deps[sheet->dep_count++] = saved[i];
    free(saved);
}

/* ------------------------------------------------------------------ */
/*  Public entry point                                                 */
/* ------------------------------------------------------------------ */

const char *parse_and_execute(Sheet *sheet, const char *input,
                              int *sr, int *sc) {
    /* scroll_to <CELL> */
    if (strncmp(input, "scroll_to ", 10) == 0) {
        const char *p = input + 10;
        /* skip optional spaces */
        while (*p == ' ') p++;
        int r, c;
        if (!parse_cell_name(&p, &r, &c)) return "unrecognized cmd";
        if (*p != '\0') return "unrecognized cmd";
        if (r < 0 || r >= sheet->rows || c < 0 || c >= sheet->cols)
            return "Invalid cell";
        *sr = r;
        *sc = c;
        return "scroll_to";
    }

    /* Formula: CELL=EXPRESSION */

    /* Parse the target cell */
    const char *p = input;
    int dest_r, dest_c;
    if (!parse_cell_name(&p, &dest_r, &dest_c)) return "unrecognized cmd";
    if (*p != '=') return "unrecognized cmd";
    p++;  /* skip '=' */

    /* Validate destination cell bounds */
    if (dest_r < 0 || dest_r >= sheet->rows ||
        dest_c < 0 || dest_c >= sheet->cols)
        return "Invalid cell";

    /* Check for backwards range before evaluating (for nicer error message) */
    if (range_is_backwards(p)) return "Invalid range";

    /* ------------------------------------------------------------------
     * Dependency registration:
     *   Before evaluating, clear old deps for this cell and re-scan the
     *   RHS expression to collect new cell references it depends on.
     * ------------------------------------------------------------------ */
    int old_dep_count = 0;
    DepEdge *old_deps = save_deps_for(sheet, dest_r, dest_c, &old_dep_count);
    sheet_clear_deps_for(sheet, dest_r, dest_c);
    if (collect_deps(sheet, p, dest_r, dest_c)) {
        sheet_clear_deps_for(sheet, dest_r, dest_c);
        restore_deps_for(sheet, old_deps, old_dep_count);
        return "Invalid cell";
    }

    /* Circular dependency check: if dest is reachable from itself, reject */
    if (has_cycle(sheet, dest_r, dest_c)) {
        /* Roll back: remove the deps we just added for this cell */
        sheet_clear_deps_for(sheet, dest_r, dest_c);
        restore_deps_for(sheet, old_deps, old_dep_count);
        return "circular dependency";
    }

    /* Evaluate the expression */
    const char *expr_end = NULL;
    Eval result = eval_expr(sheet, p, &expr_end);

    /* The expression must consume the entire right-hand side */
    if (expr_end == NULL || *expr_end != '\0') {
        sheet_clear_deps_for(sheet, dest_r, dest_c);
        restore_deps_for(sheet, old_deps, old_dep_count);
        return "unrecognized cmd";
    }

    if (result.err != EVAL_OK) {
        if (result.err == EVAL_INVALID_CELL) {
            sheet_clear_deps_for(sheet, dest_r, dest_c);
            restore_deps_for(sheet, old_deps, old_dep_count);
            return "Invalid cell";
        }
        if (result.err == EVAL_SYNTAX) {
            sheet_clear_deps_for(sheet, dest_r, dest_c);
            restore_deps_for(sheet, old_deps, old_dep_count);
            return "unrecognized cmd";
        }
        /* Save the formula text and mark has_formula before error propagation */
        Cell *cell = get_cell(sheet, dest_r, dest_c);
        if (!cell) {
            sheet_clear_deps_for(sheet, dest_r, dest_c);
            restore_deps_for(sheet, old_deps, old_dep_count);
            return "Invalid cell";
        }
        strncpy(cell->formula_text, input, sizeof(cell->formula_text) - 1);
        cell->formula_text[sizeof(cell->formula_text) - 1] = '\0';
        cell->has_formula = has_cell_ref(p);
        sheet_set_cell_error(sheet, dest_r, dest_c);
        /* Propagate ERR to dependents */
        sheet_recalc(sheet, dest_r, dest_c);
        free(old_deps);
        if (result.err == EVAL_DIV_BY_ZERO) return "div by zero";
        return "div by zero";
    }

    /* Save the formula text and mark has_formula */
    Cell *cell = get_cell(sheet, dest_r, dest_c);
    if (!cell) {
        sheet_clear_deps_for(sheet, dest_r, dest_c);
        restore_deps_for(sheet, old_deps, old_dep_count);
        return "Invalid cell";
    }
    strncpy(cell->formula_text, input, sizeof(cell->formula_text) - 1);
    cell->formula_text[sizeof(cell->formula_text) - 1] = '\0';
    cell->has_formula = has_cell_ref(p);

    sheet_set_cell_value(sheet, dest_r, dest_c, result.value);

    /* Trigger recalculation of all cells that depend on dest */
    sheet_recalc(sheet, dest_r, dest_c);
    free(old_deps);

    return "ok";
}