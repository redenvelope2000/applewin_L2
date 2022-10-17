#pragma once

#include "wincompat.h"
#include "misc.h"

#include <cstddef>
#include <ctype.h>
#include <cstdarg>

#define _TRUNCATE ((size_t)-1)

#define _strdup strdup
#define _strtoui64 strtoull
#define _tcsrchr strrchr
#define _tcsncpy strncpy
#define _tcslen     strlen
#define _tcscmp strcmp
#define _tcsicmp _stricmp
#define _stricmp strcasecmp
#define _tcsstr strstr
#define _tcscpy     strcpy
#define _tcstol strtol
#define _tcstoul strtoul
#ifndef WIN32
  #define sscanf_s sscanf
#endif
#define _tcsncmp strncmp
#define _tcsncat strncat

inline bool IsCharLower(char ch) {
        return isascii(ch) && islower(ch);
}

DWORD CharLowerBuff(LPTSTR lpsz, DWORD cchLength);

#ifndef WIN32
void strcpy_s(char * dest, size_t size, const char * source);

LPSTR _strupr( LPSTR str );

errno_t strncpy_s(char * dest, size_t numberOfElements, const char *src, size_t count);

template <size_t size>
errno_t strncpy_s(char (&dest)[size], const char *src, size_t count)
{
  return strncpy_s(dest, size, src, count);
}
#endif
