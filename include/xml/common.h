/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_common_h
#define xml_common_h

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#  ifdef XML_DLL
#    define XML_EXPORT __declspec(dllexport)
#  else
#    define XML_EXPORT __declspec(dllimport)
#  endif
#  define XML_INLINE __forceinline
#else
#  define XML_EXPORT __attribute__((visibility("default")))
#  define XML_INLINE static inline __attribute((always_inline))
#endif

#define XML_ARR_LEN(ARR) (sizeof(ARR)/sizeof(ARR[0]))

struct xml_t;
struct xml_attr_t;

typedef enum xml_type_t {
  XML_UNKOWN  = 0,
  XML_ELEMENT = 1,
  XML_STRING  = 2

  /*
  XML_BOOL    = 5,
  XML_NUMBER  = 4,

  XML_INTEGER = 6,
  XML_FLOAT   = 7,
  XML_NULL    = 8
   */
} xml_type_t;

typedef struct xml_attr_t {
  struct xml_attr_t *next;
  const char        *name;
  const char        *val;
  uint16_t           valsize;
  uint16_t           namesize;
  uint8_t            namequote;
  uint8_t            valquote;
} xml_attr_t;

typedef struct xml_t {
  struct xml_t      *parent;
  struct xml_t      *next;
  struct xml_attr_t *attr;
  const char        *prefix;
  const char        *tag;
  void              *val;
  uint32_t           valsize;
  uint16_t           tagsize;
  uint16_t           prefixsize;
  xml_type_t         type:16;
  bool               readonly:1;
  bool               reverse:1;
} xml_t;

typedef struct xml_doc_t {
  void       *memroot;
  xml_t      *root;
  const char *ptr;
  bool        readonly:1;
  bool        reverse:1;
  bool        sepPrefixes:1;
} xml_doc_t;

XML_INLINE
bool
xml__ascii_space(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

XML_INLINE
char *
xml__rtrim_ascii(char * __restrict begin,
                 char * __restrict end) {
  while (end > begin && xml__ascii_space(end[-1]))
    end--;

  return end;
}

XML_INLINE
bool
xml__quote_escaped(const char * __restrict begin,
                   const char * __restrict quote) {
  const char *p;
  size_t      count;

  if (quote == begin || quote[-1] != '\\')
    return false;

  p     = quote;
  count = 0;
  while (p > begin && p[-1] == '\\') {
    p--;
    count++;
  }

  return (count & 1u) != 0;
}

XML_INLINE
char *
xml__find_quote(char * __restrict p,
                char              quote) {
  char *found;

  while ((found = strchr(p, quote))) {
    if (!xml__quote_escaped(p, found))
      return found;

    p = found + 1;
  }

  return NULL;
}

XML_INLINE
const char*
xml__num_begin(const char * __restrict p,
               const char * __restrict end) {
  while (p < end && xml__ascii_space(*p))
    p++;

  return p;
}

XML_INLINE
uint64_t
xml__parse_uint64(const char * __restrict p,
                  const char * __restrict end,
                  uint64_t                 defaultValue) {
  uint64_t value;
  bool     found;

  if (!p || p >= end)
    return defaultValue;

  p = xml__num_begin(p, end);
  if (p < end && *p == '+')
    p++;

  value = 0;
  found = false;
  while (p < end && *p >= '0' && *p <= '9') {
    value = value * 10u + (uint64_t)(*p++ - '0');
    found = true;
  }

  return found ? value : defaultValue;
}

XML_INLINE
int64_t
xml__parse_int64(const char * __restrict p,
                 const char * __restrict end,
                 int64_t                  defaultValue) {
  uint64_t value, limit;
  bool     neg, found;

  if (!p || p >= end)
    return defaultValue;

  p = xml__num_begin(p, end);
  neg = false;
  if (p < end && (*p == '-' || *p == '+'))
    neg = *p++ == '-';

  value = 0;
  found = false;
  while (p < end && *p >= '0' && *p <= '9') {
    value = value * 10u + (uint64_t)(*p++ - '0');
    found = true;
  }

  if (!found)
    return defaultValue;

  limit = neg ? (uint64_t)INT64_MAX + 1u : (uint64_t)INT64_MAX;
  if (value > limit)
    return defaultValue;

  if (neg) {
    if (value == limit)
      return INT64_MIN;
    return -(int64_t)value;
  }

  return (int64_t)value;
}

XML_INLINE
double
xml__pow10i(int exp) {
  static const double pow10[] = {
    1.0e1, 1.0e2, 1.0e4, 1.0e8, 1.0e16,
    1.0e32, 1.0e64, 1.0e128, 1.0e256
  };
  double value;
  unsigned int e, i;
  bool neg;

  if (exp == 0)
    return 1.0;

  neg = exp < 0;
  e   = neg ? (unsigned int)-exp : (unsigned int)exp;

  value = 1.0;
  for (i = 0; e && i < XML_ARR_LEN(pow10); i++, e >>= 1) {
    if (e & 1u)
      value *= pow10[i];
  }

  return neg ? 1.0 / value : value;
}

XML_INLINE
double
xml__parse_double(const char * __restrict p,
                  const char * __restrict end,
                  double                  defaultValue) {
  double value, fracMul;
  int    exp;
  bool   neg, found;

  if (!p || p >= end)
    return defaultValue;

  p = xml__num_begin(p, end);
  neg = false;
  if (p < end && (*p == '-' || *p == '+'))
    neg = *p++ == '-';

  value = 0.0;
  found = false;
  while (p < end && *p >= '0' && *p <= '9') {
    value = value * 10.0 + (double)(*p++ - '0');
    found = true;
  }

  if (p < end && *p == '.') {
    p++;
    fracMul = 0.1;
    while (p < end && *p >= '0' && *p <= '9') {
      value  += (double)(*p++ - '0') * fracMul;
      fracMul *= 0.1;
      found = true;
    }
  }

  if (!found)
    return defaultValue;

  if (p < end && (*p == 'e' || *p == 'E')) {
    const char *expBegin;
    bool        expNeg, expFound;

    p++;
    expBegin = p;
    expNeg = false;
    if (p < end && (*p == '-' || *p == '+'))
      expNeg = *p++ == '-';

    exp = 0;
    expFound = false;
    while (p < end && *p >= '0' && *p <= '9') {
      if (exp < 10000)
        exp = exp * 10 + (*p - '0');
      p++;
      expFound = true;
    }

    if (expFound)
      value *= xml__pow10i(expNeg ? -exp : exp);
    else
      p = expBegin;
  }

  return neg ? -value : value;
}

XML_INLINE
uint32_t
xml__load4(const char * __restrict s) {
  uint32_t out;

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  memcpy(&out, s, sizeof(out));
  return out;
#elif defined(_WIN32) || defined(_M_IX86) || defined(_M_X64)                 \
      || defined(_M_ARM64) || defined(__i386__) || defined(__x86_64__)       \
      || defined(__aarch64__)
  memcpy(&out, s, sizeof(out));
  return out;
#else
  return (uint32_t)(uint8_t)s[0]
       | (uint32_t)(uint8_t)s[1] << 8
       | (uint32_t)(uint8_t)s[2] << 16
       | (uint32_t)(uint8_t)s[3] << 24;
#endif
}

XML_INLINE
uint64_t
xml__load8(const char * __restrict s) {
  uint64_t out;

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  memcpy(&out, s, sizeof(out));
  return out;
#elif defined(_WIN32) || defined(_M_IX86) || defined(_M_X64)                 \
      || defined(_M_ARM64) || defined(__i386__) || defined(__x86_64__)       \
      || defined(__aarch64__)
  memcpy(&out, s, sizeof(out));
  return out;
#else
  return (uint64_t)xml__load4(s)
       | (uint64_t)xml__load4(s + 4) << 32;
#endif
}

XML_INLINE
uint32_t
xml__pack4(const char * __restrict s, size_t len) {
  switch (len) {
    case 0:
      return 0;
    case 1:
      return (uint32_t)(uint8_t)s[0];
    case 2:
      return (uint32_t)(uint8_t)s[0]
           | (uint32_t)(uint8_t)s[1] << 8;
    case 3:
      return (uint32_t)(uint8_t)s[0]
           | (uint32_t)(uint8_t)s[1] << 8
           | (uint32_t)(uint8_t)s[2] << 16;
    default:
      return xml__load4(s);
  }
}

XML_INLINE
uint64_t
xml__pack8(const char * __restrict s, size_t len) {
  switch (len) {
    case 0:
      return 0;
    case 1:
      return (uint64_t)(uint8_t)s[0];
    case 2:
      return (uint64_t)xml__pack4(s, 2);
    case 3:
      return (uint64_t)xml__pack4(s, 3);
    case 4:
      return (uint64_t)xml__load4(s);
    case 5:
      return (uint64_t)xml__load4(s)
           | (uint64_t)(uint8_t)s[4] << 32;
    case 6:
      return (uint64_t)xml__load4(s)
           | (uint64_t)(uint8_t)s[4] << 32
           | (uint64_t)(uint8_t)s[5] << 40;
    case 7:
      return (uint64_t)xml__load4(s)
           | (uint64_t)(uint8_t)s[4] << 32
           | (uint64_t)(uint8_t)s[5] << 40
           | (uint64_t)(uint8_t)s[6] << 48;
    default:
      return xml__load8(s);
  }
}

XML_INLINE
bool
xml__bytes_eq(const char * __restrict a,
              const char * __restrict b,
              size_t                   len) {
  return len == 0 || (a[0] == b[0] && memcmp(a, b, len) == 0);
}

XML_INLINE
bool
xml__bytes_eq_packed4(const char * __restrict a,
                      size_t                   len,
                      uint32_t                 packed,
                      size_t                   expectedLen) {
  if (len != expectedLen || expectedLen > 4)
    return false;

  if (expectedLen > 0 && (uint8_t)a[0] != (uint8_t)packed)
    return false;

  return expectedLen == 4
         ? xml__load4(a) == packed
         : xml__pack4(a, len) == packed;
}

XML_INLINE
bool
xml__bytes_eq_packed8(const char * __restrict a,
                      size_t                   len,
                      uint64_t                 packed,
                      size_t                   expectedLen) {
  if (len != expectedLen || expectedLen < 5 || expectedLen > 8)
    return false;

  if ((uint8_t)a[0] != (uint8_t)packed)
    return false;

  return expectedLen == 8
         ? xml__load8(a) == packed
         : xml__pack8(a, len) == packed;
}

XML_INLINE
bool
xml__bytes_eq_packed(const char * __restrict a,
                     size_t                   len,
                     uint64_t                 packed,
                     size_t                   expectedLen) {
  return expectedLen <= 4
         ? xml__bytes_eq_packed4(a, len, (uint32_t)packed, expectedLen)
         : xml__bytes_eq_packed8(a, len, packed, expectedLen);
}

#endif /* xml_common_h */
