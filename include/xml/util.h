/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_util_h
#define xml_util_h

#include "common.h"

/*!
 * @brief return xml object from value
 */
XML_INLINE
xml_type_t
xml_valtype(const xml_t * __restrict object) {
  if (object->val == NULL)
    return 0;
  
  return ((xml_t *)object->val)->type;
}

/*!
 * @brief return xml object from value
 */
XML_INLINE
xml_t*
xml_xml(const xml_t * __restrict object) {
  if (!object || object->type != XML_ELEMENT)
    return NULL;

  return (xml_t *)object->val;
}

/*!
 * @brief return string node of next node
 *        first you should get string node by xmls() then use this to get next
 *        string node until NULL. xmls() returns first chils node this returns
 *        first next node.
 *
 * if XML_READONLY is used then the string returned from obj->val is not NULL
 * terminated, so you must use obj->valsize to copy, compare ... string
 * otherwise if XML_READONLY is used then the obj->val is NULL terminated string
 *
 * @param[in] obj xml node to compare if it is string node
 * @return string node
 */
XML_INLINE
const xml_t*
xmls_next(const xml_t * __restrict obj) {
  const xml_t *o;
  
  o = obj;

  while (o) {
    o = o->next;

    if (!o || o->type == XML_STRING) {
      if (o == obj)
        return NULL;
      return o;
    }
  }

  return NULL;
}

/*!
 * @brief return first string node
 *
 * if XML_READONLY is used then the string returned from obj->val is not NULL
 * terminated, so you must use obj->valsize to copy, compare ... string
 * otherwise if XML_READONLY is used then the obj->val is NULL terminated string
 *
 * @param[in] obj xml node to get its value
 * @return non-NULL terminated string value (pointer only)
 */
XML_INLINE
const xml_t*
xmls(const xml_t * __restrict obj) {
  if (!obj || obj->type != XML_ELEMENT || !(obj = obj->val))
    return NULL;

  if (obj->type == XML_STRING)
    return obj;

  return xmls_next(obj);
}

/*!
* @brief return sum of all length of string nodes
*
* @param[in] obj xml node to get its value
* @return sum of all length of string nodes + 1 (null terminator)
*/
XML_INLINE
size_t
xmls_sumlen(const xml_t * __restrict obj) {
  const xml_t *v;
  size_t       len;
  
  len = 0;
  if ((v = xmls(obj))) {
    len++; /* NULL terminator */
    do {
      len += v->valsize;
    } while ((v = xmls_next(v)));
  }

  return len;
}

/*!
 * @brief creates number (int32) from string value
 *
 * @param[in] obj          xml element
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
int32_t
xml_i32(const xml_t * __restrict obj, int32_t defaultValue) {
  const xml_t *v;

  if (!(v = xmls(obj)))
    return defaultValue;

  return (int32_t)strtol(v->val, NULL, 10);
}

/*!
 * @brief creates number (uint32) from string value
 *
 * @param[in] obj          xml element
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
uint32_t
xml_u32(const xml_t * __restrict obj, uint32_t defaultValue) {
  const xml_t *v;

  if (!(v = xmls(obj)))
    return defaultValue;

  return (uint32_t)strtoul(v->val, NULL, 10);
}

/*!
 * @brief creates number (int64) from string value
 *
 * @param[in] obj          xml element
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
int64_t
xml_i64(const xml_t * __restrict obj, int64_t defaultValue) {
  const xml_t *v;

  if (!(v = xmls(obj)))
    return defaultValue;

  return strtoll(v->val, NULL, 10);
}

/*!
 * @brief creates number (uint64) from string value
 *
 * @param[in] obj          xml element
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
uint64_t
xml_u64(const xml_t * __restrict obj, uint64_t defaultValue) {
  const xml_t *v;

  if (!(v = xmls(obj)))
    return defaultValue;

  return strtoull(v->val, NULL, 10);
}

/*!
 * @brief creates number (float) from string value
 *
 * @param[in] obj          xml element
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
float
xml_float(const xml_t * __restrict obj, float defaultValue) {
  const xml_t *v;

  if (!(v = xmls(obj)))
    return defaultValue;

  return strtof(v->val, NULL);
}

/*!
 * @brief creates number (double) from string value
 *
 * @param[in] obj          xml element
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
double
xml_double(const xml_t * __restrict obj, double defaultValue) {
  const xml_t *v;

  if (!(v = xmls(obj)))
    return defaultValue;

  return strtod(v->val, NULL);
}

/*!
 * @brief creates boolean from string value
 *
 * @param[in] obj          xml element
 * @param[in] defaultValue default value if operation fails
 * @return boolean values as integer: 1 true, 0 false, error: defaultValue
 */
XML_INLINE
int
xml_bool(const xml_t * __restrict obj, int defaultValue) {
  const xml_t *v;
  char         first;

  if (!(v = xmls(obj)))
    return defaultValue;

  first = *(char *)v->val;

  return first == 't' || first == '1';
}

/*!
 * @brief compares xml tag with a string
 *
 * @param[in] obj xml element
 * @param[in] str string to compare (must be NULL terminated)
 * @return true if str is equals to tag
 */
XML_INLINE
bool
xml_tag_eq(const xml_t * __restrict obj, const char * __restrict str) {
  size_t strsize;
    
  if ((strsize = strlen(str)) != (size_t)obj->tagsize)
    return false;

  return strncmp(str, obj->tag, strsize) == 0;
}

/*!
 * @brief compares xml tag with a string
 *
 * @param[in] obj     xml element
 * @param[in] str     string to compare
 * @param[in] strsize size of string to compare
 * @return true if str is equals to tag
 */
XML_INLINE
bool
xml_tag_eqsz(const xml_t * __restrict obj,
             const char  * __restrict str,
             size_t                   strsize) {
  if (strsize != (size_t)obj->tagsize)
    return false;

  return strncmp(str, obj->tag, strsize) == 0;
}

/*!
* @brief compares xml tag with a string and like strncmp.
*
* @param[in] obj     xml element
* @param[in] str     string to compare
* @return a number like strncmp returns
*/
XML_INLINE
int
xml_tag_cmp(const xml_t * __restrict obj, const char * __restrict str) {
  size_t strsize;
  int    c;
    
  strsize = strlen(str);

  if (obj->tagsize > strsize) {
    if ((c = strncmp(obj->tag, str, strsize) != 0))
      return c;
    return 1;
  } else if (obj->tagsize < strsize) {
    if ((c = strncmp(obj->tag, str, obj->tagsize) != 0))
      return c;
    return -1;
  }

  return strncmp(obj->tag, str, strsize);
}

/*!
 * @brief compares xml value with a string
 *
 * @param[in] obj xml element
 * @param[in] str string to compare (must be NULL terminated)
 * @return true if str is equals to value
 */
XML_INLINE
bool
xml_val_eq(const xml_t * __restrict obj, const char * __restrict str) {
  size_t strsize;
    
  if ((strsize = strlen(str)) != (size_t)obj->valsize)
    return false;

  return strncmp(str, obj->val, strsize) == 0;
}

/*!
 * @brief compares xml value with a string
 *
 * @param[in] obj     xml element
 * @param[in] str     string to compare
 * @param[in] strsize size of string to compare
 * @return true if str is equals to value
 */
XML_INLINE
bool
xml_val_eqsz(const xml_t * __restrict obj,
             const char   * __restrict str,
             size_t                    strsize) {
  if (strsize != (size_t)obj->valsize)
    return false;

  return strncmp(str, obj->val, strsize) == 0;
}

#endif /* xml_util_h */
