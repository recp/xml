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

/* parser-local remaining resources; never retained by the document. */
typedef struct xml_parse_budget_t {
  size_t bytes;
  size_t tokens;
  size_t depth;
  bool   exhausted;
} xml_parse_budget_t;

#define XML_MEM_PAGE (65536 - sizeof(xml_mem_t)) /* 64K */

XML_INLINE
void*
xml__impl_calloc(xml_doc_t * __restrict doc, size_t size) {
  void      *data;
  xml_mem_t *mem;
  size_t     sizeToAlloc;
  
  mem         = doc->memroot;
  sizeToAlloc = (XML_MEM_PAGE < size) ? size : XML_MEM_PAGE;
  
  if (size > mem->capacity - mem->size) {
    if (sizeToAlloc > SIZE_MAX - sizeof(*mem)
        || !(mem = calloc(1, sizeof(*mem) + sizeToAlloc)))
      return NULL;

    mem->capacity = sizeToAlloc;
    mem->next     = doc->memroot;
    doc->memroot  = mem;
  }
  
  data       = (char *)mem->data + mem->size;
  mem->size += size;

  return data;
}

XML_INLINE
void*
xml__impl_calloc_budget(xml_doc_t * __restrict doc, size_t size,
                        xml_parse_budget_t *budget) {
  xml_mem_t *mem;
  size_t     bytes;

  if (!budget)
    return xml__impl_calloc(doc, size);

  if (!budget->tokens) {
    budget->exhausted = true;
    return NULL;
  }

  mem = doc->memroot;
  if (size > mem->capacity - mem->size) {
    bytes = size > XML_MEM_PAGE ? size : XML_MEM_PAGE;
    if (bytes > SIZE_MAX - sizeof(*mem))
      return NULL;
    bytes += sizeof(*mem);
    if (bytes > budget->bytes) {
      budget->exhausted = true;
      return NULL;
    }
    budget->bytes -= bytes;
  }
  budget->tokens--;
  return xml__impl_calloc(doc, size);
}

#endif /* xml_impl_mem_h */
