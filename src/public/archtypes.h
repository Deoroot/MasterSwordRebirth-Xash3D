//
// Word size dependent definitions
// DAL 1/03
//
#ifndef ARCHTYPES_H
#define ARCHTYPES_H

#ifndef XASH_BUILD
#include "steam/steamtypes.h"
#else
// Provide basic type definitions without Steam headers
typedef unsigned char uint8;
typedef signed char int8;
#ifdef _WIN32
typedef __int16 int16;
typedef unsigned __int16 uint16;
typedef __int32 int32;
typedef unsigned __int32 uint32;
typedef __int64 int64;
typedef unsigned __int64 uint64;
#else
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;
#endif
#endif

#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <stddef.h>
#define _S_IREAD S_IREAD
#define _S_IWRITE S_IWRITE
typedef long unsigned int ulong;
#define MAX_PATH PATH_MAX
#endif

#endif // ARCHTYPES_H
