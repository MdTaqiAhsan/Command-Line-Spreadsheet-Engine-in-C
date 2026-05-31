#ifndef SHEET_H
#define SHEET_H

#include <stddef.h>

typedef enum {
    CELL_VALUE,
    CELL_ERR
} CellType;

typedef struct {
    CellType type;
    int value;              // evaluated value
    char formula_text[256]; // original formula if any
    int is_error;           // 1 if ERR
    int has_formula;        // 1 if this cell was set via a formula (not a plain constant)
} Cell;

/*
 * One directed edge in the dependency graph:
 *   "cell (from_r, from_c) is used by cell (to_r, to_c)"
 * So when (from_r, from_c) changes, (to_r, to_c) must be recalculated.
 */
typedef struct {
    int from_r, from_c;   // top-left of source range
    int from_r2, from_c2; // bottom-right of source range
    int to_r,   to_c;     // the cell that depends on it
    int is_range;         // 1 if this edge represents a range dependency
} DepEdge;

typedef struct {
    int rows, cols;
    int view_row, view_col;
    int output_enabled;
    Cell *cells;            // flattened 2D array
     
    /* Dependency graph (adjacency list stored as flat edge array) */
    DepEdge *deps;          // heap-allocated array of edges
    int dep_count;          // number of edges currently stored
    int dep_cap;            // allocated capacity
} Sheet;

Sheet* sheet_create(int rows, int cols);
void sheet_destroy(Sheet *sheet);
void sheet_set_cell_value(Sheet *sheet, int r, int c, int value);
void sheet_set_cell_error(Sheet *sheet, int r, int c);
void print_sheet(Sheet *sheet);
void scroll_sheet(Sheet *sheet, int dr, int dc);
void scroll_to_cell(Sheet *sheet, int r, int c);
Cell* get_cell(Sheet *sheet, int r, int c);

/*
 * sheet_add_dep:
 *   Records that cell (to_r, to_c) depends on cell (from_r, from_c).
 *   Called by the parser each time it sees a cell reference in a formula.
 */
void sheet_add_dep(Sheet *sheet, int from_r, int from_c, int to_r, int to_c);
void sheet_add_dep_range(Sheet *sheet, int from_r, int from_c,
                        int from_r2, int from_c2,
                        int to_r, int to_c);
 
/*
 * sheet_clear_deps_for:
 *   Removes all outgoing dependency edges whose destination is (to_r, to_c).
 *   Called before re-registering a cell's formula so we don't accumulate
 *   stale edges when a cell is reassigned.
 */
void sheet_clear_deps_for(Sheet *sheet, int to_r, int to_c);
 
/*
 * sheet_recalc:
 *   After cell (r, c) has been updated, propagates the change to all cells
 *   that (transitively) depend on it, in topological order.
 *   Each dependent cell is re-evaluated using its stored formula_text.
 *   Returns "ok" on success, or an error string if something goes wrong.
 */
const char *sheet_recalc(Sheet *sheet, int r, int c);
 
#endif // SHEET_H
