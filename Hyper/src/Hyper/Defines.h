#pragma once
#include <cstdint>
#include <iostream>

typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef float       f32;
typedef double      f64;

#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __asm { int 3 }
#endif

#define ASSERT(expr)                                    \
if (!(expr)) {                                          \
ReportAssertionFailure(#expr, "", __FILE__, __LINE__);  \
debugBreak();                                           \
}

#define ASSERT_MSG(expr, msg)                           \
if (!(expr)) {                                          \
ReportAssertionFailure(#expr, msg, __FILE__, __LINE__); \
debugBreak();                                           \
}

inline void ReportAssertionFailure(const char* expression, const char* msg, const char* file, int line)
{
	std::cerr << "Assertion failed: " << expression << " -- message: \"" << msg << "\" in file " << file << " at line " << line << std::endl;
}
