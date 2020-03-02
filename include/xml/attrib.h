/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_attr_h
#define xml_attr_h

#include "common.h"

/*!
 * @brief creates number (int32) from string value
 *
 * @param[in] attr         xml attribute
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
int32_t
xmla_i32(const xml_attr_t * __restrict attr, int32_t defaultValue) {
  if (!attr || !attr->val)
    return defaultValue;

  return (int32_t)strtol(attr->val, NULL, 10);
}

/*!
 * @brief creates number (uint32) from string value
 *
 * @param[in] attr         xml attribute
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
uint32_t
xmla_u32(const xml_attr_t * __restrict attr, uint32_t defaultValue) {
  if (!attr || !attr->val)
    return defaultValue;

  return (uint32_t)strtoul(attr->val, NULL, 10);
}

/*!
 * @brief creates number (int64) from string value
 *
 * @param[in] attr         xml attribute
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
int64_t
xmla_i64(const xml_attr_t * __restrict attr, int64_t defaultValue) {
  if (!attr || !attr->val)
    return defaultValue;

  return strtoll(attr->val, NULL, 10);
}

/*!
 * @brief creates number (uint64) from string value
 *
 * @param[in] attr         xml attribute
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
uint64_t
xmla_u64(const xml_attr_t * __restrict attr, uint64_t defaultValue) {
  if (!attr || !attr->val)
    return defaultValue;

  return strtoull(attr->val, NULL, 10);
}

/*!
 * @brief creates number (float) from string value
 *
 * @param[in] attr         xml attribute
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
float
xmla_float(const xml_attr_t * __restrict attr, float defaultValue) {
  if (!attr || !attr->val)
    return defaultValue;

  return strtof(attr->val, NULL);
}

/*!
 * @brief creates number (double) from string value
 *
 * @param[in] attr         xml attribute
 * @param[in] defaultValue default value if operation fails
 * @return number
 */
XML_INLINE
double
xmla_double(const xml_attr_t * __restrict attr, double defaultValue) {
  if (!attr || !attr->val)
    return defaultValue;

  return strtod(attr->val, NULL);
}

/*!
 * @brief creates boolean from string value
 *
 * @param[in] attr         xml attribute
 * @param[in] defaultValue default value if operation fails
 * @return boolean values as integer: 1 true, 0 false, error: defaultValue
 */
XML_INLINE
int
xmla_bool(const xml_attr_t * __restrict attr, int defaultValue) {
  const char *boolString;
  char        first;

  if (!attr || !(boolString = attr->val))
    return defaultValue;

  first = boolString[0];

  if (first == 't' || first == '1') {
    return 1;
  } else if (first == 'n' || first == '0') {
    return 0;
  }

  return defaultValue;
}

#endif /* xml_attr_h */
