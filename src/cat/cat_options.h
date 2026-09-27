#ifndef S21_CAT_OPTIONS_H_
#define S21_CAT_OPTIONS_H_

#include <stdbool.h>

#include "../common/error_code.h"

typedef struct {
  bool number_all_lines;
  bool number_nonblank_lines;
  bool squeeze_blank;
  bool show_ends;
  bool show_tabs;
  bool show_nonprinting;
} CatOptions;

ErrorCode ParseCatArguments(int argc, char* argv[], CatOptions* options,
                            char* operands[], int* operands_count);

#endif  // S21_CAT_OPTIONS_H_
