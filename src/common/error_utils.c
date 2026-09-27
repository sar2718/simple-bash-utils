#include "error_utils.h"

#include <errno.h>
#include <stdio.h>

static const char* ErrorText(ErrorCode error);

ErrorCode GetFileOpenError(void) {
  ErrorCode result = kErrorUnknown;

  if (errno == ENOENT) {
    result = kErrorFileNotFound;
  } else if (errno == EACCES) {
    result = kErrorPermission;
  }

  return result;
}

void PrintProgramError(const char* program_name, ErrorCode error,
                       const char* file_name) {
  const char* text = ErrorText(error);

  if (error != kSuccess) {
    if (file_name != NULL) {
      fprintf(stderr, "%s: %s: %s\n", program_name, file_name, text);
    } else {
      fprintf(stderr, "%s: %s\n", program_name, text);
    }
  }
}

static const char* ErrorText(ErrorCode error) {
  const char* text = "unknown error";

  switch (error) {
    case kSuccess:
      text = "success";
      break;
    case kErrorFileNotFound:
      text = "No such file or directory";
      break;
    case kErrorPermission:
      text = "Permission denied";
      break;
    case kErrorInvalidArgs:
      text = "invalid arguments";
      break;
    case kErrorFileRead:
      text = "Input/output error";
      break;
    case kErrorRegex:
      text = "invalid regular expression";
      break;
    case kErrorMemory:
      text = "memory allocation error";
      break;
    case kErrorUnknown:
      break;
  }

  return text;
}
