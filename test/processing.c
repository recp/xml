/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <xml/call/xml.h>
#include <stdio.h>

#define CHECK(test) do { \
  if (!(test)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #test); \
    return 1; \
  } \
} while (0)

typedef struct Fixture {
  const char *source;
  size_t      nameError, namespaceError;
} Fixture;

static int
fixtures(void) {
  const Fixture fixtures[] = {
    {"<?pi?><r/>", SIZE_MAX, SIZE_MAX},
    {"<?xml-stylesheet?><r/>", SIZE_MAX, SIZE_MAX},
    {"<?\xcf\x80 data?><r/>", SIZE_MAX, SIZE_MAX},
    {"<?xml version='1.0' encoding='UTF-8' standalone='yes'?><r/>", SIZE_MAX, SIZE_MAX},
    {"<r><?pi a:b?></r>", SIZE_MAX, SIZE_MAX},
    {"<?pi data <?p:q?><r/>", SIZE_MAX, SIZE_MAX},
    {"<r><![CDATA[<?p:q?>]]></r>", SIZE_MAX, SIZE_MAX},
    {"<r><!--<?p:q?>--></r>", SIZE_MAX, SIZE_MAX},
    {"<r a='&lt;?p:q?>'/>", SIZE_MAX, SIZE_MAX},
    {"<r>&lt;?p:q?></r>", SIZE_MAX, SIZE_MAX},
    {"<?p:q?><r/>", SIZE_MAX, 3},
    {"<?:p?><r/>", SIZE_MAX, 2},
    {"<?p:?><r/>", SIZE_MAX, 3},
    {"<?a:b:c?><r/>", SIZE_MAX, 3},
    {"<r><?p:q?></r>", SIZE_MAX, 6},
    {"<?\xcf\x80:p?><r/>", SIZE_MAX, 4},
    {"<r xmlns:p='u'><?p:q?></r>", SIZE_MAX, 18},
    {"<?xml:p?><r/>", SIZE_MAX, 5},
    {"<?xmlns:p?><r/>", SIZE_MAX, 7},
    {"<?1p:q?><r/>", 2, 2},
    {"<?p:?data?><r/>", 4, 3},
    {"<?p:\xc0\x80?><r/>", 4, 3},
    {"<?\xc0\x80:p?><r/>", 2, 2},
    {"<?\xed\xa0\x80:p?><r/>", 2, 2},
    {"<?p?data?><r/>", 3, 3},
    {"<?p \x01?><r/>", 4, 4},
    {"<?XML?><r/>", 2, 2},
    {"<?xml?><r/>", 5, 5},
    {"<?" "?><r/>", 2, 2},
  };
  const xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_parse_error_t error;
  xml_doc_t        *doc;
  char             *input;
  size_t            i, size, expected, prefix;
  unsigned          mode, bounded;

  for (mode = 0; mode < 16; mode++) {
    for (bounded = 0; bounded < 2; bounded++) {
      for (i = 0; i < XML_ARR_LEN(fixtures); i++) {
        size     = strlen(fixtures[i].source);
        expected = mode & XML_PREFIXES ? fixtures[i].namespaceError : fixtures[i].nameError;
        CHECK((input = malloc(size)));
        memcpy(input, fixtures[i].source, size);
        doc = xmlc_parse_span_checked(input, size, (xml_options_t)mode, bounded ? &limits : NULL, &error);
        if (error.offset != (expected == SIZE_MAX ? size : expected))
          fprintf(stderr, "fixture %zu mode %u bounded %u status %u offset %zu\n",
                  i, mode, bounded, (unsigned)error.status, error.offset);
        CHECK(!!doc == (expected == SIZE_MAX));
        CHECK(error.status == (doc ? XML_PARSE_OK : XML_PARSE_INVALID_DATA));
        CHECK(error.offset == (doc ? size : expected));
        if (mode & XML_READONLY)
          CHECK(!memcmp(input, fixtures[i].source, size));
        xmlc_free(doc);
        free(input);
      }
    }
  }

  /* exact source prefixes exercise bounded reads in both lexical modes. */
  for (i = 0; i < XML_ARR_LEN(fixtures); i++) {
    size = strlen(fixtures[i].source);
    for (prefix = 0; prefix <= size; prefix++) {
      for (mode = 0; mode < 2; mode++) {
        CHECK((input = malloc(prefix ? prefix : 1)));
        memcpy(input, fixtures[i].source, prefix);
        doc = xmlc_parse_span_checked(input, prefix, XML_READONLY | (mode ? XML_PREFIXES : 0), &limits, &error);
        CHECK(error.offset <= prefix && !memcmp(input, fixtures[i].source, prefix));
        xmlc_free(doc);
        free(input);
      }
    }
  }
  return 0;
}

static int
long_targets(void) {
  xml_parse_error_t error;
  xml_doc_t        *doc;
  char             *source, *input;
  size_t            length = 65536, size, colon;
  unsigned          mode, variant;

  size = length + 8;
  CHECK((source = malloc(size)));
  CHECK((input = malloc(size)));
  memcpy(source, "<?", 2);
  memset(source + 2, 'a', length);
  memcpy(source + length + 2, "?><r/>", 6);

  for (variant = 0; variant < 3; variant++) {
    colon = variant == 1 ? 2 : length + 1;
    if (variant)
      source[colon] = ':';

    for (mode = 0; mode < 16; mode++) {
      memcpy(input, source, size);
      doc = xmlc_parse_span_checked(input, size, (xml_options_t)mode, NULL, &error);
      if (variant && (mode & XML_PREFIXES)) {
        CHECK(!doc && error.status == XML_PARSE_INVALID_DATA && error.offset == colon);
      } else {
        CHECK(doc && error.status == XML_PARSE_OK && error.offset == size);
      }
      xmlc_free(doc);

      /* legacy unchecked parsing retains its prior skipped-target behavior. */
      memcpy(input, source, size);
      CHECK((doc = xmlc_parse_span(input, size, (xml_options_t)mode, &error)));
      xmlc_free(doc);
    }
    if (variant)
      source[colon] = 'a';
  }
  free(input);
  free(source);
  return 0;
}

static int
limits(void) {
  const char         source[] = "<?p:q?><r/>";
  xml_parse_limits_t limits = {sizeof(source) - 2, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_parse_error_t  error;
  xml_doc_t         *doc;

  CHECK(!xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY | XML_PREFIXES, &limits, &error));
  CHECK(error.status == XML_PARSE_LIMIT && error.offset == 0);
  limits.max_input_bytes  = SIZE_MAX;
  limits.max_memory_bytes = 0;
  CHECK(!xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY | XML_PREFIXES, &limits, &error));
  CHECK(error.status == XML_PARSE_LIMIT && error.offset == 0);
  limits.max_memory_bytes = SIZE_MAX;
  CHECK(!xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY | XML_PREFIXES, &limits, &error));
  CHECK(error.status == XML_PARSE_INVALID_DATA && error.offset == 3);
  CHECK((doc = xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY, &limits, &error)));
  xmlc_free(doc);
  return 0;
}

int
main(void) {
  CHECK(!fixtures());
  CHECK(!long_targets());
  CHECK(!limits());
  puts("PI Name/NCName modes, exact offsets, skipped markup, long targets and limits: pass");
  return 0;
}
