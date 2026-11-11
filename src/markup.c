/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../include/xml/call/xml.h"

static const char*
find_end(const char *begin, const char *limit, char marker) {
  const char *p;
  size_t      prefix = marker == '?' ? 1 : 2;

  /* skip long runs of either character without a search call per byte. */
  p = memchr(begin, '>', (size_t)(limit - begin));
  while (p) {
    if ((size_t)(p - begin) >= prefix && p[-1] == marker && (prefix == 1 || p[-2] == marker))
      return p + 1;
    p++;
    if (!(p = memchr(p, marker, (size_t)(limit - p))))
      return NULL;
    if ((size_t)(limit - p) <= prefix)
      return NULL;
    p += prefix;
    p = memchr(p, '>', (size_t)(limit - p));
  }
  return NULL;
}

XML_EXPORT
xml_parse_status_t
xmlc_visit_markup(const char           *contents,
                   size_t                size,
                   size_t                max_scan_bytes,
                   xml_markup_visitor_t  visitor,
                   void                 *context,
                   xml_parse_error_t    *error) {
  xml_markup_span_t span;
  const char       *p, *window, *bang = NULL, *question = NULL, *found, *open, *next, *limit;
  size_t            scanned, remaining, offset = 0;
  xml_parse_status_t status = XML_PARSE_OK;

  if (error) {
    error->status = XML_PARSE_INVALID_ARGUMENT;
    error->offset = 0;
  }
  if ((!contents && size) || size > PTRDIFF_MAX || !visitor)
    return XML_PARSE_INVALID_ARGUMENT;
  if (!size)
    goto done;

  scanned = size < max_scan_bytes ? size : max_scan_bytes;
  limit   = contents + scanned;
  p       = window = contents;

  while (p < limit) {
    /* batch punctuation searches without scanning an entire later payload. */
    if (p >= window) {
      remaining = (size_t)(limit - p);
      window    = p + (remaining < 4096 ? remaining : 4096);
      bang      = memchr(p, '!', (size_t)(window - p));
      question  = memchr(p, '?', (size_t)(window - p));
    }
    if (!bang && !question) {
      p = window;
      continue;
    }
    if (!question || (bang && bang < question))
      found = bang;
    else
      found = question;
    next = found + 1;
    if (found == contents || found[-1] != '<') {
      /* another annotation requires another literal '<'. */
      if (!(next = memchr(next, '<', (size_t)(limit - next))))
        break;
      next++;
      goto advance;
    }

    open      = found - 1;
    remaining = (size_t)(limit - open);
    if (open[1] == '?') {
      if (!(next = find_end(open + 2, limit, '?')))
        goto unfinished;
      span.kind = XML_MARKUP_PI;
      if (next - open > 5 && !memcmp(open + 2, "xml", 3) && xml__ascii_space(open[5]))
        span.kind = XML_MARKUP_DECLARATION;
    } else if (open[1] == '!') {
      if (remaining < 4)
        goto unfinished;
      if (!memcmp(open, "<!--", 4)) {
        if (!(next = find_end(open + 4, limit, '-')))
          goto unfinished;
        span.kind = XML_MARKUP_COMMENT;
      } else if (open[2] == '[') {
        if (remaining < 9)
          goto unfinished;
        if (memcmp(open, "<![CDATA[", 9))
          goto invalid;
        if (!(next = find_end(open + 9, limit, ']')))
          goto unfinished;
        goto advance;
      } else {
        goto invalid;
      }
    }

    span.offset = (size_t)(open - contents);
    span.size   = (size_t)(next - open);
    status      = visitor(context, &span);
    if (status != XML_PARSE_OK) {
      offset = span.offset;
      goto done;
    }
advance:
    p = next;
    if (p < window) {
      if (bang && bang < p)
        bang = memchr(p, '!', (size_t)(window - p));
      if (question && question < p)
        question = memchr(p, '?', (size_t)(window - p));
    }
  }

  status = scanned < size ? XML_PARSE_LIMIT : XML_PARSE_OK;
  offset = scanned;
  goto done;

unfinished:
  status = scanned < size ? XML_PARSE_LIMIT : XML_PARSE_INVALID_DATA;
  offset = scanned;
  goto done;
invalid:
  status = XML_PARSE_INVALID_DATA;
  offset = (size_t)(open - contents);
done:
  if (error) {
    error->status = status;
    error->offset = offset;
  }
  return status;
}

XML_EXPORT
xml_parse_status_t
xmlc_visit_source(const char           *contents,
                   size_t                size,
                   size_t                max_scan_bytes,
                   xml_source_visitor_t  visitor,
                   void                 *context,
                   xml_parse_error_t    *error) {
  xml_source_span_t span;
  const char       *p, *next, *end, *limit;
  size_t            scanned, remaining, offset = 0;
  xml_parse_status_t status = XML_PARSE_OK;
  char              quote;

  if (error) {
    error->status = XML_PARSE_INVALID_ARGUMENT;
    error->offset = 0;
  }
  if ((!contents && size) || size > PTRDIFF_MAX || !visitor)
    return XML_PARSE_INVALID_ARGUMENT;
  if (!size)
    goto done;

  scanned = size < max_scan_bytes ? size : max_scan_bytes;
  limit   = contents + scanned;
  p       = contents;

  while (p < limit) {
    remaining = (size_t)(limit - p);
    if (p == contents && (unsigned char)*p == 0xef) {
      if (remaining < 3)
        goto unfinished;
      if (memcmp(p, "\xef\xbb\xbf", 3))
        goto invalid;
      next      = p + 3;
      span.kind = XML_SOURCE_BOM;
    } else if (*p != '<') {
      next = memchr(p, '<', remaining);
      if (!next) {
        if (scanned < size)
          goto unfinished;
        next = limit;
      }
      span.kind = XML_SOURCE_TEXT;
    } else {
      if (remaining < 2)
        goto unfinished;
      if (p[1] == '?') {
        if (!(next = find_end(p + 2, limit, '?')))
          goto unfinished;
        span.kind = XML_SOURCE_PI;
        if (next - p > 5 && !memcmp(p + 2, "xml", 3) && xml__ascii_space(p[5]))
          span.kind = XML_SOURCE_DECLARATION;
      } else if (p[1] == '!') {
        if (remaining < 4)
          goto unfinished;
        if (!memcmp(p, "<!--", 4)) {
          if (!(next = find_end(p + 4, limit, '-')))
            goto unfinished;
          span.kind = XML_SOURCE_COMMENT;
        } else {
          if (remaining < 9)
            goto unfinished;
          if (memcmp(p, "<![CDATA[", 9))
            goto invalid;
          if (!(next = find_end(p + 9, limit, ']')))
            goto unfinished;
          span.kind = XML_SOURCE_CDATA;
        }
      } else {
        /* checked attribute values can contain '>', but never a literal '<'. */
        next = p + 2;
        while (next < limit && *next != '>') {
          quote = *next++;
          if (quote == '\'' || quote == '"') {
            if (!(end = memchr(next, quote, (size_t)(limit - next))))
              goto unfinished;
            next = end + 1;
          }
        }
        if (next == limit)
          goto unfinished;
        next++;
        span.kind = XML_SOURCE_START;
        if (p[1] == '/')
          span.kind = XML_SOURCE_END;
        else if (next[-2] == '/')
          span.kind = XML_SOURCE_EMPTY;
      }
    }

    span.offset = (size_t)(p - contents);
    span.size   = (size_t)(next - p);
    status      = visitor(context, &span);
    if (status != XML_PARSE_OK) {
      offset = span.offset;
      goto done;
    }
    p = next;
  }
  status = scanned < size ? XML_PARSE_LIMIT : XML_PARSE_OK;
  offset = scanned;
  goto done;

unfinished:
  status = scanned < size ? XML_PARSE_LIMIT : XML_PARSE_INVALID_DATA;
  offset = scanned;
  goto done;
invalid:
  status = XML_PARSE_INVALID_DATA;
  offset = (size_t)(p - contents);
done:
  if (error) {
    error->status = status;
    error->offset = offset;
  }
  return status;
}
