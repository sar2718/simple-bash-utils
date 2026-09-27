#define _POSIX_C_SOURCE 200809L

#include "grep_options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "../common/error_utils.h"

static ErrorCode ParseArgumentList(int argc, char* argv[], GrepOptions* options,
                                   char* operands[], int* operands_count);
static ErrorCode ParseShortOptions(int argc, char* argv[], int* argument_index,
                                   GrepOptions* options);
static ErrorCode ParseValueOption(int argc, char* argv[], int* argument_index,
                                  size_t option_index, GrepOptions* options);
static ErrorCode ApplyGrepFlag(char option, GrepOptions* options);
static ErrorCode ExtractImplicitPattern(GrepOptions* options, char* operands[],
                                        int* operands_count);
static ErrorCode AddString(char*** items, size_t* count, const char* value);
static ErrorCode LoadPatternFile(const char* file_name, GrepOptions* options);
static void FreeStringArray(char** items, size_t count);

ErrorCode ParseGrepArguments(int argc, char* argv[], GrepOptions* options,
                             char* operands[], int* operands_count) {
  ErrorCode result = kSuccess;

  if (argc < 1 || argv == NULL || options == NULL || operands == NULL ||
      operands_count == NULL) {
    result = kErrorInvalidArgs;
  } else {
    *options = (GrepOptions){0};
    *operands_count = 0;
    result = ParseArgumentList(argc, argv, options, operands, operands_count);
    if (result == kSuccess && options->patterns_count == 0 &&
        options->pattern_files_count == 0) {
      result = ExtractImplicitPattern(options, operands, operands_count);
    }
  }

  return result;
}

static ErrorCode ParseArgumentList(int argc, char* argv[], GrepOptions* options,
                                   char* operands[], int* operands_count) {
  ErrorCode result = kSuccess;
  bool parse_options = true;

  for (int i = 1; i < argc && result == kSuccess; ++i) {
    const char* argument = argv[i];
    if (!parse_options) {
      operands[(*operands_count)++] = argv[i];
    } else if (strcmp(argument, "--") == 0) {
      parse_options = false;
    } else if (argument[0] == '-' && argument[1] != '\0') {
      result = ParseShortOptions(argc, argv, &i, options);
    } else {
      operands[(*operands_count)++] = argv[i];
    }
  }

  return result;
}

static ErrorCode ParseShortOptions(int argc, char* argv[], int* argument_index,
                                   GrepOptions* options) {
  ErrorCode result = kSuccess;
  const char* argument = argv[*argument_index];
  bool done = false;

  for (size_t i = 1; argument[i] != '\0' && result == kSuccess && !done; ++i) {
    if (argument[i] == 'e' || argument[i] == 'f') {
      result = ParseValueOption(argc, argv, argument_index, i, options);
      done = true;
    } else {
      result = ApplyGrepFlag(argument[i], options);
    }
  }

  return result;
}

static ErrorCode ParseValueOption(int argc, char* argv[], int* argument_index,
                                  size_t option_index, GrepOptions* options) {
  ErrorCode result = kSuccess;
  const char* argument = argv[*argument_index];
  const char* value = NULL;

  if (argument[option_index + 1] != '\0') {
    value = &argument[option_index + 1];
  } else if (*argument_index + 1 < argc) {
    ++(*argument_index);
    value = argv[*argument_index];
  } else {
    result = kErrorInvalidArgs;
  }
  if (result == kSuccess && argument[option_index] == 'e') {
    result = AddString(&options->patterns, &options->patterns_count, value);
  } else if (result == kSuccess) {
    result = AddString(&options->pattern_files, &options->pattern_files_count,
                       value);
  }

  return result;
}

static ErrorCode ApplyGrepFlag(char option, GrepOptions* options) {
  ErrorCode result = kSuccess;

  switch (option) {
    case 'i':
      options->ignore_case = true;
      break;
    case 'v':
      options->invert_match = true;
      break;
    case 'c':
      options->count_only = true;
      break;
    case 'l':
      options->files_with_matches = true;
      break;
    case 'n':
      options->line_number = true;
      break;
    case 'h':
      options->no_filename = true;
      break;
    case 's':
      options->suppress_errors = true;
      break;
    case 'o':
      options->only_matching = true;
      break;
    default:
      result = kErrorInvalidArgs;
      break;
  }

  return result;
}

static ErrorCode ExtractImplicitPattern(GrepOptions* options, char* operands[],
                                        int* operands_count) {
  ErrorCode result = kSuccess;

  if (*operands_count == 0) {
    result = kErrorInvalidArgs;
  } else {
    result =
        AddString(&options->patterns, &options->patterns_count, operands[0]);
    if (result == kSuccess) {
      for (int i = 1; i < *operands_count; ++i) {
        operands[i - 1] = operands[i];
      }
      --(*operands_count);
    }
  }

  return result;
}

ErrorCode LoadGrepPatternFiles(GrepOptions* options,
                               const char** error_file_name) {
  ErrorCode result = kSuccess;

  if (options == NULL || error_file_name == NULL) {
    result = kErrorInvalidArgs;
  } else {
    *error_file_name = NULL;
    for (size_t i = 0; i < options->pattern_files_count && result == kSuccess;
         ++i) {
      result = LoadPatternFile(options->pattern_files[i], options);
      if (result != kSuccess) {
        *error_file_name = options->pattern_files[i];
      }
    }
  }

  return result;
}

static ErrorCode LoadPatternFile(const char* file_name, GrepOptions* options) {
  ErrorCode result = kSuccess;
  FILE* file = fopen(file_name, "r");

  if (file == NULL) {
    result = GetFileOpenError();
  } else {
    char* line = NULL;
    size_t capacity = 0;
    ssize_t length = 0;
    while ((length = getline(&line, &capacity, file)) != -1 &&
           result == kSuccess) {
      if (length > 0 && line[length - 1] == '\n') {
        line[length - 1] = '\0';
      }
      result = AddString(&options->patterns, &options->patterns_count, line);
    }
    if (result == kSuccess && ferror(file) != 0) {
      result = kErrorFileRead;
    }
    free(line);
    fclose(file);
  }

  return result;
}

static ErrorCode AddString(char*** items, size_t* count, const char* value) {
  ErrorCode result = kSuccess;
  char** resized = realloc(*items, (*count + 1) * sizeof(**items));

  if (resized == NULL) {
    result = kErrorMemory;
  } else {
    *items = resized;
    size_t length = strlen(value) + 1;
    char* copy = malloc(length);
    if (copy == NULL) {
      result = kErrorMemory;
    } else {
      memcpy(copy, value, length);
      (*items)[*count] = copy;
      ++(*count);
    }
  }

  return result;
}

void FreeGrepOptions(GrepOptions* options) {
  if (options != NULL) {
    FreeStringArray(options->patterns, options->patterns_count);
    FreeStringArray(options->pattern_files, options->pattern_files_count);
    *options = (GrepOptions){0};
  }
}

static void FreeStringArray(char** items, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    free(items[i]);
  }
  free(items);
}
