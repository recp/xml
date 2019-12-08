/*
* Copyright (c), Recep Aslantas.
*
* MIT License (MIT), http://opensource.org/licenses/MIT
* Full license can be found in the LICENSE file
*/

#ifndef xml_impl_common_h
#define xml_impl_common_h

#include "../xml.h"
#include "impl_mem.h"

XML_INLINE
void
xml_free(xml_doc_t * __restrict doc) {
  xml_mem_t *mem, *tofree;

  mem = doc->memroot;
  while (mem) {
    tofree = mem;
    mem    = mem->next;
    free(tofree);
  }

  free(doc);
}

#endif /* xml_impl_common_h */
