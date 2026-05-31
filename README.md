## Command-Line Spreadsheet Engine in C

Developed a fully functional **spreadsheet application in C** featuring formula evaluation, dependency tracking, automatic recalculation, and error propagation. The program provides a command-line interface that allows users to create and interact with spreadsheets containing up to **999 rows and 18,278 columns (A–ZZZ)**.

### Key Features

* Supports direct cell assignments (e.g., `A1=10`).
* Evaluates arithmetic expressions involving constants and cell references.
* Implements spreadsheet functions:

  * `SUM`
  * `AVG`
  * `MIN`
  * `MAX`
  * `STDEV`
  * `SLEEP`
* Supports both one-dimensional and two-dimensional cell ranges.
* Automatic recalculation of dependent cells when referenced values change.
* Efficient dependency graph management to avoid unnecessary recomputations.
* Scrollable spreadsheet view for large sheets.
* Output suppression and navigation commands for improved usability.

### Error Handling

* Invalid commands and malformed expressions.
* Out-of-bounds cell references.
* Division-by-zero detection with error propagation.
* Circular dependency detection and prevention.
* Robust handling of invalid formulas and ranges.

### Technical Highlights

* Expression parsing and evaluation.
* Dependency graph construction and traversal.
* Incremental recalculation algorithms.
* Error propagation across dependent cells.
* Memory-efficient management of large spreadsheets.
* Modular C design with automated testing support.

### Skills Demonstrated

* C Programming
* Data Structures and Algorithms
* Dependency Graphs
* Parsing and Expression Evaluation
* Software Design and Testing
* Performance Optimization
* Error Handling and Validation

This project was completed as part of a systems programming laboratory and demonstrates the implementation of core spreadsheet functionality, similar to that found in modern spreadsheet applications, entirely from scratch in C.
This project was done in collaboration with Abhiramachandra Vemparala (abhivemparala on GitHub)
