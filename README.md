# Simple Bash Utils

A C11 implementation of two common Unix command-line text utilities:

- `s21_cat` — displays and concatenates file contents;
- `s21_grep` — searches text using regular expressions.

The project focuses on reproducing the behavior of standard Unix utilities, including command-line flag parsing, file processing, regular expressions, error handling, and integration testing.

## Features

### s21_cat

Supported options:

- `-b`, `--number-nonblank` — number non-empty output lines;
- `-e` — display line endings as `$` and show non-printing characters;
- `-E` — display line endings as `$`;
- `-n`, `--number` — number all output lines;
- `-s`, `--squeeze-blank` — suppress repeated empty lines;
- `-t` — display tabs as `^I` and show non-printing characters;
- `-T` — display tabs as `^I`.

The implementation also handles multiple input files as a continuous output stream and preserves numbering and blank-line state between files.

### s21_grep

Supported options:

- `-e` — specify a search pattern;
- `-i` — ignore case;
- `-v` — invert matching;
- `-c` — print only the number of selected lines;
- `-l` — print only names of files containing matches;
- `-n` — prefix output lines with line numbers;
- `-h` — suppress file name prefixes;
- `-s` — suppress file error messages;
- `-f` — read patterns from a file;
- `-o` — print only non-empty matching parts.

Multiple patterns can be provided with both `-e` and `-f`. Regular expressions are implemented using the POSIX regex API.

## Implementation

The source code is split into three parts:

```text
src/
├── cat/
│   ├── Makefile
│   ├── cat_options.c
│   ├── cat_options.h
│   ├── s21_cat.c
│   └── test_cat.sh
├── common/
│   ├── error_code.h
│   ├── error_utils.c
│   └── error_utils.h
└── grep/
    ├── Makefile
    ├── grep_options.c
    ├── grep_options.h
    ├── s21_grep.c
    └── test_grep.sh
```

Command-line parsing is separated from file-processing logic. Shared file-error handling is placed in `src/common` to avoid code duplication.

The implementation follows a structured programming approach: no global variables, no `goto`, no `continue`, and no immediate program termination with `exit()`.

## Build

Build `cat`:

```bash
make -C src/cat
```

Build `grep`:

```bash
make -C src/grep
```

The project is compiled with GCC using C11 and strict warning flags:

```text
-std=c11 -Wall -Wextra -Werror -pedantic
```

## Testing

Both utilities include integration tests that compare their behavior with the system `cat` and `grep`.

Run:

```bash
make -C src/cat test
make -C src/grep test
```

The tests compare:

- standard output;
- exit status;
- presence of error output.

They cover all combinations of the implemented boolean flags as well as separate cases for multiple files, empty input, regular expressions, `-e`, `-f`, invalid arguments, and file errors.

Current test suites include:

```text
cat:  146 integration tests
grep: 295 integration tests
```

## Technologies

- C11
- GCC
- POSIX regular expressions
- Make
- POSIX shell
- clang-format
