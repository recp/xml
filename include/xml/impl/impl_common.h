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

XML_INLINE
xml_attr_t*
xmla(const xml_t * __restrict object, const char * __restrict name) {
  xml_attr_t *iter;
  size_t      namesize;

  if (!object || !name || !(iter = object->attr))
    return NULL;

  namesize = strlen(name);
  while (iter && strncmp(iter->name, name, namesize) != 0)
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_t*
xml_elem(const xml_t * __restrict object, const char * __restrict name) {
  xml_t *iter;
  size_t namesize;
  
  if (!object || !name || object->type != XML_ELEMENT || !(iter = object->val))
    return NULL;
  
  namesize = strlen(name);
  while (iter && strncmp(iter->tag, name, namesize) != 0)
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_t*
xml_elem_next(const xml_t * __restrict current, const char * __restrict name) {
  xml_t *iter;
  size_t namesize;
  
  if (!current || !name || (iter = current->next))
    return NULL;
  
  namesize = strlen(name);
  while (iter && strncmp(iter->tag, name, namesize) != 0)
    iter = iter->next;

  return iter;
}

#endif /* xml_impl_common_h */
