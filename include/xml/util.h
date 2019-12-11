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
  return (xml_t *)object->val;
}

/*!
 * @brief return non-NULL terminated string value
 *
 * you must use object->valsize to copy, compare ... string
 *
 * @param[in] object xml object
 * @return non-NULL terminated string value (pointer only)
 */
XML_INLINE
const char*
xml_string(const xml_t * __restrict object) {
  return (const char *)object->val;
}

/*!
 * @brief creates number (int32) from string value
 *
 * @param[in] object       xml element
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
int32_t
xml_int32(const xml_t * __restrict object, int32_t defaultValue) {
  if (!object || object->type != XML_STRING || !object->val)
    return defaultValue;

  return (int32_t)strtol(object->val, NULL, 10);
}

/*!
 * @brief creates number (uint32) from string value
 *
 * @param[in] object       xml element
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
uint32_t
xml_uint32(const xml_t * __restrict object, uint32_t defaultValue) {
  if (!object || object->type != XML_STRING || !object->val)
    return defaultValue;

  return (uint32_t)strtoul(object->val, NULL, 10);
}

/*!
 * @brief creates number (int64) from string value
 *
 * @param[in] object       xml element
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
int64_t
xml_int64(const xml_t * __restrict object, int64_t defaultValue) {
  if (!object || object->type != XML_STRING || !object->val)
    return defaultValue;

  return strtoll(object->val, NULL, 10);
}

/*!
 * @brief creates number (uint64) from string value
 *
 * @param[in] object       xml element
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
uint64_t
xml_uint64(const xml_t * __restrict object, uint64_t defaultValue) {
  if (!object || object->type != XML_STRING || !object->val)
    return defaultValue;

  return strtoull(object->val, NULL, 10);
}

/*!
 * @brief creates number (float) from string value
 *
 * @param[in] object       xml element
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
float
xml_float(const xml_t * __restrict object, float defaultValue) {
  if (!object || object->type != XML_STRING || !object->val)
    return defaultValue;

  return strtof(object->val, NULL);
}

/*!
 * @brief creates number (double) from string value
 *
 * @param[in] object       xml element
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
double
xml_double(const xml_t * __restrict object, double defaultValue) {
  if (!object || object->type != XML_STRING || !object->val)
    return defaultValue;

  return strtod(object->val, NULL);
}

/*!
 * @brief creates boolean from string value
 *
 * @param[in] object       xml element
 * @param[in] defaultValue default value if operation fails
 * @return boolean values as integer: 1 true, 0 false, error: defaultValue
 */
XML_INLINE
int
xml_bool(const xml_t * __restrict object, int defaultValue) {
  const char *boolString;
  char        first;

  if (!object || object->type != XML_STRING || !(boolString = object->val))
    return defaultValue;

  first = boolString[0];

  if (first == 't' || first == '1') {
    return 1;
  } else if (first == 'n' || first == '0') {
    return 0;
  }

  return defaultValue;
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
