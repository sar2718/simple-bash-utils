#define _POSIX_C_SOURCE 200809L

#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "../common/error_utils.h"
#include "grep_options.h"

typedef struct {
  regex_t* items;
  size_t count;
} RegexList;

typedef struct {
  size_t line_number;
  size_t selected_count;
  bool file_selected;
  bool stop;
} GrepFileState;

static int RunGrep(int argc, char* argv[], GrepOptions* options,
                   RegexList* regexes, char* operands[]);
static int ProcessFiles(char* operands[], int operands_count,
                        const GrepOptions* options, const RegexList* regexes);
static ErrorCode CompileRegexes(const GrepOptions* options, RegexList* regexes);
static void FreeRegexes(RegexList* regexes);
static ErrorCode ProcessFile(const char* file_name, const GrepOptions* options,
                             const RegexList* regexes, bool show_file_name,
                             bool* file_selected);
static ErrorCode ProcessStream(FILE* file, const char* file_name,
                               const GrepOptions* options,
                               const RegexList* regexes, bool show_file_name,
                               GrepFileState* state);
static void ProcessLine(const char* file_name, const char* line,
                        const GrepOptions* options, const RegexList* regexes,
                        bool show_file_name, GrepFileState* state);
static bool LineMatches(const char* line, const RegexList* regexes);
static void PrintSelection(const char* file_name, const char* line,
                           const GrepOptions* options, const RegexList* regexes,
                           bool show_file_name, size_t line_number);
static void PrintSelectedLine(const char* file_name, size_t line_number,
                              const char* line, bool show_file_name,
                              const GrepOptions* options);
static void PrintOnlyMatches(const char* file_name, size_t line_number,
                             const char* line, bool show_file_name,
                             const GrepOptions* options,
                             const RegexList* regexes);
static bool FindNextNonEmptyMatch(const char* line, size_t start_offset,
                                  const RegexList* regexes, size_t* match_start,
                                  size_t* match_end);
static bool FindRegexMatch(const char* line, size_t start_offset,
                           const regex_t* regex, size_t* match_start,
                           size_t* match_end);
static void PrintPrefix(const char* file_name, size_t line_number,
                        bool show_file_name, bool show_line_number);
static void PrintFileSummary(const char* file_name, const GrepOptions* options,
                             bool show_file_name, const GrepFileState* state);

int main(int argc, char* argv[]) {
  GrepOptions options = {0};
  RegexList regexes = {0};
  char* operands[argc];
  int exit_code = RunGrep(argc, argv, &options, &regexes, operands);

  FreeRegexes(&regexes);
  FreeGrepOptions(&options);

  return exit_code;
}

static int RunGrep(int argc, char* argv[], GrepOptions* options,
                   RegexList* regexes, char* operands[]) {
  int exit_code = 2;
  int operands_count = 0;
  const char* error_file_name = NULL;
  ErrorCode error =
      ParseGrepArguments(argc, argv, options, operands, &operands_count);

  if (error == kSuccess) {
    error = LoadGrepPatternFiles(options, &error_file_name);
  }
  if (error == kSuccess && operands_count == 0) {
    error = kErrorInvalidArgs;
  }
  if (error == kSuccess) {
    error = CompileRegexes(options, regexes);
  }
  if (error == kSuccess) {
    exit_code = ProcessFiles(operands, operands_count, options, regexes);
  } else {
    PrintProgramError("s21_grep", error, error_file_name);
  }

  return exit_code;
}

static int ProcessFiles(char* operands[], int operands_count,
                        const GrepOptions* options, const RegexList* regexes) {
  int exit_code = 1;
  bool has_error = false;
  bool any_selected = false;
  bool show_file_name = operands_count > 1 && !options->no_filename;

  for (int i = 0; i < operands_count; ++i) {
    bool file_selected = false;
    ErrorCode error = ProcessFile(operands[i], options, regexes, show_file_name,
                                  &file_selected);
    if (error != kSuccess) {
      has_error = true;
      if (!options->suppress_errors) {
        PrintProgramError("s21_grep", error, operands[i]);
      }
    }
    if (file_selected) {
      any_selected = true;
    }
  }
  if (has_error) {
    exit_code = 2;
  } else if (any_selected) {
    exit_code = 0;
  }

  return exit_code;
}

static ErrorCode CompileRegexes(const GrepOptions* options,
                                RegexList* regexes) {
  ErrorCode result = kSuccess;

  regexes->items = NULL;
  regexes->count = 0;
  if (options->patterns_count > 0) {
    regexes->items = calloc(options->patterns_count, sizeof(*regexes->items));
    if (regexes->items == NULL) {
      result = kErrorMemory;
    }
  }
  for (size_t i = 0; i < options->patterns_count && result == kSuccess; ++i) {
    int flags = options->ignore_case ? REG_ICASE : 0;
    int regex_result = regcomp(&regexes->items[i], options->patterns[i], flags);
    if (regex_result != 0) {
      result = kErrorRegex;
    } else {
      ++regexes->count;
    }
  }

  return result;
}

static void FreeRegexes(RegexList* regexes) {
  if (regexes != NULL) {
    for (size_t i = 0; i < regexes->count; ++i) {
      regfree(&regexes->items[i]);
    }
    free(regexes->items);
    regexes->items = NULL;
    regexes->count = 0;
  }
}

static ErrorCode ProcessFile(const char* file_name, const GrepOptions* options,
                             const RegexList* regexes, bool show_file_name,
                             bool* file_selected) {
  ErrorCode result = kSuccess;
  FILE* file = fopen(file_name, "r");
  GrepFileState state = {0, 0, false, false};

  if (file == NULL) {
    result = GetFileOpenError();
  } else {
    result = ProcessStream(file, file_name, options, regexes, show_file_name,
                           &state);
    if (result == kSuccess) {
      PrintFileSummary(file_name, options, show_file_name, &state);
    }
    fclose(file);
  }
  *file_selected = state.file_selected;

  return result;
}

static ErrorCode ProcessStream(FILE* file, const char* file_name,
                               const GrepOptions* options,
                               const RegexList* regexes, bool show_file_name,
                               GrepFileState* state) {
  ErrorCode result = kSuccess;
  char* line = NULL;
  size_t capacity = 0;
  ssize_t length = 0;

  while (!state->stop && (length = getline(&line, &capacity, file)) != -1) {
    ++state->line_number;
    if (length > 0 && line[length - 1] == '\n') {
      line[length - 1] = '\0';
    }
    ProcessLine(file_name, line, options, regexes, show_file_name, state);
  }
  if (ferror(file) != 0) {
    result = kErrorFileRead;
  }
  free(line);

  return result;
}

static void ProcessLine(const char* file_name, const char* line,
                        const GrepOptions* options, const RegexList* regexes,
                        bool show_file_name, GrepFileState* state) {
  bool matched = LineMatches(line, regexes);
  bool selected = options->invert_match ? !matched : matched;

  if (selected) {
    state->file_selected = true;
    ++state->selected_count;
    if (options->files_with_matches) {
      state->stop = true;
    } else if (!options->count_only) {
      PrintSelection(file_name, line, options, regexes, show_file_name,
                     state->line_number);
    }
  }
}

static bool LineMatches(const char* line, const RegexList* regexes) {
  bool matched = false;

  for (size_t i = 0; i < regexes->count && !matched; ++i) {
    if (regexec(&regexes->items[i], line, 0, NULL, 0) == 0) {
      matched = true;
    }
  }

  return matched;
}

static void PrintSelection(const char* file_name, const char* line,
                           const GrepOptions* options, const RegexList* regexes,
                           bool show_file_name, size_t line_number) {
  if (options->only_matching) {
    if (!options->invert_match) {
      PrintOnlyMatches(file_name, line_number, line, show_file_name, options,
                       regexes);
    }
  } else {
    PrintSelectedLine(file_name, line_number, line, show_file_name, options);
  }
}

static void PrintSelectedLine(const char* file_name, size_t line_number,
                              const char* line, bool show_file_name,
                              const GrepOptions* options) {
  PrintPrefix(file_name, line_number, show_file_name, options->line_number);
  fputs(line, stdout);
  putchar('\n');
}

static void PrintOnlyMatches(const char* file_name, size_t line_number,
                             const char* line, bool show_file_name,
                             const GrepOptions* options,
                             const RegexList* regexes) {
  size_t offset = 0;
  size_t line_length = strlen(line);
  bool done = false;

  while (offset <= line_length && !done) {
    size_t match_start = 0;
    size_t match_end = 0;
    bool found =
        FindNextNonEmptyMatch(line, offset, regexes, &match_start, &match_end);
    if (found) {
      PrintPrefix(file_name, line_number, show_file_name, options->line_number);
      fwrite(line + match_start, 1, match_end - match_start, stdout);
      putchar('\n');
      offset = match_end;
    } else {
      done = true;
    }
  }
}

static bool FindNextNonEmptyMatch(const char* line, size_t start_offset,
                                  const RegexList* regexes, size_t* match_start,
                                  size_t* match_end) {
  bool found = false;

  for (size_t i = 0; i < regexes->count; ++i) {
    size_t current_start = 0;
    size_t current_end = 0;
    bool current_found = FindRegexMatch(line, start_offset, &regexes->items[i],
                                        &current_start, &current_end);
    bool earlier = !found || current_start < *match_start;
    bool longer = current_start == *match_start && current_end > *match_end;
    if (current_found && (earlier || longer)) {
      *match_start = current_start;
      *match_end = current_end;
      found = true;
    }
  }

  return found;
}

static bool FindRegexMatch(const char* line, size_t start_offset,
                           const regex_t* regex, size_t* match_start,
                           size_t* match_end) {
  bool found = false;
  size_t line_length = strlen(line);
  size_t search_offset = start_offset;

  while (search_offset <= line_length && !found) {
    regmatch_t match = {0};
    int flags = search_offset > 0 ? REG_NOTBOL : 0;
    int regex_result = regexec(regex, line + search_offset, 1, &match, flags);
    if (regex_result == 0) {
      size_t current_start = search_offset + (size_t)match.rm_so;
      size_t current_end = search_offset + (size_t)match.rm_eo;
      if (current_end > current_start) {
        *match_start = current_start;
        *match_end = current_end;
        found = true;
      } else if (current_start < line_length) {
        search_offset = current_start + 1;
      } else {
        search_offset = line_length + 1;
      }
    } else {
      search_offset = line_length + 1;
    }
  }

  return found;
}

static void PrintPrefix(const char* file_name, size_t line_number,
                        bool show_file_name, bool show_line_number) {
  if (show_file_name) {
    printf("%s:", file_name);
  }
  if (show_line_number) {
    printf("%zu:", line_number);
  }
}

static void PrintFileSummary(const char* file_name, const GrepOptions* options,
                             bool show_file_name, const GrepFileState* state) {
  if (options->files_with_matches && state->file_selected) {
    printf("%s\n", file_name);
  } else if (options->count_only) {
    if (show_file_name) {
      printf("%s:", file_name);
    }
    printf("%zu\n", state->selected_count);
  }
}
