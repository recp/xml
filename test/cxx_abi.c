/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <xml/call/xml.h>

void
xml_test_layout(size_t *layout) {
  const size_t values[] = {
    sizeof(xml_t), sizeof(xml_attr_t), sizeof(xml_doc_t), sizeof(xml_mem_t),
    sizeof(xml_parse_limits_t), sizeof(xml_parse_error_t), sizeof(xml_markup_span_t),
    offsetof(xml_t, val), offsetof(xml_t, valsize), offsetof(xml_t, tagsize),
    offsetof(xml_attr_t, valsize), offsetof(xml_attr_t, namesize),
    offsetof(xml_doc_t, root), offsetof(xml_mem_t, size), offsetof(xml_mem_t, capacity)
  };

  memcpy(layout, values, sizeof(values));
}

/* exercise C-created tokens through C++ access and release, and the reverse. */
xml_doc_t*
xml_test_parse(char *source, size_t size, xml_options_t options, xml_parse_error_t *error) {
  return xml_parse_span(source, size, options, error);
}

void
xml_test_free(xml_doc_t *document) {
  xml_free(document);
}
