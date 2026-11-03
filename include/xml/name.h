/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_name_h
#define xml_name_h

#include "decode.h"

typedef enum xml_name_kind_t {
  XML_NAME = 0,
  XML_NCNAME,
  XML_QNAME
} xml_name_kind_t;

/*!
 * @brief validate one complete UTF-8 XML name without allocating
 *
 * XML_NAME uses XML 1.0 Fifth Edition Name syntax, including colons. XML_NCNAME
 * excludes colons. XML_QNAME is one NCName or two NCNames separated by a colon.
 * The input is literal: no entity decoding, trimming, Unicode normalization or
 * case folding. size excludes any terminator and must fit PTRDIFF_MAX; embedded
 * null bytes are invalid. The span need not be terminated and stays unchanged.
 * NULL contents is valid only with size zero, which fails as an empty name.
 *
 * Optional error reports OK at offset size, INVALID_DATA at the first invalid
 * scalar/UTF-8 sequence, or offset size for a missing local name after a colon.
 * Invalid arguments/kind report INVALID_ARGUMENT at offset zero. error must not
 * overlap contents. Work is linear with fixed stack and no allocation/recursion.
 *
 * This validates lexical syntax only. Namespace bindings, reserved prefix/URI
 * rules, duplicate attributes and document/PI grammar are separate checks. No
 * parser option or token length limit changes; callers select this explicitly.
 */
XML_INLINE
xml_parse_status_t
xml_validate_name_span(const char        *contents,
                       size_t             size,
                       xml_name_kind_t    kind,
                       xml_parse_error_t *error);

XML_INLINE
bool
xml__name_unicode_start(uint32_t c) {
  return (c >= 0xc0 && c <= 0xd6) || (c >= 0xd8 && c <= 0xf6)
         || (c >= 0xf8 && c <= 0x2ff) || (c >= 0x370 && c <= 0x37d)
         || (c >= 0x37f && c <= 0x1fff) || (c >= 0x200c && c <= 0x200d)
         || (c >= 0x2070 && c <= 0x218f) || (c >= 0x2c00 && c <= 0x2fef)
         || (c >= 0x3001 && c <= 0xd7ff) || (c >= 0xf900 && c <= 0xfdcf)
         || (c >= 0xfdf0 && c <= 0xfffd) || (c >= 0x10000 && c <= 0xeffff);
}

XML_INLINE
xml_parse_status_t
xml_validate_name_span(const char        *contents,
                       size_t             size,
                       xml_name_kind_t    kind,
                       xml_parse_error_t *error) {
  size_t              i = 0, count;
  uint32_t            c;
  xml_parse_status_t  status = XML_PARSE_INVALID_ARGUMENT;
  bool                first = true, colon = false;

  if ((!contents && size) || size > PTRDIFF_MAX
      || kind < XML_NAME || kind > XML_QNAME)
    goto done;

  status = XML_PARSE_INVALID_DATA;
  while (i < size) {
    c = (unsigned char)contents[i];
    if (c < 0x80) {
      if (c == ':') {
        if (kind == XML_NCNAME || (kind == XML_QNAME && (first || colon)))
          goto done;
        colon = true;
        first = kind == XML_QNAME;
      } else {
        if (!((c >= 'A' && c <= 'Z') || c == '_' || (c >= 'a' && c <= 'z'))
            && (first || !((c >= '0' && c <= '9') || c == '-' || c == '.')))
          goto done;
        first = false;
      }
      i++;
      continue;
    }
    if (!(count = xml__decode_utf8((const unsigned char *)contents + i, size - i, &c)))
      goto done;

    if (!xml__name_unicode_start(c)
        && (first || !(c == 0xb7 || (c >= 0x300 && c <= 0x36f)
                      || (c >= 0x203f && c <= 0x2040))))
      goto done;
    first = false;
    i += count;
  }
  if (!first)
    status = XML_PARSE_OK;

done:
  if (error) {
    error->status = status;
    error->offset = i;
  }
  return status;
}

#endif /* xml_name_h */
