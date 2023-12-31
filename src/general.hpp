#ifndef GENERAL_HPP
#define GENERAL_HPP

#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#if !defined(__clang__)
#error Compilation only supported with clang
#endif

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#error Big endian compilation not supported
#endif

#if defined(NDEBUG)
#define DEBUG 0
#else
#define DEBUG 1
#endif

#define PICK2_0(a0, a1) a0
#define PICK2_1(a0, a1) a1

#define DECLARE_KIND(picklist, name)                                                                                   \
  namespace name {                                                                                                     \
  enum Enum : i16 { picklist(PICK2_0) NumItems };                                                                      \
  const cstr to_string[] __attribute__((unused)) = {picklist(PICK2_1)};                                                \
  }

static_assert(sizeof(intptr_t) == 8, "Compilation only supported on 64-bit machine");

using u8    = uint8_t;
using u16   = uint16_t;
using u32   = uint32_t;
using u64   = uint64_t;
using usize = size_t;

using i8   = int8_t;
using i16  = int16_t;
using i32  = int32_t;
using i64  = int64_t;
using size = ssize_t;

using f32 = float;
using f64 = double;

using cstr = const char *;

enum Result : i32 { ok = 0, err = 1 };

struct Span {
  i32 index;
  i32 length;
};

inline void debug(cstr msg, ...) {
#if DEBUG
  va_list args;
  va_start(args, msg);
  vfprintf(stdout, msg, args);
  va_end(args);
#else
  (void)msg;
#endif
}

inline void error(cstr msg, ...) {
  fprintf(stderr, "err: ");
  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
}

[[noreturn]] inline void panic(cstr msg, ...) {
  fflush(stdout);
  fprintf(stderr, "PANIC: ");
  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  abort();
}

#endif
