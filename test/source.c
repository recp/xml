/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <xml/call/xml.h>
#include <stdio.h>
#if defined(__unix__) || defined(__APPLE__)
#  include <sys/mman.h>
#  include <unistd.h>
#endif

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

typedef struct Capture {
  xml_source_span_t spans[32];
  size_t            count;
  xml_parse_status_t failure;
} Capture;

static xml_parse_status_t
capture(void *context, const xml_source_span_t *span) {
  Capture *result;

  result = context;
  if (result->count == XML_ARR_LEN(result->spans))
    return XML_PARSE_LIMIT;
  result->spans[result->count++] = *span;
  return result->failure;
}

static int
fixtures(void) {
  const char *const sources[] = {
    "<r/>", "<r></r>", "<r>text<e/>tail</r>", "<r a='>' b=\"/>\">x</r>",
    "\357\273\277<?xml version='1.0'?>\r\n<r><!--a--><?p?><![CDATA[x<y]]></r>\t",
    " <!--a--><r> &amp; <x><!--inside--></x><!--after--></r><?p?> ",
    "<r><![CDATA[]]><![CDATA[a]]]></r>", "<r><x a='&lt;'/>x\317\200</r>",
    "<r><!-- >-x --><?p > < ?x ?></r>", "<?xml-stylesheet a='x'?><r/>"
  };
  Capture           full, result;
  xml_parse_error_t error;
  xml_doc_t        *document;
  size_t            i, j, cap, size, end;
  unsigned          status;

  for (i = 0; i < XML_ARR_LEN(sources); i++) {
    size = strlen(sources[i]);
    CHECK((document = xmlc_parse_span_checked(sources[i], size, XML_READONLY, NULL, NULL)));
    xmlc_free(document);
    memset(&full, 0, sizeof(full));
    CHECK(xmlc_visit_source(sources[i], size, SIZE_MAX, capture, &full, &error) == XML_PARSE_OK);
    end = 0;
    for (j = 0; j < full.count; j++) {
      CHECK(full.spans[j].offset == end && full.spans[j].size);
      end += full.spans[j].size;
    }
    CHECK(end == size && error.offset == size);
    for (cap = 0; cap <= size + 1; cap++) {
      memset(&result, 0, sizeof(result));
      CHECK(xmlc_visit_source(sources[i], size, cap, capture, &result, &error)
            == (cap < size ? XML_PARSE_LIMIT : XML_PARSE_OK));
      CHECK(error.offset == (cap < size ? cap : size));
      CHECK(result.count <= full.count);
      for (j = 0; j < result.count; j++) {
        CHECK(result.spans[j].offset == full.spans[j].offset);
        CHECK(result.spans[j].size == full.spans[j].size);
        CHECK(result.spans[j].kind == full.spans[j].kind);
        CHECK(result.spans[j].offset + result.spans[j].size <= cap);
      }
    }
    for (status = XML_PARSE_INVALID_ARGUMENT; status <= XML_PARSE_LIMIT; status++) {
      memset(&result, 0, sizeof(result));
      result.failure = (xml_parse_status_t)status;
      CHECK(xmlc_visit_source(sources[i], size, SIZE_MAX, capture, &result, &error) == result.failure);
      CHECK(result.count == 1 && error.offset == 0 && error.status == result.failure);
    }
  }
  memset(&result, 0, sizeof(result));
  CHECK(xmlc_visit_source(NULL, 0, 0, capture, &result, &error) == XML_PARSE_OK);
  CHECK(xmlc_visit_source(NULL, 1, 0, capture, &result, &error) == XML_PARSE_INVALID_ARGUMENT);
  CHECK(xmlc_visit_source("<r/>", 4, 4, NULL, NULL, &error) == XML_PARSE_INVALID_ARGUMENT);
  CHECK(xmlc_visit_source("x", (size_t)PTRDIFF_MAX + 1, 0, capture, &result, &error)
        == XML_PARSE_INVALID_ARGUMENT);
  CHECK(!result.count);
  return 0;
}

static int
guarded_prefix(void) {
#if defined(__unix__) || defined(__APPLE__)
  Capture           result = {0};
  xml_parse_error_t  error;
  xml_doc_t        *doc;
  char             *source;
  size_t            page, size;
  xml_parse_status_t partial, zero;

  page = (size_t)sysconf(_SC_PAGESIZE);
  size = page * 2;
  source = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  CHECK(source != MAP_FAILED);
  memcpy(source, "<r a='", 6);
  memset(source + 6, 'x', size - 9);
  memcpy(source + size - 3, "'/> ", 3);
  doc = xmlc_parse_span_checked(source, size, XML_READONLY, NULL, NULL);
  if (!doc) {
    munmap(source, size);
    return 1;
  }
  xmlc_free(doc);
  if (mprotect(source + page, page, PROT_NONE)) {
    munmap(source, size);
    return 1;
  }
  partial = xmlc_visit_source(source, size, page, capture, &result, &error);
  if (mprotect(source, size, PROT_NONE)) {
    munmap(source, size);
    return 1;
  }
  zero = xmlc_visit_source(source, size, 0, capture, &result, &error);
  munmap(source, size);
  CHECK(partial == XML_PARSE_LIMIT && zero == XML_PARSE_LIMIT && !error.offset && !result.count);
#else
  puts("guard-page prefix check unavailable on this platform");
#endif
  return 0;
}


static xml_parse_status_t
emit(void *context, const xml_source_span_t *span) {
  (void)context;
  printf("%u %zu %zu\n", (unsigned)span->kind, span->offset, span->size);
  return XML_PARSE_OK;
}

static int
oracle(void) {
  char              input[32769];
  xml_parse_error_t  error;
  xml_doc_t        *doc;
  char             *source;
  size_t            size, i;
  unsigned          byte;

  while (scanf("%32768s", input) == 1) {
    size = strlen(input) / 2;
    CHECK((source = malloc(size)));
    for (i = 0; i < size; i++) {
      CHECK(sscanf(input + i * 2, "%2x", &byte) == 1);
      source[i] = (char)byte;
    }
    CHECK((doc = xmlc_parse_span_checked(source, size, XML_READONLY, NULL, NULL)));
    xmlc_free(doc);
    puts("BEGIN");
    CHECK(xmlc_visit_source(source, size, SIZE_MAX, emit, NULL, &error) == XML_PARSE_OK);
    printf("END %zu\n", error.offset);
    free(source);
  }
  return 0;
}

int
main(int argc, char **argv) {
  if (argc == 2 && !strcmp(argv[1], "--oracle"))
    return oracle();
  CHECK(!fixtures());
  CHECK(!guarded_prefix());
  puts("source partitions, bounded reads and callback status: pass");
  return 0;
}
