/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static size_t calls, failAt, live;

static void*
test_calloc(size_t count, size_t size) {
  void *result;

  if (++calls == failAt)
    return NULL;

  result = calloc(count, size);
  if (result)
    live++;

  return result;
}

static void
test_free(void *pointer) {
  if (pointer)
    live--;

  free(pointer);
}

#define calloc test_calloc
#define free test_free
#include <xml/xml.h>
#undef calloc
#undef free

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

  for (span = 0; span < 2; span++) {
    for (mode = 0; mode < 8; mode++) {
      memcpy(copy, source, size + 1);
      calls = failAt = 0;
      doc = span ? xml_parse_span(copy, size, (xml_options_t)mode, &error)
                 : xml_parse(copy, (xml_options_t)mode);
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
        doc = span ? xml_parse_span(copy, size, (xml_options_t)mode, &error)
                   : xml_parse(copy, (xml_options_t)mode);
        if (doc || live
            || (span && (error.status != XML_PARSE_OUT_OF_MEMORY || error.offset > size))) {
          free(copy);
          return 4;
        }
      }
    }
  }
  free(copy);
  printf("all %zu allocation failures, 8 modes, both APIs release prior pages: pass\n", allocationCount);
  return 0;
}

int
main(void) {
  char   source[32768];
  size_t i, offset;

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
  return check_failures(source);
}
