/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_impl_objmap_h
#define xml_impl_objmap_h

#include "../xml.h"
#include "impl_mem.h"
#include "../util.h"

XML_INLINE
void
xml_objmap(xml_t        * __restrict obj,
           xml_objmap_t * __restrict objmap,
           size_t                    count) {
  xml_objmap_t *item;
  size_t        start, end, i;

  if (!obj || obj->type != XML_ELEMENT || !(obj = obj->val))
    return;

  start = 0;
  end   = count;

  while (obj) {
    for (i = start; i < end; i++) {
      item = &objmap[i];

      if (xml_tag_eq(obj, item->key)) {
        item->object = obj;
        if (i == start)
          start++;
        else if (i == end - 1)
          end--;

        /* duplicated keys are not allowed for now */
        break;
      }
    }
    obj = obj->next;
  }
}

XML_INLINE
void
xml_objmap_call(xml_t        * __restrict obj,
                xml_objmap_t * __restrict objmap,
                size_t                    count,
                bool         * __restrict stop) {
  xml_objmap_t *item;
  size_t        i;

  if (!obj || obj->type != XML_ELEMENT)
    return;

  xml_objmap(obj, objmap, count);

  for (i = 0; i < count; i++) {
    if (stop && *stop)
      break;

    item = &objmap[i];
    if (item->object) {
      if (item->foundFunc.func)
        item->foundFunc.func(item->object, item->foundFunc.param);
    } else if (item->notFoundFunc.func) {
      item->notFoundFunc.func(item->object, item->notFoundFunc.param);
    }
  }
}

#endif /* xml_impl_objmap_h */
