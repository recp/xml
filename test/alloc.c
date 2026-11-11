/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static size_t calls, failAt, live, bytes;

#if defined(XML_TEST_CHECKED)
static void  *pointers[128];
static size_t sizes[128], liveBytes;

static void
record_allocation(void *pointer, size_t size) {
  size_t i;

  for (i = 0; i < 128; i++) {
    if (!pointers[i]) {
      pointers[i] = pointer;
      sizes[i]    = size;
      liveBytes  += size;
      if (liveBytes > bytes)
        bytes = liveBytes;
      return;
    }
  }
  abort();
}

static void*
test_malloc(size_t size) {
  void *result;

  if (++calls == failAt)
    return NULL;

  result = malloc(size);
  if (result) {
    live++;
    record_allocation(result, size);
  }
  return result;
}
#endif

static void*
test_calloc(size_t count, size_t size) {
  void *result;

  if (++calls == failAt)
    return NULL;

  result = calloc(count, size);
  if (result) {
    live++;
#if defined(XML_TEST_CHECKED)
    record_allocation(result, count * size);
#else
    bytes += count * size;
#endif
  }

  return result;
}

static void
test_free(void *pointer) {
#if defined(XML_TEST_CHECKED)
  size_t i;
#endif

  if (pointer) {
    live--;
#if defined(XML_TEST_CHECKED)
    for (i = 0; i < 128; i++) {
      if (pointers[i] == pointer) {
        liveBytes  -= sizes[i];
        pointers[i] = NULL;
        break;
      }
    }
    if (i == 128)
      abort();
#endif
  }
  free(pointer);
}

#define calloc test_calloc
#define free test_free
#if defined(XML_TEST_CHECKED)
#  define malloc test_malloc
#  include "../src/checked.c"
#  undef malloc
#else
#include <xml/xml.h>
#endif
#undef calloc
#undef free

static xml_doc_t*
parse(unsigned api, char *copy, size_t size, unsigned mode, xml_parse_error_t *error) {
  const xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};

#if defined(XML_TEST_CHECKED)
  (void)api;
  return xmlc_parse_span_checked(copy, size, (xml_options_t)mode, &limits, error);
#else
  switch (api) {
    case 0: return xml_parse(copy, (xml_options_t)mode);
    case 1: return xml_parse_span(copy, size, (xml_options_t)mode, error);
    case 2: return xml_parse_span_limited(copy, size, (xml_options_t)mode, NULL, error);
    default: return xml_parse_span_limited(copy, size, (xml_options_t)mode, &limits, error);
  }
#endif
}

static int
check_caps(const char *source) {
  xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_parse_error_t  error;
  xml_doc_t         *doc;
  char             *copy;
  size_t            size, cap;
  unsigned          mode, kind;

  size = strlen(source);
  if (!(copy = malloc(size)))
    return 1;

  for (mode = 0; mode < 16; mode++) {
    for (kind = 0; kind < 4; kind++) {
      for (cap = 0; cap < 4; cap++) {
        limits.max_input_bytes  = SIZE_MAX;
        limits.max_memory_bytes = SIZE_MAX;
        limits.max_tokens       = SIZE_MAX;
        limits.max_depth        = SIZE_MAX;
        switch (kind) {
          case 0: limits.max_input_bytes = cap; break;
          case 1: limits.max_memory_bytes = sizeof(xml_doc_t) + cap * 65536; break;
          case 2: limits.max_tokens = cap; break;
          default: limits.max_depth = cap; break;
        }
        calls = failAt = bytes = 0;
        memcpy(copy, source, size);
#if defined(XML_TEST_CHECKED)
        doc = xmlc_parse_span_checked(copy, size, (xml_options_t)mode, &limits, &error);
#else
        doc = xml_parse_span_limited(copy, size, (xml_options_t)mode, &limits, &error);
#endif
        if (bytes > limits.max_memory_bytes || error.offset > size
            || (!doc && error.status != XML_PARSE_LIMIT)
            || (doc && error.status != XML_PARSE_OK)
            || (kind == 0 && calls)) {
          xml_free(doc);
          free(copy);
          return 2;
        }
        xml_free(doc);
        if (live) {
          free(copy);
          return 3;
        }
      }
    }
  }
  free(copy);
  return 0;
}

static int
check_failures(const char *source) {
  xml_parse_error_t error;
  xml_doc_t        *doc;
  char             *copy;
  size_t            i, size, allocationCount;
  unsigned          span, mode;

  size = strlen(source);
  if (!(copy = malloc(size + 1)))
    return 1;

  for (span = 0; span < 4; span++) {
    for (mode = 0; mode < 16; mode++) {
      memcpy(copy, source, size + 1);
      calls = failAt = 0;
      doc = parse(span, copy, size, mode, &error);
      if (!doc || (span && error.status != XML_PARSE_OK)) {
        free(copy);
        return 2;
      }
      allocationCount = calls;
      xml_free(doc);
      if (live) {
        free(copy);
        return 3;
      }
      for (i = 1; i <= allocationCount; i++) {
        memcpy(copy, source, size + 1);
        failAt = i;
        calls = 0;
        memset(&error, 0xff, sizeof(error));
        doc = parse(span, copy, size, mode, &error);
        if (doc || live
            || (span && (error.status != XML_PARSE_OUT_OF_MEMORY || error.offset > size))) {
          free(copy);
          return 4;
        }
      }
    }
  }
  free(copy);
#if defined(XML_TEST_CHECKED)
  printf("all %zu checked allocation failures, 16 modes, release prior pages: pass\n", allocationCount);
#else
  printf("all %zu allocation failures, 16 modes, legacy/span/limited APIs release prior pages: pass\n", allocationCount);
#endif
  return check_caps(source);
}

int
main(void) {
  char   source[32768];
  size_t i, offset;
#if defined(XML_TEST_CHECKED)
  size_t attribute;
#endif

  memcpy(source, "<r>", 3u);
  for (i = 0u; i < 2000u; i++)
    memcpy(source + 3u + i * 4u, "<x/>", 4u);

  memcpy(source + 8003u, "</r>", 5u);
  if (check_failures(source))
    return 1;

  /* fill the page with attributes so failure occurs at the attribute site. */
  memcpy(source, "<r ", 3u);
  offset = 3u;
  for (i = 0u; i < 2046u; i++)
    offset += (size_t)snprintf(source + offset, sizeof(source) - offset, "a%zu='x' ", i);

  memcpy(source + offset, "/>", 3u);
  if (check_failures(source))
    return 2;

  /* leave less than one text token after completing the child elements. */
  memcpy(source, "<r>", 3u);
  for (i = 0u; i < 1022u; i++)
    memcpy(source + 3u + i * 4u, "<x/>", 4u);

  memcpy(source + 4091u, "text</r>", 9u);
  if (check_failures(source))
    return 3;

#if defined(XML_TEST_CHECKED)
  offset = (size_t)snprintf(source, sizeof(source), "<r>");
  for (i = 0; i < 2; i++) {
    offset += (size_t)snprintf(source + offset, sizeof(source) - offset, "<n ");
    for (attribute = 0; attribute < 128; attribute++) {
      offset += (size_t)snprintf(source + offset, sizeof(source) - offset, "a%zu='v' ", attribute);
    }
    offset += (size_t)snprintf(source + offset, sizeof(source) - offset, "/>");
  }
  memcpy(source + offset, "</r>", 5);
  if (check_failures(source))
    return 4;
#endif

  /* retained CDATA fills another token page in preserve mode. */
  memcpy(source, "<r>", 3);
  for (i = 0; i < 1500; i++)
    memcpy(source + 3 + i * 13, "<![CDATA[x]]>", 13);
  memcpy(source + 19503, "</r>", 5);
  return check_failures(source);
}
