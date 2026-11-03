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
  xml_doc_t *doc;
  size_t     i, allocationCount;

  calls = failAt = 0u;
  doc = xml_parse(source, XML_READONLY);
  if (!doc)
    return 1;

  allocationCount = calls;
  xml_free(doc);
  if (live)
    return 2;

  for (i = 1u; i <= allocationCount; i++) {
    failAt = i;
    calls = 0u;
    doc = xml_parse(source, XML_READONLY);
    if (doc || live)
      return 3;
  }
  printf("all %zu document/page allocation failures release prior pages: pass\n", allocationCount);
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
