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
  return xml_parse(contents, XML_REVERSE | XML_PREFIXES);
}

static
xml_doc_t*
xmlc__parse_reverse_nopref(const char * __restrict contents) {
  return xml_parse(contents, XML_REVERSE);
}

static
xml_doc_t*
xmlc__parse_normal(const char * __restrict contents) {
  return xml_parse(contents, XML_PREFIXES);
}

static
xml_doc_t*
xmlc__parse_normal_nopref(const char * __restrict contents) {
  return xml_parse(contents, XML_NONE);
}

/* retain separate default loops when optional text preservation is enabled. */
#if defined(_MSC_VER)
static __declspec(noinline)
#else
static __attribute__((noinline))
#endif
xml_doc_t*
xmlc__parse_default(const char * __restrict contents, xml_options_t options) {
  bool reverse, separatePrefixes;

  if (options & XML_READONLY)
    return xml__parse(contents,
                      (xml_options_t)((options | XML_READONLY) & ~XML_PRESERVE_TEXT),
                      NULL,
                      NULL,
                      NULL);

  reverse          = options & XML_REVERSE;
  separatePrefixes = options & XML_PREFIXES;

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

#if defined(_MSC_VER)
static __declspec(noinline)
#else
static __attribute__((noinline))
#endif
xml_doc_t*
xmlc__parse_span_default(const char        * __restrict contents,
                         size_t                        size,
                         xml_options_t                 options,
                         xml_parse_error_t            *error) {
  if (options & XML_READONLY)
    return xml__parse_span(contents,
                           size,
                           (xml_options_t)((options | XML_READONLY) & ~XML_PRESERVE_TEXT),
                           error,
                           NULL);

  return xml__parse_span(contents,
                         size,
                         (xml_options_t)(options & ~(XML_READONLY | XML_PRESERVE_TEXT)),
                         error,
                         NULL);
}

XML_EXPORT
xml_doc_t*
xmlc_parse(const char * __restrict contents, xml_options_t options) {
  if (options & XML_PRESERVE_TEXT)
    return xml_parse(contents, options);

  return xmlc__parse_default(contents, options);
}

XML_EXPORT
xml_doc_t*
xmlc_parse_span(const char        * __restrict contents,
                size_t                        size,
                xml_options_t                 options,
                xml_parse_error_t            *error) {
  if (options & XML_PRESERVE_TEXT)
    return xml_parse_span(contents, size, options, error);

  return xmlc__parse_span_default(contents, size, options, error);
}

XML_EXPORT
void
xmlc_free(xml_doc_t * __restrict jsondoc) {
  xml_free(jsondoc);
}

XML_EXPORT
xml_doc_t*
xmlc_parse_span_limited(const char               * __restrict contents,
                        size_t                               size,
                        xml_options_t                        options,
                        const xml_parse_limits_t            *limits,
                        xml_parse_error_t                   *error) {
  if (!limits)
    return xmlc_parse_span(contents, size, options, error);

  return xml_parse_span_limited(contents, size, options, limits, error);
}
