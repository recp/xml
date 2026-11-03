/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_call_decode_h
#define xml_call_decode_h
#ifdef __cplusplus
extern "C" {
#endif

#include "../decode.h"

/* same value-span, capacity and failure contract as xml_decode_span(). */
XML_EXPORT
xml_parse_status_t
xmlc_decode_span(const char        *contents,
                 size_t             size,
                 xml_decode_mode_t  mode,
                 char              *output,
                 size_t             capacity,
                 size_t            *decoded_size,
                 xml_parse_error_t *error);

#ifdef __cplusplus
}
#endif
#endif /* xml_call_decode_h */
