#ifndef S21_ERROR_UTILS_H_
#define S21_ERROR_UTILS_H_

#include "error_code.h"

ErrorCode GetFileOpenError(void);
void PrintProgramError(const char* program_name, ErrorCode error,
                       const char* file_name);

#endif  // S21_ERROR_UTILS_H_
