/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_util_h
#define xml_util_h

#include "common.h"
#include <stdlib.h>
#include <string.h>

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

#endif /* xml_util_h */
