#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sheet.h"

/* ------------------------------------------------------------------ */
/*  Internal helper: convert 0-based column index to letter name      */
/* ------------------------------------------------------------------ */
static void col_index_to_name(int col, char *buf) {
    /* col is 0-based; treat as bijective base-26 */
    col++;  /* 1-based */
    char tmp[4];
    int len = 0;
    while (col > 0) {
        col--;
        tmp[len++] = 'A' + (col % 26);
        col /= 26;
    }
    /* reverse */
    for (int i = 0; i < len; i++)
        buf[i] = tmp[len - 1 - i];
    buf[len] = '\0';
}

/* ------------------------------------------------------------------ */
/*  Sheet lifecycle                                                    */
/* ------------------------------------------------------------------ */

Sheet* sheet_create(int rows, int cols) {
    Sheet *sheet = malloc(sizeof(Sheet));
    if (!sheet) return NULL;
 
    sheet->rows = rows;
    sheet->cols = cols;
    sheet->view_row = 0;
    sheet->view_col = 0;
    sheet->output_enabled = 1;
    sheet->cells = calloc(rows * cols, sizeof(Cell));
    if (!sheet->cells) {
        free(sheet);
        return NULL;
    }
 
    /* Dependency graph */
    sheet->dep_count = 0;
    sheet->dep_cap   = 64;
    sheet->deps      = malloc(sheet->dep_cap * sizeof(DepEdge));
    if (!sheet->deps) {
        free(sheet->cells);
        free(sheet);
        return NULL;
    }
 
    return sheet;
}

void sheet_destroy(Sheet *sheet) {
    free(sheet->cells);
    free(sheet->deps);
    free(sheet);
}

/* ------------------------------------------------------------------ */
/*  Cell accessors                                                     */
/* ------------------------------------------------------------------ */

void sheet_set_cell_value(Sheet *sheet, int r, int c, int value) {
    if (r < 0 || r >= sheet->rows || c < 0 || c >= sheet->cols) return;
    Cell *cell = &sheet->cells[r * sheet->cols + c];
    cell->type = CELL_VALUE;
    cell->value = value;
    cell->is_error = 0;
}

void sheet_set_cell_error(Sheet *sheet, int r, int c) {
    if (r < 0 || r >= sheet->rows || c < 0 || c >= sheet->cols) return;
    Cell *cell = &sheet->cells[r * sheet->cols + c];
    cell->type = CELL_ERR;
    cell->is_error = 1;
}

Cell *get_cell(Sheet *sheet, int r, int c) {
    if (r < 0 || r >= sheet->rows || c < 0 || c >= sheet->cols) return NULL;
    return &sheet->cells[r * sheet->cols + c];
}

/* ------------------------------------------------------------------ */
/*  Printing                                                           */
/* ------------------------------------------------------------------ */

/*
 * print_sheet:
 *   Displays the current 10x10 view window with column headers and
 *   row numbers, matching the assignment's example output format.
 */
void print_sheet(Sheet *sheet) {
    if (!sheet->output_enabled) return;

    int row_end = sheet->rows < sheet->view_row + 10
                  ? sheet->rows : sheet->view_row + 10;
    int col_end = sheet->cols < sheet->view_col + 10
                  ? sheet->cols : sheet->view_col + 10;

    /* Column header row */
    printf("   ");  /* space for row-number column */
    for (int c = sheet->view_col; c < col_end; c++) {
        char name[4];
        col_index_to_name(c, name);
        printf("%6s", name);
    }
    printf("\n");

    /* Data rows */
    for (int r = sheet->view_row; r < row_end; r++) {
        printf("%-3d", r + 1);  /* 1-based row number */
        for (int c = sheet->view_col; c < col_end; c++) {
            Cell *cell = &sheet->cells[r * sheet->cols + c];
            if (cell->type == CELL_ERR || cell->is_error) {
                printf("%6s", "ERR");
            } else {
                printf("%6d", cell->value);
            }
        }
        printf("\n");
    }
}

/* ------------------------------------------------------------------ */
/*  Scrolling                                                          */
/* ------------------------------------------------------------------ */

void scroll_sheet(Sheet *sheet, int dr, int dc) {
    sheet->view_row += dr;
    sheet->view_col += dc;
    if (sheet->view_row < 0) sheet->view_row = 0;
    if (sheet->view_col < 0) sheet->view_col = 0;
    if (sheet->view_row > sheet->rows - 10) sheet->view_row = sheet->rows - 10;
    if (sheet->view_col > sheet->cols - 10) sheet->view_col = sheet->cols - 10;
    /* Clamp again in case sheet is smaller than 10 */
    if (sheet->view_row < 0) sheet->view_row = 0;
    if (sheet->view_col < 0) sheet->view_col = 0;
}

void scroll_to_cell(Sheet *sheet, int r, int c) {
    if (r < 0 || r >= sheet->rows || c < 0 || c >= sheet->cols) return;
    sheet->view_row = r;
    sheet->view_col = c;
}

/* ------------------------------------------------------------------ */
/*  Dependency graph                                                   */
/* ------------------------------------------------------------------ */
 
/*
 * sheet_add_dep:
 *   Records that (to_r, to_c) depends on (from_r, from_c).
 *   Grows the edge array if needed.
 */
static void ensure_dep_capacity(Sheet *sheet) {
    if (sheet->dep_count == sheet->dep_cap) {
        sheet->dep_cap *= 2;
        sheet->deps = realloc(sheet->deps, sheet->dep_cap * sizeof(DepEdge));
    }
}

void sheet_add_dep(Sheet *sheet, int from_r, int from_c, int to_r, int to_c) {
    ensure_dep_capacity(sheet);
    DepEdge *e = &sheet->deps[sheet->dep_count++];
    e->from_r = from_r;
    e->from_c = from_c;
    e->from_r2 = from_r;
    e->from_c2 = from_c;
    e->to_r   = to_r;
    e->to_c   = to_c;
    e->is_range = 0;
}

void sheet_add_dep_range(Sheet *sheet, int from_r, int from_c,
                        int from_r2, int from_c2,
                        int to_r, int to_c) {
    ensure_dep_capacity(sheet);
    DepEdge *e = &sheet->deps[sheet->dep_count++];
    e->from_r = from_r;
    e->from_c = from_c;
    e->from_r2 = from_r2;
    e->from_c2 = from_c2;
    e->to_r   = to_r;
    e->to_c   = to_c;
    e->is_range = 1;
}
 
/*
 * sheet_clear_deps_for:
 *   Removes all edges whose destination is (to_r, to_c).
 *   Used when a cell is reassigned so old dependencies are forgotten.
 */
void sheet_clear_deps_for(Sheet *sheet, int to_r, int to_c) {
    int write = 0;
    for (int i = 0; i < sheet->dep_count; i++) {
        DepEdge *e = &sheet->deps[i];
        if (e->to_r == to_r && e->to_c == to_c) continue;  /* drop it */
        sheet->deps[write++] = *e;
    }
    sheet->dep_count = write;
}
 
/* ------------------------------------------------------------------ */
/*  Recalculation (topological BFS)                                   */
/* ------------------------------------------------------------------ */
 
/*
 * sheet_recalc:
 *   After cell (start_r, start_c) changes, find all cells that (transitively)
 *   depend on it and re-evaluate them in topological order (BFS over the
 *   dependency graph).
 *
 *   Re-evaluation is done by calling parse_and_execute on the stored
 *   formula_text of each dependent cell (forward declaration below).
 */
 
/* Forward declaration — implemented in parser.c */
const char *parse_and_execute(Sheet *sheet, const char *input, int *sr, int *sc);
 
const char *sheet_recalc(Sheet *sheet, int start_r, int start_c) {
    int total = sheet->rows * sheet->cols;
 
    /* BFS queue — stores flat cell indices */
    int *queue   = malloc(total * sizeof(int));
    unsigned char *visited = calloc(total, sizeof(unsigned char));
    if (!queue || !visited) { free(queue); free(visited); return "ok"; }
 
    int head = 0, tail = 0;
 
    /* Seed: direct dependents of the changed cell */
    for (int i = 0; i < sheet->dep_count; i++) {
        DepEdge *e = &sheet->deps[i];
        if (e->is_range) {
            if (start_r >= e->from_r && start_r <= e->from_r2 &&
                start_c >= e->from_c && start_c <= e->from_c2) {
                int idx = e->to_r * sheet->cols + e->to_c;
                if (!visited[idx]) {
                    visited[idx] = 1;
                    queue[tail++] = idx;
                }
            }
        } else {
            if (e->from_r == start_r && e->from_c == start_c) {
                int idx = e->to_r * sheet->cols + e->to_c;
                if (!visited[idx]) {
                    visited[idx] = 1;
                    queue[tail++] = idx;
                }
            }
        }
    }
 
    /* BFS */
    while (head < tail) {
        int idx = queue[head++];
        int r = idx / sheet->cols;
        int c = idx % sheet->cols;
 
        Cell *cell = get_cell(sheet, r, c);
        if (!cell || !cell->has_formula) continue;
 
        /* Re-evaluate the formula.  parse_and_execute updates the cell. */
        int dummy_sr = 0, dummy_sc = 0;
        parse_and_execute(sheet, cell->formula_text, &dummy_sr, &dummy_sc);
 
        /* Enqueue this cell's own dependents */
        for (int i = 0; i < sheet->dep_count; i++) {
            DepEdge *e = &sheet->deps[i];
            int nidx = -1;
            if (e->is_range) {
                if (r >= e->from_r && r <= e->from_r2 &&
                    c >= e->from_c && c <= e->from_c2) {
                    nidx = e->to_r * sheet->cols + e->to_c;
                }
            } else if (e->from_r == r && e->from_c == c) {
                nidx = e->to_r * sheet->cols + e->to_c;
            }
            if (nidx >= 0 && !visited[nidx]) {
                visited[nidx] = 1;
                queue[tail++] = nidx;
            }
        }
    }
 
    free(queue);
    free(visited);
    return "ok";
}