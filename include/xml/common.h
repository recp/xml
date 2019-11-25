/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_common_h
#define xml_common_h

#define _USE_MATH_DEFINES /* for windows */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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

typedef enum xml_type_t {
  XML_UNKOWN  = 0,
  XML_OBJECT  = 1,
  XML_ARRAY   = 2,
  XML_STRING  = 3,

  /*
  XML_BOOL    = 5,
  XML_NUMBER  = 4,

  XML_INTEGER = 6,
  XML_FLOAT   = 7,
  XML_NULL    = 8
   */
} xml_type_t;

typedef struct xml_t {
  struct xml_t *parent;
  struct xml_t *next;
  const char   *key;
  void         *value;
  int           valSize;
  int           keySize;
  xml_type_t    type;
} xml_t;

typedef struct xml_array_t {
  xml_t base;
  int   count;
} xml_array_t;

typedef struct xml_doc_t {
  void       *memroot;
  xml_t      *root;
  const char *ptr;
} xml_doc_t;

#endif /* xml_common_h */
