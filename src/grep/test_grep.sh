#!/bin/sh

export LC_ALL=C

PASS=0
FAIL=0

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT HUP INT TERM

FILE1="$TMP_DIR/file1.txt"
FILE2="$TMP_DIR/file2.txt"
EMPTY="$TMP_DIR/empty.txt"
SPACE_FILE="$TMP_DIR/file with spaces.txt"
OVERLAP="$TMP_DIR/overlap.txt"

PATTERNS1="$TMP_DIR/patterns1.txt"
PATTERNS2="$TMP_DIR/patterns2.txt"
EMPTY_PATTERNS="$TMP_DIR/empty_patterns.txt"
BLANK_PATTERN="$TMP_DIR/blank_pattern.txt"
NO_NEWLINE_PATTERN="$TMP_DIR/no_newline_pattern.txt"

MISSING="$TMP_DIR/missing.txt"

printf 'alpha one\nAlpha two\nbeta\nalpha alpha\n\n-alpha\n' > "$FILE1"
printf 'gamma\nALPHA\nalpha beta' > "$FILE2"
: > "$EMPTY"
printf 'alpha in spaced file\nbeta\n' > "$SPACE_FILE"
printf 'aaaa\nbaaa\n' > "$OVERLAP"

printf 'alpha\nbeta\n' > "$PATTERNS1"
printf 'gamma\n' > "$PATTERNS2"
: > "$EMPTY_PATTERNS"
printf '\n' > "$BLANK_PATTERN"
printf 'alpha' > "$NO_NEWLINE_PATTERN"

RunTest() {
  name="$1"
  shift

  ./s21_grep "$@" > "$TMP_DIR/actual.out" 2> "$TMP_DIR/actual.err"
  actual_status=$?

  grep "$@" > "$TMP_DIR/expected.out" 2> "$TMP_DIR/expected.err"
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

# Все 256 комбинаций флагов:
# -i -v -c -l -n -h -s -o
mask=0
while [ "$mask" -lt 256 ]; do
  combo="-"
  bit=1

  for flag in i v c l n h s o; do
    if [ $((mask & bit)) -ne 0 ]; then
      combo="${combo}${flag}"
    fi
    bit=$((bit * 2))
  done

  if [ "$mask" -eq 0 ]; then
    RunTest "no flags" alpha "$FILE1" "$FILE2"
  else
    RunTest "flags $combo" "$combo" alpha "$FILE1" "$FILE2"
  fi

  mask=$((mask + 1))
done

# Несколько флагов отдельными аргументами.
RunTest "separate -i -n" -i -n alpha "$FILE1" "$FILE2"
RunTest "separate -v -c" -v -c alpha "$FILE1" "$FILE2"
RunTest "separate -l -h" -l -h alpha "$FILE1" "$FILE2"
RunTest "mixed grouped and separate" -in -v alpha "$FILE1" "$FILE2"

# -e: отдельный, приклеенный и несколько шаблонов.
RunTest "-e separate" -e alpha "$FILE1" "$FILE2"
RunTest "-e attached" -ealpha "$FILE1" "$FILE2"
RunTest "multiple -e" -e alpha -e beta "$FILE1" "$FILE2"
RunTest "-e with flags" -in -e alpha "$FILE1" "$FILE2"
RunTest "empty -e pattern" -e "" "$FILE1"

# -o с несколькими шаблонами.
RunTest "-o multiple -e a then aa" -o -e a -e aa "$OVERLAP"
RunTest "-o multiple -e aa then a" -o -e aa -e a "$OVERLAP"
RunTest "-o empty pattern" -o -e "" "$FILE1"

# -f и комбинации источников шаблонов.
RunTest "-f separate" -f "$PATTERNS1" "$FILE1" "$FILE2"
RunTest "-f attached" "-f$PATTERNS1" "$FILE1" "$FILE2"
RunTest "multiple -f" -f "$PATTERNS1" -f "$PATTERNS2" "$FILE1" "$FILE2"
RunTest "-e and -f" -e gamma -f "$PATTERNS1" "$FILE1" "$FILE2"
RunTest "empty pattern file" -f "$EMPTY_PATTERNS" "$FILE1"
RunTest "blank pattern file" -f "$BLANK_PATTERN" "$FILE1"
RunTest "pattern file without newline" -f "$NO_NEWLINE_PATTERN" "$FILE1"

# Основные варианты BRE-регулярных выражений.
RunTest "regex start" '^alpha' "$FILE1"
RunTest "regex end" 'beta$' "$FILE1" "$FILE2"
RunTest "regex character class" '[Aa]lpha' "$FILE1"
RunTest "regex dot star" 'a.*a' "$FILE1"
RunTest "regex empty line" '^$' "$FILE1"
RunTest "regex repetition" 'a\{2\}' "$OVERLAP"
RunTest "invalid regex" '[' "$FILE1"

# Разные входные данные.
RunTest "empty input file" alpha "$EMPTY"
RunTest "filename with spaces" alpha "$SPACE_FILE"
RunTest "double dash" -- alpha "$FILE1"
RunTest "pattern beginning with dash" -- -alpha "$FILE1"

# Ошибки.
RunTest "missing input file" alpha "$MISSING"
RunTest "missing then existing" alpha "$MISSING" "$FILE1"
RunTest "-s missing input" -s alpha "$MISSING"
RunTest "missing pattern file" -f "$MISSING" "$FILE1"
RunTest "-s missing pattern file" -s -f "$MISSING" "$FILE1"
RunTest "missing -e argument" -e
RunTest "missing -f argument" -f
RunTest "missing pattern" 
RunTest "invalid option" -Q alpha "$FILE1"

echo "grep tests: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
