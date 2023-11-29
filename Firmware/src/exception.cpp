#include "exception.h"

// SvKo: added
RTC_DATA_ATTR char stackTrace[STACK_LEN];

Exception::Exception(const char* msg) {
	_msg = msg;
}

const char* Exception::Message() {
    return _msg;
}

void ThrowExceptionFunc() {
	throw Exception("Something bad happened");
}

void ValidateExceptionHandling() {
  try
  {
    ThrowExceptionFunc();
    Serial.print("Exception was not thrown:");
  }
  catch(Exception& ex)
  {
    Serial.print("Exception happened:");
    Serial.println(ex.Message());
  }
}

void StoreStackTrace(const char* msg) {
	if (strlen(msg) < STACK_LEN)
		strcpy(stackTrace, msg);
	else
		strncpy(stackTrace, msg, STACK_LEN-1);
}

const char* GetStackTrace() {
	return stackTrace;
}
	
void CleanStackTrace() {
	strcpy(stackTrace, "");
}