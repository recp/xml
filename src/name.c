/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../include/xml/call/name.h"

XML_EXPORT
xml_parse_status_t
xmlc_validate_name_span(const char        *contents,
                        size_t             size,
                        xml_name_kind_t    kind,
                        xml_parse_error_t *error) {
  return xml_validate_name_span(contents, size, kind, error);
}
