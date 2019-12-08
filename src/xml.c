/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../include/xml/call/xml.h"

static
xml_doc_t*
xmlc__parse_reverse(const char * __restrict contents) {
  return xml_parse(contents, true, true);
}

static
xml_doc_t*
xmlc__parse_reverse_nopref(const char * __restrict contents) {
  return xml_parse(contents, true, false);
}

static
xml_doc_t*
xmlc__parse_normal(const char * __restrict contents) {
  return xml_parse(contents, false, true);
}

static
xml_doc_t*
xmlc__parse_normal_nopref(const char * __restrict contents) {
  return xml_parse(contents, false, false);
}

XML_EXPORT
xml_doc_t*
xmlc_parse(const char * __restrict contents,
            bool                    reverse,
            bool                    separatePrefixes) {
  if (!reverse) {
    if (separatePrefixes)
      return xmlc__parse_normal(contents);
    else
      return xmlc__parse_normal_nopref(contents);
  }
  
  if (separatePrefixes)
    return xmlc__parse_reverse(contents);
  else
    return xmlc__parse_reverse_nopref(contents);
}

XML_EXPORT
void
xmlc_free(xml_doc_t * __restrict jsondoc) {
  xml_free(jsondoc);
}
