#ifndef K_GENERAL_
#define K_GENERAL_
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define internal        static
#define global_variable static
#define local_persist   static

typedef uint32_t  bool32;
typedef int8_t    int8;
typedef int16_t   int16;
typedef int32_t   int32;
typedef int64_t   int64;

typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;
typedef uint64_t  uint64;

typedef float     real32;
typedef double    real64;

typedef size_t    size;
typedef intptr_t  iptr;
typedef uintptr_t uptr;

#define BYTES(n) ((size)n)
#define KILOBYTES(n) (KILOBYTES(n)*1024)
#define MEGABYTES(n) (MEGABYTES(n)*1024)
#define GIGABYTES(n) (MEGABYTES(n)*1024)

#if  _MSC_VER
#define STR_RED(string) string
#define STR_GREEN(string) string
#define STR_YELLOW(string) string
#else 
#define STR_RED(string) "\033[1;31m" string "\033[0m"
#define STR_GREEN(string) "\033[1;32m" string "\033[0m"
#define STR_YELLOW(string) "\033[1;33m" string "\033[0m"
#endif

#define SWAPVAR(ty, a, b) do { ty temp = a; a = b; b = temp; } while(0)

#ifdef _MSC_VER
#define BREAK __debugbreak();
#else
#include <signal.h>
#ifdef SIGTRAP
#define BREAK raise(SIGTRAP);
#else
#define BREAK abort();
#endif
#endif

// SIMD ALIGNMENT DECLARATION
#ifdef _MSC_VER
#define ALIGN16 __declspec(align(16))
#else
#define ALIGN16 __attribute__((aligned(16)))
#endif


#define ASSERT(expr) { \
    if (!(expr)) { \
        printf(STR_RED("[ASSERT FAILED(line %d)]: expr `" #expr "` (%s, %s)") "\n", __LINE__, __FILE__, __func__); \
        BREAK\
    } \
}
#define TRACE(message, ...) { printf(STR_YELLOW("[TRACE(line %d)]: " message  " (%s, %s)") "\n", __LINE__, ##__VA_ARGS__, __FILE__, __func__); }

#define NOT_IMPLEMENTED { int Assert_Not_Implemented = 0; ASSERT(Assert_Not_Implemented); }
#define INVALID_PATH    { int Assert_This_Path_Is_Invalid = 0; ASSERT(Assert_This_Path_Is_Invalid); }
#endif // K_GENERAL_
