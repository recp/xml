/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <xml/call/xml.h>
#include <stdio.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

static xml_doc_t*
parse(unsigned api, char *input, size_t size, unsigned mode,
      const xml_parse_limits_t *limits, xml_parse_error_t *error) {
  return api ? xmlc_parse_span_limited(input, size, (xml_options_t)mode, limits, error)
             : xml_parse_span_limited(input, size, (xml_options_t)mode, limits, error);
}

static int
check(const char *source, const xml_parse_limits_t *limits, bool success) {
  xml_parse_error_t error;
  xml_doc_t        *doc;
  xml_mem_t        *page;
  char             *input;
  size_t            size, bytes;
  unsigned          api, mode;

  size = strlen(source);
  for (api = 0; api < 2; api++) {
    for (mode = 0; mode < 16; mode++) {
      CHECK((input = malloc(size)));
      memcpy(input, source, size);
      doc = parse(api, input, size, mode, limits, &error);
      CHECK(!!doc == success && error.offset <= size);
      if (success) {
        CHECK(error.status == XML_PARSE_OK && error.offset == size);
        bytes = sizeof(*doc);
        for (page = doc->memroot; page; page = page->next)
          bytes += sizeof(*page) + page->capacity;
        CHECK(!limits || bytes <= limits->max_memory_bytes);
      } else {
        CHECK(error.status == XML_PARSE_LIMIT);
      }
      if (mode & XML_READONLY)
        CHECK(!memcmp(input, source, size));
      xml_free(doc);
      free(input);
    }
  }
  return 0;
}

static int
boundaries(void) {
  char               source[] = "<r a='v'><x/>text<![CDATA[v]]></r>";
  xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_parse_error_t  error;
  xml_doc_t         *doc;
  char              *input;
  size_t             size, tokens;
  unsigned           api, mode;

  CHECK(!check("<r/>", NULL, true));
  limits.max_input_bytes = 4;
  CHECK(!check("<r/>", &limits, true));
  limits.max_input_bytes--;
  CHECK(!check("<r/>", &limits, false));
  limits.max_input_bytes = SIZE_MAX;

  limits.max_memory_bytes = sizeof(xml_doc_t) + sizeof(xml_mem_t) + XML_MEM_PAGE;
  CHECK(!check("<r/>", &limits, true));
  limits.max_memory_bytes--;
  CHECK(!check("<r/>", &limits, false));
  limits.max_memory_bytes = SIZE_MAX;

  limits.max_tokens = 1;
  limits.max_depth = 1;
  CHECK(!check("<r/>", &limits, true));
  limits.max_tokens = 0;
  CHECK(!check("<r/>", &limits, false));
  limits.max_tokens = SIZE_MAX;
  limits.max_depth = 0;
  CHECK(!check("<r/>", &limits, false));
  limits.max_depth = 1;
  CHECK(!check("<r><x/></r>", &limits, false));
  limits.max_depth = 2;
  CHECK(!check("<r><x/><y/><z/></r>", &limits, true));
  CHECK(!check("<r><x><y/></x></r>", &limits, false));
  limits.max_depth = SIZE_MAX;

  size = strlen(source);
  for (api = 0; api < 2; api++) {
    for (mode = 0; mode < 16; mode++) {
      CHECK((input = malloc(size)));
      tokens = (mode & XML_PRESERVE_TEXT) ? 5 : 4;
      limits.max_tokens = tokens;
      memcpy(input, source, size);
      CHECK((doc = parse(api, input, size, mode, &limits, &error)));
      xml_free(doc);
      limits.max_tokens--;
      memcpy(input, source, size);
      CHECK(!parse(api, input, size, mode, &limits, &error));
      CHECK(error.status == XML_PARSE_LIMIT && error.offset < size);
      memcpy(input, source, size);
      CHECK(!parse(api, input, size, mode, &limits, NULL));
      memcpy(input, "<r><![CDATA[]]></r>", 19);
      limits.max_tokens = 1;
      doc = parse(api, input, 19, mode, &limits, &error);
      CHECK(!!doc == !(mode & XML_PRESERVE_TEXT));
      CHECK(error.status == ((mode & XML_PRESERVE_TEXT) ? XML_PARSE_LIMIT : XML_PARSE_OK));
      xml_free(doc);
      free(input);
    }
  }
  return 0;
}

static int
pages(void) {
  xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, 2};
  char              *source;
  size_t             i, count, size, first;

  /* exactly fill the first page with as many nodes as fit, then cross it. */
  count = XML_MEM_PAGE / sizeof(xml_t) - 1;
  size = 7 + (count + 1) * 4;
  CHECK((source = malloc(size + 1)));
  memcpy(source, "<r>", 3);
  for (i = 0; i < count + 1; i++)
    memcpy(source + 3 + i * 4, "<x/>", 4);
  memcpy(source + size - 4, "</r>", 5);

  first = sizeof(xml_doc_t) + sizeof(xml_mem_t) + XML_MEM_PAGE;
  limits.max_memory_bytes = first;
  CHECK(!check(source, &limits, false));
  limits.max_memory_bytes += sizeof(xml_mem_t) + XML_MEM_PAGE;
  CHECK(!check(source, &limits, true));
  limits.max_memory_bytes--;
  CHECK(!check(source, &limits, false));
  memcpy(source + 3 + count * 4, "</r>", 5);
  limits.max_memory_bytes = first;
  CHECK(!check(source, &limits, true));
  free(source);
  return 0;
}

static int
deep(void) {
  xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, 10000};
  char              *source;
  size_t             i;

  CHECK((source = malloc(70001)));
  for (i = 0; i < 10000; i++) {
    memcpy(source + i * 3, "<r>", 3);
    memcpy(source + 30000 + i * 4, "</r>", 4);
  }
  source[70000] = '\0';
  CHECK(!check(source, &limits, true));
  limits.max_depth--;
  CHECK(!check(source, &limits, false));
  free(source);
  return 0;
}

static int
arguments(void) {
  xml_parse_limits_t limits = {0, 0, 0, 0};
  xml_parse_error_t  error;
  char              nul[] = "<r>\0</r>";
  char             *input;
  unsigned          api;

  CHECK((input = malloc(1)));
  input[0] = '<';
  for (api = 0; api < 2; api++) {
    CHECK(!parse(api, input, 65536, XML_READONLY, &limits, &error));
    CHECK(error.status == XML_PARSE_LIMIT && error.offset == 0);
    CHECK(!parse(api, NULL, 4, XML_READONLY, &limits, &error));
    CHECK(error.status == XML_PARSE_INVALID_ARGUMENT && error.offset == 0);
    CHECK(!parse(api, input, SIZE_MAX, XML_READONLY, &limits, &error));
    CHECK(error.status == XML_PARSE_INVALID_ARGUMENT && error.offset == 0);
    CHECK(!parse(api, input, 1, 16, &limits, &error));
    CHECK(error.status == XML_PARSE_INVALID_ARGUMENT && error.offset == 0);
    CHECK(!parse(api, input, 0, XML_READONLY, &limits, &error));
    CHECK(error.status == XML_PARSE_INVALID_DATA && error.offset == 0);
    limits.max_input_bytes = SIZE_MAX;
    CHECK(!parse(api, nul, sizeof(nul) - 1, XML_READONLY, &limits, &error));
    CHECK(error.status == XML_PARSE_INVALID_DATA && error.offset == 3);
    limits.max_input_bytes = 0;
  }
  free(input);
  return 0;
}

int
main(void) {
  CHECK(!boundaries());
  CHECK(!pages());
  CHECK(!deep());
  CHECK(!arguments());
  puts("inclusive input, owned memory, token and element-depth caps: pass");
  return 0;
}
