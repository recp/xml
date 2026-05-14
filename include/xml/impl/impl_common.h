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
  if (!name)
    return NULL;

  return xmla_sz(object, name, strlen(name));
}

XML_INLINE
xml_attr_t*
xmla_sz(const xml_t * __restrict object,
        const char  * __restrict name,
        size_t                   namesize) {
  xml_attr_t *iter;

  if (!object || !name || !(iter = object->attr))
    return NULL;

  while (iter
         && (!iter->name
             || (size_t)iter->namesize != namesize
             || iter->name[0] != name[0]
             || !xml__bytes_eq(iter->name, name, namesize)))
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_attr_t*
xmla_packed4(const xml_t * __restrict object,
             uint32_t                 packed,
             size_t                   namesize) {
  xml_attr_t *iter;

  if (!object || !(iter = object->attr))
    return NULL;

  while (iter
         && (!iter->name
             || !xml__bytes_eq_packed4(iter->name,
                                       (size_t)iter->namesize,
                                       packed,
                                       namesize)))
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_attr_t*
xmla_packed8(const xml_t * __restrict object,
             uint64_t                 packed,
             size_t                   namesize) {
  xml_attr_t *iter;

  if (!object || !(iter = object->attr))
    return NULL;

  while (iter
         && (!iter->name
             || !xml__bytes_eq_packed8(iter->name,
                                       (size_t)iter->namesize,
                                       packed,
                                       namesize)))
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_attr_t*
xmla_packed(const xml_t * __restrict object,
            uint64_t                 packed,
            size_t                   namesize) {
  xml_attr_t *iter;

  if (!object || !(iter = object->attr))
    return NULL;

  while (iter
         && (!iter->name
             || !xml__bytes_eq_packed(iter->name,
                                      (size_t)iter->namesize,
                                      packed,
                                      namesize)))
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_t*
xml_elem(const xml_t * __restrict object, const char * __restrict name) {
  if (!name)
    return NULL;

  return xml_elem_sz(object, name, strlen(name));
}

XML_INLINE
xml_t*
xml_elem_sz(const xml_t * __restrict object,
            const char  * __restrict name,
            size_t                   namesize) {
  xml_t *iter;
  
  if (!object || !name || object->type != XML_ELEMENT || !(iter = object->val))
    return NULL;
  
  while (iter
         && (iter->type != XML_ELEMENT
             || !iter->tag
             || (size_t)iter->tagsize != namesize
             || iter->tag[0] != name[0]
             || !xml__bytes_eq(iter->tag, name, namesize)))
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_t*
xml_elem_packed4(const xml_t * __restrict object,
                 uint32_t                 packed,
                 size_t                   namesize) {
  xml_t *iter;

  if (!object || object->type != XML_ELEMENT || !(iter = object->val))
    return NULL;

  while (iter
         && (iter->type != XML_ELEMENT
             || !iter->tag
             || !xml__bytes_eq_packed4(iter->tag,
                                       (size_t)iter->tagsize,
                                       packed,
                                       namesize)))
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_t*
xml_elem_packed8(const xml_t * __restrict object,
                 uint64_t                 packed,
                 size_t                   namesize) {
  xml_t *iter;

  if (!object || object->type != XML_ELEMENT || !(iter = object->val))
    return NULL;

  while (iter
         && (iter->type != XML_ELEMENT
             || !iter->tag
             || !xml__bytes_eq_packed8(iter->tag,
                                       (size_t)iter->tagsize,
                                       packed,
                                       namesize)))
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_t*
xml_elem_packed(const xml_t * __restrict object,
                uint64_t                 packed,
                size_t                   namesize) {
  xml_t *iter;

  if (!object || object->type != XML_ELEMENT || !(iter = object->val))
    return NULL;

  while (iter
         && (iter->type != XML_ELEMENT
             || !iter->tag
             || !xml__bytes_eq_packed(iter->tag,
                                      (size_t)iter->tagsize,
                                      packed,
                                      namesize)))
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_t*
xml_elem_next(const xml_t * __restrict current, const char * __restrict name) {
  if (!name)
    return NULL;

  return xml_elem_next_sz(current, name, strlen(name));
}

XML_INLINE
xml_t*
xml_elem_next_sz(const xml_t * __restrict current,
                 const char  * __restrict name,
                 size_t                   namesize) {
  xml_t *iter;

  if (!current || !name || !(iter = current->next))
    return NULL;

  while (iter
         && (iter->type != XML_ELEMENT
             || !iter->tag
             || (size_t)iter->tagsize != namesize
             || iter->tag[0] != name[0]
             || !xml__bytes_eq(iter->tag, name, namesize)))
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_t*
xml_elem_next_packed4(const xml_t * __restrict current,
                      uint32_t                 packed,
                      size_t                   namesize) {
  xml_t *iter;

  if (!current || !(iter = current->next))
    return NULL;

  while (iter
         && (iter->type != XML_ELEMENT
             || !iter->tag
             || !xml__bytes_eq_packed4(iter->tag,
                                       (size_t)iter->tagsize,
                                       packed,
                                       namesize)))
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_t*
xml_elem_next_packed8(const xml_t * __restrict current,
                      uint64_t                 packed,
                      size_t                   namesize) {
  xml_t *iter;

  if (!current || !(iter = current->next))
    return NULL;

  while (iter
         && (iter->type != XML_ELEMENT
             || !iter->tag
             || !xml__bytes_eq_packed8(iter->tag,
                                       (size_t)iter->tagsize,
                                       packed,
                                       namesize)))
    iter = iter->next;

  return iter;
}

XML_INLINE
xml_t*
xml_elem_next_packed(const xml_t * __restrict current,
                     uint64_t                 packed,
                     size_t                   namesize) {
  xml_t *iter;

  if (!current || !(iter = current->next))
    return NULL;

  while (iter
         && (iter->type != XML_ELEMENT
             || !iter->tag
             || !xml__bytes_eq_packed(iter->tag,
                                      (size_t)iter->tagsize,
                                      packed,
                                      namesize)))
    iter = iter->next;

  return iter;
}

#endif /* xml_impl_common_h */
