/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_impl_mem_h
#define xml_impl_mem_h

#include "../common.h"
#include <stdlib.h>
#include <string.h>

typedef struct xml_mem_t {
  struct xml_mem_t *next;
  size_t            size;
  size_t            capacity;
  char              data[];
} xml_mem_t;

#define XML_MEM_PAGE (32768 - sizeof(xml_mem_t)) /* 32K */

XML_INLINE
void*
xml__impl_calloc(xml_doc_t * __restrict doc, size_t size) {
  void      *data;
  xml_mem_t *mem;
  size_t     sizeToAlloc;
  
  mem         = doc->memroot;
  sizeToAlloc = (XML_MEM_PAGE < size) ? size : XML_MEM_PAGE;
  
  if (mem->capacity < (mem->size + size)) {
    mem           = calloc(1, sizeof(*mem) + sizeToAlloc);
    mem->capacity = sizeToAlloc;
    mem->next     = doc->memroot;
    doc->memroot  = mem;
  }
  
  data       = (char *)mem->data + mem->size;
  mem->size += size;

  return data;
}

#endif /* xml_impl_mem_h */
