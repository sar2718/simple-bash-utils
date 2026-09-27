#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "../common/error_utils.h"
#include "cat_options.h"

typedef struct {
  size_t line_number;
  bool at_line_start;
  bool previous_blank;
} CatState;

static ErrorCode ProcessFile(const char* file_name, const CatOptions* options,
                             CatState* state);
static ErrorCode ProcessStream(FILE* file, const CatOptions* options,
                               CatState* state);
static void ProcessCharacter(int ch, const CatOptions* options,
                             CatState* state);
static void ProcessCrBeforeNewline(const CatOptions* options, CatState* state);
static bool ShouldNumberLine(bool blank_line, const CatOptions* options,
                             const CatState* state);
static void UpdateState(int ch, bool blank_line, CatState* state);
static void PrintLineNumber(CatState* state);
static void PrintCharacter(unsigned char ch, const CatOptions* options);
static void PrintNonprinting(unsigned char ch);

int main(int argc, char* argv[]) {
  int exit_code = 0;
  CatOptions options = {0};
  char* operands[argc];
  int operands_count = 0;
  ErrorCode error =
      ParseCatArguments(argc, argv, &options, operands, &operands_count);

  if (error != kSuccess) {
    PrintProgramError("s21_cat", error, NULL);
    exit_code = 1;
  } else {
    CatState state = {1, true, false};
    for (int i = 0; i < operands_count; ++i) {
      error = ProcessFile(operands[i], &options, &state);
      if (error != kSuccess) {
        PrintProgramError("s21_cat", error, operands[i]);
        exit_code = 1;
      }
    }
  }

  return exit_code;
}

static ErrorCode ProcessFile(const char* file_name, const CatOptions* options,
                             CatState* state) {
  ErrorCode result = kSuccess;
  FILE* file = fopen(file_name, "r");

  if (file == NULL) {
    result = GetFileOpenError();
  } else {
    result = ProcessStream(file, options, state);
    fclose(file);
  }

  return result;
}

static ErrorCode ProcessStream(FILE* file, const CatOptions* options,
                               CatState* state) {
  ErrorCode result = kSuccess;
  bool pending_cr = false;
  int ch = 0;

  while ((ch = fgetc(file)) != EOF) {
    if (pending_cr) {
      if (ch == '\n') {
        ProcessCrBeforeNewline(options, state);
      } else {
        ProcessCharacter('\r', options, state);
      }
      pending_cr = false;
    }
    if (ch == '\r' && options->show_ends && !options->show_nonprinting) {
      pending_cr = true;
    } else {
      ProcessCharacter(ch, options, state);
    }
  }
  if (pending_cr) {
    ProcessCharacter('\r', options, state);
  }
  if (ferror(file) != 0) {
    result = kErrorFileRead;
  }

  return result;
}

static void ProcessCharacter(int ch, const CatOptions* options,
                             CatState* state) {
  bool blank_line = state->at_line_start && ch == '\n';
  bool skip_line =
      options->squeeze_blank && blank_line && state->previous_blank;

  if (!skip_line) {
    if (ShouldNumberLine(blank_line, options, state)) {
      PrintLineNumber(state);
    }
    PrintCharacter((unsigned char)ch, options);
  }
  UpdateState(ch, blank_line, state);
}

static void ProcessCrBeforeNewline(const CatOptions* options, CatState* state) {
  if (ShouldNumberLine(false, options, state)) {
    PrintLineNumber(state);
  }
  fputs("^M", stdout);
  UpdateState('\r', false, state);
}

static bool ShouldNumberLine(bool blank_line, const CatOptions* options,
                             const CatState* state) {
  bool should_number = false;

  if (state->at_line_start) {
    if (options->number_nonblank_lines) {
      should_number = !blank_line;
    } else if (options->number_all_lines) {
      should_number = true;
    }
  }

  return should_number;
}

static void UpdateState(int ch, bool blank_line, CatState* state) {
  if (ch == '\n') {
    state->at_line_start = true;
    state->previous_blank = blank_line;
  } else {
    state->at_line_start = false;
    state->previous_blank = false;
  }
}

static void PrintLineNumber(CatState* state) {
  printf("%6zu\t", state->line_number);
  ++state->line_number;
}

static void PrintCharacter(unsigned char ch, const CatOptions* options) {
  if (ch == '\n') {
    if (options->show_ends) {
      putchar('$');
    }
    putchar('\n');
  } else if (ch == '\t') {
    if (options->show_tabs) {
      fputs("^I", stdout);
    } else {
      putchar('\t');
    }
  } else if (options->show_nonprinting) {
    PrintNonprinting(ch);
  } else {
    putchar(ch);
  }
}

static void PrintNonprinting(unsigned char ch) {
  unsigned char value = ch;

  if (value >= 128) {
    fputs("M-", stdout);
    value = (unsigned char)(value - 128);
  }
  if (value < 32) {
    putchar('^');
    putchar((int)(value + 64));
  } else if (value == 127) {
    fputs("^?", stdout);
  } else {
    putchar(value);
  }
}
