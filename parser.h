#ifndef PARSER_H
#define PARSER_H

#include "sheet.h"

/*
 * parse_and_execute:
 *   Takes a raw input string and the sheet, tries to interpret it as a formula
 *   assignment (e.g. "A1=B2+3", "C3=SUM(A1:B2)").
 *
 *   Returns a status string:
 *     "ok"              – success
 *     "unrecognized cmd"– not a valid formula
 *     "Invalid range"   – range specifier is backwards / out of bounds
 *     "Invalid cell"    – cell reference is out of sheet bounds
 *     "div by zero"     – division by zero occurred
 *     "scroll_to"       – caller should handle scroll_to (cell already parsed)
 *
 *   On a scroll_to command the two output parameters *sr and *sc are filled
 *   with the 0-based row/column of the target cell.
 */
const char *parse_and_execute(Sheet *sheet, const char *input,
                              int *sr, int *sc);

/*
 * col_name_to_index:
 *   Converts a column name like "A" -> 0, "Z" -> 25, "AA" -> 26, etc.
 *   Returns -1 if the string is not a valid column name.
 */
int col_name_to_index(const char *s, int len);

/*
 * parse_cell_name:
 *   Parses a cell name like "A1" or "ZZZ999" from the start of *p.
 *   Advances *p past the parsed name.
 *   Stores 0-based row/col in *r, *c.
 *   Returns 1 on success, 0 on failure.
 */
int parse_cell_name(const char **p, int *r, int *c);

#endif /* PARSER_H */
