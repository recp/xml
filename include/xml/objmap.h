/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_objmap_h
#define xml_objmap_h

#include "common.h"

/*
 helpers to create objmap arrays, for example:

  xml_objmap_t objmap[] = {
    XML_OBJMAP_FN("key 1",  func1, param1),
    XML_OBJMAP_FN("key 2",  func2, param2),
    XML_OBJMAP_FN("key 3",  func3, param3),
    XML_OBJMAP_FN("key 4",  func4, param4),
    XML_OBJMAP_FN("key 5",  func5, param5)
  };

 this may be more readable than manually put values to objmap struct.
 */

#define XML_OBJMAP_FN(KEY, FUN, PARAM)                                        \
  {                                                                           \
    .object       = NULL,                                                     \
    .key          = KEY,                                                      \
    .foundFunc    = {FUN, PARAM},                                             \
    .notFoundFunc = {NULL, NULL},                                             \
    .userdata     = NULL                                                      \
  }

#define XML_OBJMAP_OBJ(KEY, USERDATA)                                         \
  {                                                                           \
    .object       = NULL,                                                     \
    .key          = KEY,                                                      \
    .foundFunc    = {NULL, NULL},                                             \
    .notFoundFunc = {NULL, NULL},                                             \
    .userdata     = USERDATA                                                  \
  }

typedef void (*xml_objmap_fun_t)(xml_t * __restrict object,
                                 void  * __restrict userdata);

typedef struct xml_func_t {
  xml_objmap_fun_t func;
  void            *param;
} xml_func_t;

typedef struct xml_objmap_t {
  xml_t       *object; /* found xml object */
  const char  *key;
  void        *userdata;
  xml_func_t   foundFunc;
  xml_func_t   notFoundFunc;
} xml_objmap_t;

XML_INLINE
void
xml_objmap(xml_t        * __restrict obj,
           xml_objmap_t * __restrict objmap,
           size_t                    count);

XML_INLINE
void
xml_objmap_call(xml_t        * __restrict obj,
                xml_objmap_t * __restrict objmap,
                size_t                    count,
                bool         * __restrict stop);

#include "impl/impl_objmap.h"

#endif /* xml_objmap_h */
