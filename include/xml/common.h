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

#endif /* xml_common_h */
