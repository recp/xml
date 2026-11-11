/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <stdbool.h>
#include <stddef.h>

static bool
xml__parse_check_name(const char *text, size_t size, bool qualified, size_t *offset);

static bool
xml__parse_check_value(const char *text, size_t size, bool attribute, size_t *offset);

static bool
xml__parse_check_chars(const char *text, size_t size, size_t *offset);

static bool
xml__parse_check_pi(const char *text, size_t size, size_t *offset);

/* specialize the same parser at lexical token boundaries. */
#define XML__PARSE_CHECKED
#include "../include/xml/name.h"
#include "../include/xml/call/xml.h"

static bool
xml__parse_check_name(const char *text, size_t size, bool qualified, size_t *offset) {
  xml_parse_error_t  error;
  xml_parse_status_t status;

  status  = xml_validate_name_span(text, size, qualified ? XML_QNAME : XML_NAME, &error);
  *offset = error.offset;
  return status == XML_PARSE_OK;
}

static bool
xml__parse_check_value(const char *text, size_t size, bool attribute, size_t *offset) {
  xml_parse_error_t  error;
  xml_parse_status_t status;

  status = xml_decode_span(text, size, attribute ? XML_DECODE_ATTRIBUTE : XML_DECODE_TEXT,
                            NULL, SIZE_MAX, NULL, &error);
  *offset = error.offset;
  return status == XML_PARSE_OK;
}

static bool
xml__parse_check_chars(const char *text, size_t size, size_t *offset) {
  size_t   i = 0, count;
  uint32_t c;

  while (i < size) {
    c = (unsigned char)text[i];
    if (c < 0x80) {
      if (!xml__decode_char(c))
        break;
      i++;
    } else {
      count = xml__decode_utf8((const unsigned char *)text + i, size - i, &c);
      if (!count)
        break;
      i += count;
    }
  }
  *offset = i;
  return i == size;
}

static bool
xml__parse_check_pi(const char *text, size_t size, size_t *offset) {
  size_t end = 0;

  while (end < size && !xml__ascii_space(text[end]))
    end++;
  if (!xml__parse_check_name(text, end, false, offset))
    return false;

  return xml__parse_check_chars(text, size, offset);
}

XML_EXPORT
xml_doc_t*
xmlc_parse_span_checked(const char               * __restrict contents,
                        size_t                               size,
                        xml_options_t                        options,
                        const xml_parse_limits_t            *limits,
                        xml_parse_error_t                   *error) {
  return xml_parse_span_limited(contents, size,
                                (xml_options_t)(options | XML_PRESERVE_TEXT), limits, error);
}
