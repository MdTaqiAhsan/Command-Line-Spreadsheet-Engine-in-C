#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sheet.h"
#include <stdbool.h>
#include "parser.h"

double time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: ./sheet R C\n");
        return 1;
    }

    /*Adding a validity checking loop to make sure argments are +ve ints */
    for (int i = 0; argv[1][i]; i++)
        if (argv[1][i] < '0' || argv[1][i] > '9') {
            printf("Usage: ./sheet R C\n");
            return 1;
        }
    for (int i = 0; argv[2][i]; i++)
        if (argv[2][i] < '0' || argv[2][i] > '9') {
            printf("Usage: ./sheet R C\n");
            return 1;
        }

    int R = atoi(argv[1]);
    int C = atoi(argv[2]);

    /*Making sure the row index and col index are in range*/
    if (R < 1 || R > 999 || C < 1 || C > 18278) {
        printf("Sheet size out of range (1<=R<=999, 1<=C<=18278)\n");
        return 1;
    }

    char input[256];
    double elapsed = 0.0;
    const char *status = "ok";
    int sheet_output_enabled = 1;
    int sheet_output_skip=0;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    Sheet *sheet = sheet_create(R, C);
    if (!sheet) {
        printf("Failed to create sheet\n");
        return 1;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = time_diff(start, end);
    

    while (true) {
        /*Printing sheet*/
        if(sheet_output_enabled==1 && sheet_output_skip==0){
            print_sheet(sheet);
        }
        sheet_output_skip=0;

        /*Printing time taken to execute last instruction and taking new instruction*/
        printf("[%.1f] (%s) > ", elapsed, status);
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = '\0';     //remove trailing newline

        /* Time the command */
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        /* ---- Built-in control commands ---- */
        if (strcmp(input, "q") == 0) {
            break;
        } 
        else if(strcmp(input, "disable_output") == 0){
            sheet_output_enabled=0;
            status="ok";
        }
        else if(strcmp(input, "enable_output") == 0){
            sheet_output_enabled=1;
            sheet_output_skip=1;
            status="ok";
        }
        else if(strcmp(input, "w") == 0){
            scroll_sheet(sheet, -10, 0);
            status="ok";
        }
        else if(strcmp(input, "a") == 0){
            scroll_sheet(sheet, 0, -10);
            status="ok";
        }
        else if(strcmp(input, "s") == 0){
            scroll_sheet(sheet, 10, 0);
            status="ok";
        }
        else if(strcmp(input, "d") == 0){
            scroll_sheet(sheet, 0, 10);
            status="ok";
        }
        else {
            /* Try to parse as formula or scroll_to */
            int sr = 0, sc = 0;
            const char *res = parse_and_execute(sheet, input, &sr, &sc);

            if (strcmp(res, "scroll_to") == 0) {
                scroll_to_cell(sheet, sr, sc);
                status = "ok";
            } else {
                status = res;
            }
        }
        //Ending time
        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed = time_diff(start, end);
        
    }

    sheet_destroy(sheet);
    return 0;
}
