.PHONY: report test clean

sheet: main.o sheet.o parser.o
	gcc  main.o sheet.o parser.o -o sheet -lm

main.o: main.c sheet.h parser.h
	gcc  -c main.c

sheet.o: sheet.c sheet.h
	gcc  -c sheet.c

parser.o: parser.c parser.h sheet.h
	gcc  -c parser.c

clean:
	rm -f *.o sheet
	rm -f report/report.pdf
	rm -f report/report.aux
	rm -f report/report.out
	rm -f report/report.toc
	rm -f report/report.log
	rm -f tests/test_basic
	rm -f tests/test_scroll_to
	rm -f tests/test_arithmetic
	rm -f tests/test_runner
	rm -f tests/test_sum_function
	rm -f tests/test_invalid_range
	rm -f tests/test_invalid_cell_range
	rm -f tests/test_recalc
	rm -f tests/test_div_zero_propagation
	rm -f tests/output_arith.txt
	rm -f tests/output_basic.txt
	rm -f tests/output_scroll.txt
	rm -f tests/output_arith.txt
	rm -f tests/output_sum_function.txt
	rm -f tests/output_invalid_range.txt
	rm -f tests/output_invalid_cell_range.txt
	rm -f tests/output_recalc.txt
	rm -f tests/output_div_zero_propagation.txt


report:
	cd report && pdflatex report.tex && pdflatex report.tex

test_basic: tests/test_basic.c
	gcc tests/test_basic.c -o tests/test_basic

test_scroll_to: tests/test_scroll_to.c
	gcc tests/test_scroll_to.c -o tests/test_scroll_to

test_arithmetic: tests/test_arithmetic.c
	gcc tests/test_arithmetic.c -o tests/test_arithmetic

test_runner: tests/test_runner.c
	gcc tests/test_runner.c -o tests/test_runner

test_sum_function: tests/test_sum_function.c
	gcc tests/test_sum_function.c -o tests/test_sum_function

test_invalid_range: tests/test_invalid_range.c
	gcc tests/test_invalid_range.c -o tests/test_invalid_range

test_recalc: tests/test_recalc.c
	gcc tests/test_recalc.c -o tests/test_recalc

test_invalid_cell_range: tests/test_invalid_cell_range.c
	gcc tests/test_invalid_cell_range.c -o tests/test_invalid_cell_range

test_div_zero_propagation: tests/test_div_zero_propagation.c
	gcc tests/test_div_zero_propagation.c -o tests/test_div_zero_propagation

test: sheet test_basic test_scroll_to test_arithmetic test_sum_function test_invalid_range test_recalc test_invalid_cell_range test_div_zero_propagation test_runner
	./tests/test_runner
