#include "cat_options.h"

#include <string.h>

static ErrorCode ParseLongOption(const char* argument, CatOptions* options);
static ErrorCode ParseShortOptions(const char* argument, CatOptions* options);
static ErrorCode ApplyShortOption(char option, CatOptions* options);

ErrorCode ParseCatArguments(int argc, char* argv[], CatOptions* options,
                            char* operands[], int* operands_count) {
  ErrorCode result = kSuccess;
  bool parse_options = true;

  if (argc < 1 || argv == NULL || options == NULL || operands == NULL ||
      operands_count == NULL) {
    result = kErrorInvalidArgs;
  } else {
    *options = (CatOptions){0};
    *operands_count = 0;
    for (int i = 1; i < argc && result == kSuccess; ++i) {
      const char* argument = argv[i];
      if (!parse_options) {
        operands[(*operands_count)++] = argv[i];
      } else if (strcmp(argument, "--") == 0) {
        parse_options = false;
      } else if (argument[0] == '-' && argument[1] == '-' &&
                 argument[2] != '\0') {
        result = ParseLongOption(argument, options);
      } else if (argument[0] == '-' && argument[1] != '\0') {
        result = ParseShortOptions(argument, options);
      } else {
        operands[(*operands_count)++] = argv[i];
      }
    }
  }

  return result;
}

static ErrorCode ParseLongOption(const char* argument, CatOptions* options) {
  ErrorCode result = kSuccess;

  if (strcmp(argument, "--number-nonblank") == 0) {
    options->number_nonblank_lines = true;
  } else if (strcmp(argument, "--number") == 0) {
    options->number_all_lines = true;
  } else if (strcmp(argument, "--squeeze-blank") == 0) {
    options->squeeze_blank = true;
  } else {
    result = kErrorInvalidArgs;
  }

  return result;
}

static ErrorCode ParseShortOptions(const char* argument, CatOptions* options) {
  ErrorCode result = kSuccess;

  for (size_t i = 1; argument[i] != '\0' && result == kSuccess; ++i) {
    result = ApplyShortOption(argument[i], options);
  }

  return result;
}

static ErrorCode ApplyShortOption(char option, CatOptions* options) {
  ErrorCode result = kSuccess;

  switch (option) {
    case 'b':
      options->number_nonblank_lines = true;
      break;
    case 'e':
      options->show_ends = true;
      options->show_nonprinting = true;
      break;
    case 'E':
      options->show_ends = true;
      break;
    case 'n':
      options->number_all_lines = true;
      break;
    case 's':
      options->squeeze_blank = true;
      break;
    case 't':
      options->show_tabs = true;
      options->show_nonprinting = true;
      break;
    case 'T':
      options->show_tabs = true;
      break;
    default:
      result = kErrorInvalidArgs;
      break;
  }

  return result;
}
