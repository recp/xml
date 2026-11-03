/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../include/xml/call/decode.h"

XML_EXPORT
xml_parse_status_t
xmlc_decode_span(const char        *contents,
                 size_t             size,
                 xml_decode_mode_t  mode,
                 char              *output,
                 size_t             capacity,
                 size_t            *decoded_size,
                 xml_parse_error_t *error) {
  return xml_decode_span(contents, size, mode, output, capacity, decoded_size, error);
}
