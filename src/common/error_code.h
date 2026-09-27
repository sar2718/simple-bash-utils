#ifndef S21_ERROR_CODE_H_
#define S21_ERROR_CODE_H_

typedef enum {
  kSuccess = 0,
  kErrorFileNotFound,
  kErrorPermission,
  kErrorInvalidArgs,
  kErrorFileRead,
  kErrorRegex,
  kErrorMemory,
  kErrorUnknown
} ErrorCode;

#endif  // S21_ERROR_CODE_H_
