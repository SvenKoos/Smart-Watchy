#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#include "Arduino.h"

#define STACK_LEN 1024

class Exception {
  private:
  const char* _msg;

  public:
  Exception(const char* msg);
  const char* Message();
};

void ThrowExceptionFunc();
void ValidateExceptionHandling();
void StoreStackTrace(const char* msg);
const char* GetStackTrace();
void CleanStackTrace();

#endif
