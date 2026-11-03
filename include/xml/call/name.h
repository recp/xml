/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_call_name_h
#define xml_call_name_h
#ifdef __cplusplus
extern "C" {
#endif

#include "../name.h"

/* same bounded name syntax and error contract as xml_validate_name_span(). */
XML_EXPORT
xml_parse_status_t
xmlc_validate_name_span(const char        *contents,
                        size_t             size,
                        xml_name_kind_t    kind,
                        xml_parse_error_t *error);

#ifdef __cplusplus
}
#endif
#endif /* xml_call_name_h */
