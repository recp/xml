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
  const char *duplicate;
} Fixture;

static int
fixtures(void) {
  const Fixture fixtures[] = {
    {"<svg/>", NULL},
    {"<r a='1' b='2' xml:lang='en'/>", NULL},
    {"<r xmlns='u' xmlns:p='u' a='1' p:a='2'/>", NULL},
    {"<r xmlns:p='u' xmlns:q='u' p:a='1' q:b='2'/>", NULL},
    {"<r xmlns:p='u' xmlns:q='v' p:a='1' q:a='2'/>", NULL},
    {"<r xmlns:p='u' xmlns:q='u' p:a='1' q:A='2'/>", NULL},
    {"<r xmlns:p='u' xmlns:q='u'><n xmlns:q='v' p:a='1' q:a='2'/></r>", NULL},
    {"<r xmlns:p='u' xmlns:q='v'><n p:a='1' q:a='2'/><n xmlns:q='u' p:a='1' q:b='2'/></r>", NULL},
    {"<r xmlns:p='http://x/%61' xmlns:q='http://x/a' p:a='1' q:a='2'/>", NULL},
    {"<r xmlns:p='http://x/A' xmlns:q='http://x/a' p:a='1' q:a='2'/>", NULL},
    {"<r xmlns:p='u\tx' xmlns:q='u&#9;x' p:a='1' q:a='2'/>", NULL},
    {"<r xmlns:p='u'><n p:a='1'/><n p:a='2'/></r>", NULL},
    {"<r xmlns:p='u' xmlns:q='u' p:a='1' q:a='2'/>", "q:a"},
    {"<r xmlns:XML='u' xmlns:q='u' XML:a='1' q:a='2'/>", "q:a"},
    {"<r xmlns:p='u' xmlns:q='u' p:xmlns='1' q:xmlns='2'/>", "q:xmlns"},
    {"<r p:a='1' q:a='2' xmlns:p='u' xmlns:q='u'/>", "q:a"},
    {"<r xmlns:p='u' xmlns:q='v'><n xmlns:q='u' p:a='1' q:a='2'/></r>", "q:a"},
    {"<p:r xmlns:p='u' xmlns:q='v'><p:n/><n xmlns:q='u' p:a='1' q:a='2'/></p:r>", "q:a"},
    {"<r xmlns:p='u' xmlns:q='u'><n xmlns:q='v'/><n p:a='1' q:a='2'/></r>", "q:a"},
    {"<r xmlns:p='u&amp;x' xmlns:q='u&#38;x' p:a='1' q:a='2'/>", "q:a"},
    {"<r xmlns:p='u\r\nx' xmlns:q='u x' p:a='1' q:a='2'/>", "q:a"},
    {"<r xmlns:p='u\tx' xmlns:q='u&#32;x' p:a='1' q:a='2'/>", "q:a"},
    {"<r xmlns:p='u' xmlns:q='u' p:\xcf\x80='1' q:\xcf\x80='2'/>", "q:\xcf\x80"},
    {"<r xmlns:p='u' xmlns:q='u' xmlns:s='u' p:a='1' q:a='2' s:a='3'/>", "q:a"},
    {"<r xmlns:p='u' xmlns:q='u' p:b='1' p:a='2' q:b='3' q:a='4'/>", "q:b"},
    {"<r xmlns:p='u' xmlns:q='u'>text<![CDATA[x]]><n p:a='1' q:a='2'/></r>", "q:a"},
  };
  xml_doc_t         *doc;
  const char        *invalid;
  char              *input, *copy;
  size_t             i, size;
  xml_parse_status_t status;
  unsigned           mode;

  for (mode = 0; mode < 16; mode++) {
    for (i = 0; i < XML_ARR_LEN(fixtures); i++) {
      size = strlen(fixtures[i].source);
      CHECK((input = malloc(size)));
      CHECK((copy = malloc(size)));
      memcpy(input, fixtures[i].source, size);
      CHECK((doc = xmlc_parse_span_checked(input, size, (xml_options_t)(mode | XML_PREFIXES), NULL, NULL)));
      CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, NULL) == XML_PARSE_OK);
      memcpy(copy, input, size);
      status = xmlc_validate_expanded_attributes(doc, SIZE_MAX, SIZE_MAX, &invalid);
      if (status != (fixtures[i].duplicate ? XML_PARSE_INVALID_DATA : XML_PARSE_OK))
        fprintf(stderr, "fixture %zu mode %u status %u\n", i, mode, (unsigned)status);
      CHECK(status == (fixtures[i].duplicate ? XML_PARSE_INVALID_DATA : XML_PARSE_OK));
      if (fixtures[i].duplicate)
        CHECK(invalid == input + (strstr(fixtures[i].source, fixtures[i].duplicate) - fixtures[i].source));
      else
        CHECK(!invalid);
      CHECK(!memcmp(copy, input, size));
      xmlc_free(doc);
      free(copy);
      free(input);
    }
  }
  return 0;
}

static int
lists(void) {
  const size_t counts[] = {0, 1, 2, 8, 9, 32, 128};
  char               source[32768], copy[32768];
  xml_doc_t         *doc;
  const char        *invalid;
  size_t             c, i, size, last;
  xml_parse_status_t status;
  unsigned           mode, duplicate;

  for (c = 0; c < XML_ARR_LEN(counts); c++) {
    for (duplicate = 0; duplicate < 2; duplicate++) {
      if (duplicate && counts[c] < 2)
        continue;
      size = 2;
      memcpy(source, "<r", 2);
      for (i = 0; i < counts[c]; i++) {
        size += (size_t)snprintf(source + size, sizeof(source) - size,
                                " xmlns:p%zu='urn:%zu'", i, duplicate && i == counts[c] - 1 ? 0 : i);
      }
      last = size + 1;
      for (i = 0; i < counts[c]; i++) {
        last = size + 1;
        size += (size_t)snprintf(source + size, sizeof(source) - size, " p%zu:a='v'", i);
      }
      memcpy(source + size, "/>", 2);
      size += 2;
      for (mode = 0; mode < 16; mode++) {
        memcpy(copy, source, size);
        CHECK((doc = xmlc_parse_span_checked(copy, size, (xml_options_t)(mode | XML_PREFIXES), NULL, NULL)));
        CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, NULL) == XML_PARSE_OK);
        status = xmlc_validate_expanded_attributes(doc, SIZE_MAX, SIZE_MAX, &invalid);
        CHECK(status == (duplicate ? XML_PARSE_INVALID_DATA : XML_PARSE_OK));
        CHECK(duplicate ? invalid == copy + last : !invalid);
        xmlc_free(doc);
      }
    }
  }
  return 0;
}

static int
shadowed_lists(void) {
  char               source[32768], copy[32768];
  xml_doc_t         *doc;
  const char        *invalid;
  size_t             i, size, last;
  xml_parse_status_t status;
  unsigned           mode, duplicate;

  for (duplicate = 0; duplicate < 2; duplicate++) {
    size = 2;
    memcpy(source, "<r", 2);
    for (i = 0; i < 16; i++) {
      size += (size_t)snprintf(source + size, sizeof(source) - size,
                              " xmlns:p%zu='urn:%zu'", i, i == 15 ? 0 : i);
    }
    size += (size_t)snprintf(source + size, sizeof(source) - size,
                            "><n xmlns:p15='urn:%zu'", (size_t)(duplicate ? 0 : 15));
    last = size + 1;
    for (i = 0; i < 16; i++) {
      last = size + 1;
      size += (size_t)snprintf(source + size, sizeof(source) - size, " p%zu:a='v'", i);
    }
    memcpy(source + size, "/></r>", 6);
    size += 6;
    for (mode = 0; mode < 16; mode++) {
      memcpy(copy, source, size);
      CHECK((doc = xmlc_parse_span_checked(copy, size, (xml_options_t)(mode | XML_PREFIXES), NULL, NULL)));
      CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, NULL) == XML_PARSE_OK);
      status = xmlc_validate_expanded_attributes(doc, SIZE_MAX, SIZE_MAX, &invalid);
      CHECK(status == (duplicate ? XML_PARSE_INVALID_DATA : XML_PARSE_OK));
      CHECK(duplicate ? invalid == copy + last : !invalid);
      xmlc_free(doc);
    }
  }
  return 0;
}

static int
long_values(void) {
  xml_doc_t  *doc;
  xml_attr_t *binding;
  const char *invalid;
  char       *source, *value;
  size_t      length = 65536, size;

  CHECK((source = malloc(length * 2 + 128)));
  CHECK((value = malloc(length + 1)));
  memset(value, 'a', length);
  value[length] = '\0';
  size = (size_t)snprintf(source, length * 2 + 128,
                          "<r xmlns:p='%s' xmlns:q='%s' p:a='1' q:a='2'/>", value, value);
  CHECK((doc = xmlc_parse_span_checked(source, size, XML_READONLY | XML_PREFIXES, NULL, NULL)));
  CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, NULL) == XML_PARSE_OK);
  CHECK(xmlc_validate_expanded_attributes(doc, SIZE_MAX, 0, &invalid) == XML_PARSE_INVALID_DATA);
  CHECK(invalid == strstr(source, "q:a"));

  /* an equal-length URI edit between calls must not reuse a stale resolution. */
  CHECK((binding = xmla(doc->root, "xmlns:q")));
  source[(size_t)(binding->val - source) + length - 1] = 'b';
  CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, NULL) == XML_PARSE_OK);
  CHECK(xmlc_validate_expanded_attributes(doc, SIZE_MAX, 0, &invalid) == XML_PARSE_OK && !invalid);
  xmlc_free(doc);
  free(value);
  free(source);
  return 0;
}

static int
limits_and_arguments(void) {
  const char source[] = "<r xmlns:p='u' xmlns:q='v' p:a='1' q:a='2'/>";
  xml_doc_t         *doc;
  const char        *invalid = source;
  size_t             cap;
  xml_parse_status_t status;

  CHECK(xmlc_validate_expanded_attributes(NULL, SIZE_MAX, SIZE_MAX, &invalid) == XML_PARSE_INVALID_ARGUMENT);
  CHECK(!invalid);
  CHECK((doc = xmlc_parse_span_checked("<r/>", 4, XML_READONLY, NULL, NULL)));
  CHECK(xmlc_validate_expanded_attributes(doc, SIZE_MAX, SIZE_MAX, &invalid) == XML_PARSE_INVALID_ARGUMENT);
  CHECK(!invalid);
  xmlc_free(doc);
  CHECK((doc = xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY | XML_PREFIXES, NULL, NULL)));
  CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, NULL) == XML_PARSE_OK);
  for (cap = 0; cap < 4096; cap++) {
    status = xmlc_validate_expanded_attributes(doc, cap, 0, &invalid);
    if (status == XML_PARSE_OK)
      break;
    CHECK(status == XML_PARSE_LIMIT && invalid >= source && invalid < source + sizeof(source) - 1);
  }
  CHECK(cap > 0 && cap < 4096 && !invalid);
  CHECK(xmlc_validate_expanded_attributes(doc, cap - 1, 0, NULL) == XML_PARSE_LIMIT);
  CHECK(xmlc_validate_expanded_attributes(doc, cap + 1, 0, NULL) == XML_PARSE_OK);
  xmlc_free(doc);
  return 0;
}

int
main(void) {
  CHECK(!fixtures());
  CHECK(!lists());
  CHECK(!shadowed_lists());
  CHECK(!long_values());
  CHECK(!limits_and_arguments());
  puts("expanded attributes, effective scope, normalized URI equality and work caps: pass");
  return 0;
}
