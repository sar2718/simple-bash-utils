#!/bin/sh

export LC_ALL=C

PASS=0
FAIL=0

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT HUP INT TERM

MAIN="$TMP_DIR/main.txt"
EMPTY="$TMP_DIR/empty.txt"
NO_NEWLINE="$TMP_DIR/no_newline.txt"
FIRST="$TMP_DIR/first.txt"
SECOND="$TMP_DIR/second.txt"
MISSING="$TMP_DIR/missing.txt"

printf 'alpha\n\n\nbeta\tX\nA\001B\177C\ncarriage\r\nlast\n' > "$MAIN"
: > "$EMPTY"
printf 'without final newline' > "$NO_NEWLINE"
printf 'first\n\n\n' > "$FIRST"
printf '\n\nsecond\n' > "$SECOND"

RunTest() {
  name="$1"
  shift

  ./s21_cat "$@" > "$TMP_DIR/actual.out" 2> "$TMP_DIR/actual.err"
  actual_status=$?

  cat "$@" > "$TMP_DIR/expected.out" 2> "$TMP_DIR/expected.err"
  expected_status=$?

  actual_has_error=0
  expected_has_error=0
  [ -s "$TMP_DIR/actual.err" ] && actual_has_error=1
  [ -s "$TMP_DIR/expected.err" ] && expected_has_error=1

  if cmp -s "$TMP_DIR/actual.out" "$TMP_DIR/expected.out" &&
     [ "$actual_status" -eq "$expected_status" ] &&
     [ "$actual_has_error" -eq "$expected_has_error" ]; then
    PASS=$((PASS + 1))
  else
    echo "FAIL: $name"
    cmp -s "$TMP_DIR/actual.out" "$TMP_DIR/expected.out" ||
      echo "  stdout differs"
    [ "$actual_status" -eq "$expected_status" ] ||
      echo "  exit code: got $actual_status, expected $expected_status"
    [ "$actual_has_error" -eq "$expected_has_error" ] ||
      echo "  stderr presence differs"
    FAIL=$((FAIL + 1))
  fi
}

# Все 127 непустых комбинаций коротких флагов:
# -b -e -E -n -s -t -T
mask=1
while [ "$mask" -lt 128 ]; do
  combo="-"
  bit=1

  for flag in b e E n s t T; do
    if [ $((mask & bit)) -ne 0 ]; then
      combo="${combo}${flag}"
    fi
    bit=$((bit * 2))
  done

  RunTest "flags $combo" "$combo" "$MAIN"
  mask=$((mask + 1))
done

# Те же опции, переданные отдельными аргументами.
RunTest "separate -b -n" -b -n "$MAIN"
RunTest "separate -e -t" -e -t "$MAIN"
RunTest "separate -E -T -s" -E -T -s "$MAIN"

# GNU long options и их комбинации.
RunTest "--number" --number "$MAIN"
RunTest "--number-nonblank" --number-nonblank "$MAIN"
RunTest "--squeeze-blank" --squeeze-blank "$MAIN"
RunTest "long number + squeeze" --number --squeeze-blank "$MAIN"
RunTest "long nonblank + squeeze" --number-nonblank --squeeze-blank "$MAIN"
RunTest "long number + nonblank" --number --number-nonblank "$MAIN"
RunTest "all long options" --number --number-nonblank --squeeze-blank "$MAIN"

# Разные входные данные.
RunTest "empty file" "$EMPTY"
RunTest "no final newline" "$NO_NEWLINE"
RunTest "multiple files" "$MAIN" "$NO_NEWLINE"
RunTest "squeeze across files" -s "$FIRST" "$SECOND"
RunTest "number across files" -n "$FIRST" "$SECOND"

# Парсер и ошибки.
RunTest "double dash" -- "$MAIN"
RunTest "missing file" "$MISSING"
RunTest "missing then existing" "$MISSING" "$MAIN"
RunTest "invalid option" -q "$MAIN"

echo "cat tests: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
