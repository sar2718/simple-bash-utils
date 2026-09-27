#ifndef S21_GREP_OPTIONS_H_
#define S21_GREP_OPTIONS_H_

#include <stdbool.h>
#include <stddef.h>

#include "../common/error_code.h"

typedef struct {
  bool ignore_case;
  bool invert_match;
  bool count_only;
  bool files_with_matches;
  bool line_number;
  bool no_filename;
  bool suppress_errors;
  bool only_matching;
  char** patterns;
  size_t patterns_count;
  char** pattern_files;
  size_t pattern_files_count;
} GrepOptions;

ErrorCode ParseGrepArguments(int argc, char* argv[], GrepOptions* options,
                             char* operands[], int* operands_count);
ErrorCode LoadGrepPatternFiles(GrepOptions* options,
                               const char** error_file_name);
void FreeGrepOptions(GrepOptions* options);

#endif  // S21_GREP_OPTIONS_H_
