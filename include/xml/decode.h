/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_decode_h
#define xml_decode_h

#include "xml.h"

typedef enum xml_decode_mode_t {
  XML_DECODE_TEXT = 0,
  XML_DECODE_ATTRIBUTE,
  XML_DECODE_CDATA
} xml_decode_mode_t;

/*!
 * @brief decode one raw UTF-8 value span without allocating
 *
 * TEXT and ATTRIBUTE resolve amp/lt/gt/apos/quot and decimal/hex character
 * references once. CDATA treats ampersands literally. All modes validate UTF-8
 * and XML 1.0 characters and normalize literal CR/CRLF to LF. ATTRIBUTE applies
 * CDATA-typed attribute normalization: literal tab/LF/CRLF/CR becomes one space,
 * with no trimming/collapse. Character references retain the referenced scalar,
 * including tab/LF/CR. DTD-defined entities and tokenized attribute types are not
 * supported. TEXT rejects literal '<' and ']]>'; ATTRIBUTE rejects literal '<';
 * CDATA takes only the payload, without its delimiters, and rejects ']]>'.
 * Attribute quote delimiters and document grammar are the parser's concern.
 *
 * size excludes any terminator and must fit PTRDIFF_MAX. NULL contents is valid
 * only with size zero. Output may be NULL to validate/measure; capacity still
 * caps the decoded size, so use SIZE_MAX for an unrestricted measurement. Output
 * is not null-terminated and never exceeds input size. The buffers must be
 * disjoint or have exactly the same start for in-place decoding. Other overlap,
 * including overlap with decoded_size/error, is not supported.
 *
 * On success, optional decoded_size receives the byte count and error reports
 * OK at offset size. On failure, decoded_size is unchanged, error reports a
 * status and source offset, and output may contain an incomplete prefix. Discard
 * that prefix. A malformed reference/UTF-8 sequence reports its starting offset;
 * capacity exhaustion reports the start of the next output span. A failure may
 * precede validation of the suffix. Query first if atomic output is required.
 *
 * Work is linear in size, with fixed stack and no recursion, allocation, fetch,
 * input transcoding or implicit tree mutation. Do not decode a normalized value twice.
 * Select CDATA for a retained XML_STRING whose cdata flag is set. Raw parse tokens
 * and lengths stay unchanged; the document owner publishes decoded values only
 * after success. This does not establish full XML document conformance.
 */
XML_INLINE
xml_parse_status_t
xml_decode_span(const char        *contents,
                size_t             size,
                xml_decode_mode_t  mode,
                char              *output,
                size_t             capacity,
                size_t            *decoded_size,
                xml_parse_error_t *error);

XML_INLINE
bool
xml__decode_char(uint32_t value) {
  return value == 9 || value == 10 || value == 13
         || (value >= 0x20 && value <= 0xd7ff)
         || (value >= 0xe000 && value <= 0xfffd)
         || (value >= 0x10000 && value <= 0x10ffff);
}

XML_INLINE
size_t
xml__decode_utf8(const unsigned char *text, size_t size, uint32_t *value) {
  size_t   count, i;
  uint32_t scalar, minimum;

  if (text[0] >= 0xc2 && text[0] <= 0xdf) {
    count   = 2;
    scalar  = text[0] & 0x1f;
    minimum = 0x80;
  } else if (text[0] >= 0xe0 && text[0] <= 0xef) {
    count   = 3;
    scalar  = text[0] & 0x0f;
    minimum = 0x800;
  } else if (text[0] >= 0xf0 && text[0] <= 0xf4) {
    count   = 4;
    scalar  = text[0] & 0x07;
    minimum = 0x10000;
  } else {
    return 0;
  }
  if (size < count)
    return 0;

  for (i = 1; i < count; i++) {
    if ((text[i] & 0xc0) != 0x80)
      return 0;
    scalar = (scalar << 6) | (text[i] & 0x3f);
  }
  if (scalar < minimum || !xml__decode_char(scalar))
    return 0;

  *value = scalar;
  return count;
}

XML_INLINE
size_t
xml__decode_reference(const char *text, size_t size, uint32_t *value) {
  size_t        i;
  uint32_t      scalar = 0, base = 10, digit;
  unsigned char c;

  if (size < 4)
    return 0;

  if (text[1] != '#') {
    if (size >= 4 && !memcmp(text, "&lt;", 4)) {
      *value = '<';
      return 4;
    }
    if (size >= 4 && !memcmp(text, "&gt;", 4)) {
      *value = '>';
      return 4;
    }
    if (size >= 5 && !memcmp(text, "&amp;", 5)) {
      *value = '&';
      return 5;
    }
    if (size >= 6 && !memcmp(text, "&quot;", 6)) {
      *value = '"';
      return 6;
    }
    if (size >= 6 && !memcmp(text, "&apos;", 6)) {
      *value = '\'';
      return 6;
    }
    return 0;
  }

  i = 2;
  if (text[i] == 'x') {
    base = 16;
    i++;
  }
  if (i == size || text[i] == ';')
    return 0;

  for (; i < size && text[i] != ';'; i++) {
    c = (unsigned char)text[i];
    if (c >= '0' && c <= '9')
      digit = c - '0';
    else if (base == 16 && c >= 'a' && c <= 'f')
      digit = c - 'a' + 10;
    else if (base == 16 && c >= 'A' && c <= 'F')
      digit = c - 'A' + 10;
    else
      return 0;

    if (scalar > (0x10ffffu - digit) / base)
      return 0;
    scalar = scalar * base + digit;
  }
  if (i == size || !xml__decode_char(scalar))
    return 0;

  *value = scalar;
  return i + 1;
}

XML_INLINE
size_t
xml__decode_encode(uint32_t value, char output[4]) {
  if (value < 0x80) {
    output[0] = (char)value;
    return 1;
  }
  if (value < 0x800) {
    output[0] = (char)(0xc0 | (value >> 6));
    output[1] = (char)(0x80 | (value & 0x3f));
    return 2;
  }
  if (value < 0x10000) {
    output[0] = (char)(0xe0 | (value >> 12));
    output[1] = (char)(0x80 | ((value >> 6) & 0x3f));
    output[2] = (char)(0x80 | (value & 0x3f));
    return 3;
  }
  output[0] = (char)(0xf0 | (value >> 18));
  output[1] = (char)(0x80 | ((value >> 12) & 0x3f));
  output[2] = (char)(0x80 | ((value >> 6) & 0x3f));
  output[3] = (char)(0x80 | (value & 0x3f));
  return 4;
}

XML_INLINE
xml_parse_status_t
xml_decode_span(const char        *contents,
                size_t             size,
                xml_decode_mode_t  mode,
                char              *output,
                size_t             capacity,
                size_t            *decoded_size,
                xml_parse_error_t *error) {
  char                encoded[4];
  const char         *data;
  size_t              i = 0, start = 0, written = 0, count, consumed;
  uint32_t            value;
  xml_parse_status_t  status = XML_PARSE_INVALID_ARGUMENT;
  unsigned char       c;

  if ((!contents && size) || size > PTRDIFF_MAX
      || mode < XML_DECODE_TEXT || mode > XML_DECODE_CDATA)
    goto done;

  status = XML_PARSE_INVALID_DATA;
  while (i < size) {
    start = i;

    /* ordinary ASCII spans share one copy/capacity check. */
    while (i < size) {
      c = (unsigned char)contents[i];
      if (c < 0x20 || c >= 0x80 || c == ']'
          || (mode != XML_DECODE_CDATA && (c == '&' || c == '<')))
        break;
      i++;
    }
    if (i != start) {
      count = i - start;
      data  = contents + start;
    } else {
      c        = (unsigned char)contents[i];
      consumed = 1;

      if (c == '&' && mode != XML_DECODE_CDATA) {
        if (!(consumed = xml__decode_reference(contents + i, size - i, &value)))
          goto done;
        count = xml__decode_encode(value, encoded);
        data  = encoded;
      } else if (c >= 0x80) {
        if (!(consumed = xml__decode_utf8((const unsigned char *)contents + i, size - i, &value)))
          goto done;
        count = consumed;
        data  = contents + i;
      } else {
        if (!xml__decode_char(c) || (c == '<' && mode != XML_DECODE_CDATA)
            || (c == ']' && mode != XML_DECODE_ATTRIBUTE && size - i >= 3
                && contents[i + 1] == ']' && contents[i + 2] == '>'))
          goto done;

        if (c == '\r') {
          if (size - i >= 2 && contents[i + 1] == '\n')
            consumed++;
          c = '\n';
        }
        if (mode == XML_DECODE_ATTRIBUTE && (c == '\t' || c == '\n'))
          c = ' ';
        encoded[0] = (char)c;
        count      = 1;
        data       = encoded;
      }
      i += consumed;
    }
    if (count > capacity - written) {
      status = XML_PARSE_LIMIT;
      goto done;
    }
    if (output)
      memmove(output + written, data, count);
    written += count;
  }

  status = XML_PARSE_OK;
  start  = size;
  if (decoded_size)
    *decoded_size = written;
done:
  if (error) {
    error->status = status;
    error->offset = start;
  }
  return status;
}

#endif /* xml_decode_h */
