/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CHECK(test) do { \
  if (!(test)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #test); \
    return 1; \
  } \
} while (0)

static void  *pointers[8];
static size_t sizes[8], calls, failAt, live, peak;

static void*
tracked_malloc(size_t size) {
  void  *pointer;
  size_t i;

  if (++calls == failAt)
    return NULL;
  if (!(pointer = malloc(size)))
    return NULL;

  for (i = 0; i < 8; i++) {
    if (!pointers[i]) {
      pointers[i] = pointer;
      sizes[i]    = size;
      live       += size;
      if (live > peak)
        peak = live;
      return pointer;
    }
  }
  abort();
}

static void
tracked_free(void *pointer) {
  size_t i;

  if (!pointer)
    return;
  for (i = 0; i < 8; i++) {
    if (pointers[i] == pointer) {
      live       -= sizes[i];
      pointers[i] = NULL;
      free(pointer);
      return;
    }
  }
  abort();
}

#define malloc tracked_malloc
#define free tracked_free
#define xmlc_validate_expanded_attributes tracked_attributes
#include "../src/namespace_attributes.c"
#undef xmlc_validate_expanded_attributes
#undef malloc
#undef free

static int
failures(void) {
  char               source[32768];
  xml_doc_t         *doc;
  const char        *invalid;
  size_t             i, j, size, groups, duplicate, count, bytes, cap, lower, upper;
  xml_parse_status_t status, expected;

  for (groups = 1; groups < 3; groups++) {
    for (duplicate = 0; duplicate < 2; duplicate++) {
      size = 2;
      memcpy(source, "<r", 2);
      for (i = 0; i < 9; i++) {
        size += (size_t)snprintf(source + size, sizeof(source) - size, " xmlns:p%zu='", i);
        for (j = 0; j < 96; j++) {
          memcpy(source + size, "&#97;", 5);
          size += 5;
        }
        size += (size_t)snprintf(source + size, sizeof(source) - size, "%zu'", duplicate && i == 8 ? 0 : i);
      }
      for (j = 0; j < groups; j++) {
        for (i = 0; i < 9; i++) {
          size += (size_t)snprintf(source + size, sizeof(source) - size, " p%zu:%c='v'", i, (int)('a' + j));
        }
      }
      memcpy(source + size, "/>", 2);
      size += 2;
      CHECK((doc = xmlc_parse_span_checked(source, size, XML_READONLY | XML_PREFIXES, NULL, NULL)));
      CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, NULL) == XML_PARSE_OK);
      expected = duplicate ? XML_PARSE_INVALID_DATA : XML_PARSE_OK;
      calls = failAt = peak = 0;
      CHECK(tracked_attributes(doc, SIZE_MAX, SIZE_MAX, &invalid) == expected && !live);
      count = calls;
      bytes = peak;
      CHECK(count == groups + 1 && bytes > 128);
      for (i = 1; i <= count; i++) {
        calls  = peak = 0;
        failAt = i;
        CHECK(tracked_attributes(doc, SIZE_MAX, SIZE_MAX, &invalid) == XML_PARSE_OUT_OF_MEMORY);
        CHECK(!live && invalid >= source && invalid < source + size);
      }
      for (cap = bytes - 1; cap <= bytes + 1; cap++) {
        calls = failAt = peak = 0;
        status = tracked_attributes(doc, SIZE_MAX, cap, &invalid);
        CHECK(status == (cap < bytes ? XML_PARSE_LIMIT : expected));
        CHECK(!live && peak <= cap);
      }
      lower = 0;
      upper = 1;
      for (;;) {
        calls = failAt = peak = 0;
        status = tracked_attributes(doc, upper, SIZE_MAX, &invalid);
        if (status == expected)
          break;
        CHECK(status == XML_PARSE_LIMIT && !live && peak <= bytes);
        CHECK(upper < 1048576);
        lower  = upper;
        upper *= 2;
      }
      while (upper - lower > 1) {
        cap = lower + (upper - lower) / 2;
        calls = failAt = peak = 0;
        status = tracked_attributes(doc, cap, SIZE_MAX, &invalid);
        CHECK(!live && peak <= bytes);
        if (status == expected)
          upper = cap;
        else {
          CHECK(status == XML_PARSE_LIMIT);
          lower = cap;
        }
      }
      for (i = 0; i <= 32; i++) {
        cap = upper * i / 32;
        calls = failAt = peak = 0;
        status = tracked_attributes(doc, cap, SIZE_MAX, &invalid);
        CHECK(status == (cap < upper ? XML_PARSE_LIMIT : expected));
        CHECK(!live && peak <= bytes);
      }
      printf("groups=%zu duplicate=%zu calls=%zu peak=%zu work=%zu: pass\n", groups, duplicate, count, bytes, upper);
      xmlc_free(doc);
    }
  }
  return 0;
}

int
main(void) {
  CHECK(!failures());
  return 0;
}
