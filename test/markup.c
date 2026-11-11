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

#define CHECK(test) do { \
  if (!(test)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #test); \
    return 1; \
  } \
} while (0)

typedef struct Capture {
  xml_markup_span_t spans[8];
  size_t            count, failAt;
  xml_parse_status_t failure;
} Capture;

typedef struct Fixture {
  const char       *source;
  xml_markup_span_t spans[8];
  size_t            count;
  bool              generic;
} Fixture;

static xml_parse_status_t
capture(void *context, const xml_markup_span_t *span) {
  Capture *result = context;

  if (result->count == XML_ARR_LEN(result->spans))
    return XML_PARSE_LIMIT;
  result->spans[result->count++] = *span;
  if (result->failAt && result->count == result->failAt)
    return result->failure;
  return XML_PARSE_OK;
}

static int
fixtures(void) {
  const Fixture fixtures[] = {
    {"<r/>",
     {{0}}, 0, false},
    {"<!--a--><r/><!--b-->",
     {{0, 8, XML_MARKUP_COMMENT}, {12, 8, XML_MARKUP_COMMENT}}, 2, false},
    {"<?pi?><r/><?q a:b?>",
     {{0, 6, XML_MARKUP_PI}, {10, 9, XML_MARKUP_PI}}, 2, false},
    {"<?xml version='1.0'?><r/>",
     {{0, 21, XML_MARKUP_DECLARATION}}, 1, false},
    {"\357\273\277<?xml \tversion=\"1.0\"\r\n encoding='UTF-8' standalone='yes' ?><r/>\r\n<!--tail-->",
     {{3, 59, XML_MARKUP_DECLARATION}, {68, 11, XML_MARKUP_COMMENT}}, 2, false},
    {"<r a='&lt;?p?>' b='&lt;!--x-->'>&lt;?p?><![CDATA[<?p?><!--fake-->]]><!--real--><?p?></r>",
     {{68, 11, XML_MARKUP_COMMENT}, {79, 5, XML_MARKUP_PI}}, 2, false},
    {"<r><!-- <?nested?> <![CDATA[x]]> --></r>",
     {{3, 33, XML_MARKUP_COMMENT}}, 1, false},
    {"<r><?p <!--fake--> <![CDATA[x]]> ?></r>",
     {{3, 32, XML_MARKUP_PI}}, 1, false},
    {"<?xml-stylesheet href='x.css'?><r/>",
     {{0, 31, XML_MARKUP_PI}}, 1, false},
    {"<?xmlish?><r/>",
     {{0, 10, XML_MARKUP_PI}}, 1, false},
    {"<r><!----><?p?><![CDATA[]]><![CDATA[a]]]></r>",
     {{3, 7, XML_MARKUP_COMMENT}, {10, 5, XML_MARKUP_PI}}, 2, false},
    {"<r><!--\r\nraw &amp; \317\200--><?\317\200 raw &amp;\r\n?></r>",
     {{3, 21, XML_MARKUP_COMMENT}, {24, 18, XML_MARKUP_PI}}, 2, false},
    {"<r><x><!--inner--></x><?p?></r>",
     {{6, 12, XML_MARKUP_COMMENT}, {22, 5, XML_MARKUP_PI}}, 2, false},
    {" \r\n<!--before--><?p?><r/> \t<?q?><!--after-->",
     {{3, 13, XML_MARKUP_COMMENT}, {16, 5, XML_MARKUP_PI}, {27, 5, XML_MARKUP_PI}, {32, 12, XML_MARKUP_COMMENT}}, 4, false},
    {"<?xml:note?><r/>",
     {{0, 12, XML_MARKUP_PI}}, 1, true},
  };
  Capture           result;
  xml_parse_error_t error;
  xml_doc_t        *doc;
  char             *input;
  size_t            i, j, size, cap, count;
  unsigned          mode;

  for (mode = XML_READONLY; mode < XML_PRESERVE_TEXT; mode++) {
    for (i = 0; i < XML_ARR_LEN(fixtures); i++) {
      if (fixtures[i].generic && (mode & XML_PREFIXES))
        continue;
      size = strlen(fixtures[i].source);
      CHECK((input = malloc(size)));
      memcpy(input, fixtures[i].source, size);
      CHECK((doc = xmlc_parse_span_checked(input, size, (xml_options_t)mode, NULL, NULL)));
      xmlc_free(doc);
      for (cap = 0; cap <= size + 1; cap++) {
        memset(&result, 0, sizeof(result));
        CHECK(xmlc_visit_markup(input, size, cap, capture, &result, &error)
              == (cap < size ? XML_PARSE_LIMIT : XML_PARSE_OK));
        CHECK(error.offset == (cap < size ? cap : size));
        count = 0;
        for (j = 0; j < fixtures[i].count; j++) {
          if (fixtures[i].spans[j].offset + fixtures[i].spans[j].size > cap)
            break;
          CHECK(result.spans[j].offset == fixtures[i].spans[j].offset);
          CHECK(result.spans[j].size == fixtures[i].spans[j].size);
          CHECK(result.spans[j].kind == fixtures[i].spans[j].kind);
          count++;
        }
        CHECK(result.count == count && !memcmp(input, fixtures[i].source, size));
      }
      free(input);
    }
  }
  return 0;
}

static int
callback_failures(void) {
  const char        source[] = "<!--a--><r><?p?></r><!--b-->";
  Capture           result;
  xml_parse_error_t error;
  xml_doc_t        *doc;
  unsigned          status;

  CHECK((doc = xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY, NULL, NULL)));
  xmlc_free(doc);
  for (status = XML_PARSE_INVALID_ARGUMENT; status <= XML_PARSE_LIMIT; status++) {
    memset(&result, 0, sizeof(result));
    result.failAt  = 2;
    result.failure = (xml_parse_status_t)status;
    CHECK(xmlc_visit_markup(source, sizeof(source) - 1, SIZE_MAX, capture, &result, &error)
          == result.failure);
    CHECK(result.count == 2 && error.status == result.failure && error.offset == 11);
  }
  return 0;
}

static int
long_comment(void) {
  const char        before[] = "<r><!--", after[] = "--></r>";
  Capture           result = {0};
  xml_doc_t        *doc;
  char             *source;
  size_t            length = 1048576, size;

  size = length + sizeof(before) + sizeof(after) - 2;
  CHECK((source = malloc(size)));
  memcpy(source, before, sizeof(before) - 1);
  memset(source + sizeof(before) - 1, 'x', length);
  memcpy(source + sizeof(before) - 1 + length, after, sizeof(after) - 1);
  CHECK((doc = xmlc_parse_span_checked(source, size, XML_READONLY, NULL, NULL)));
  xmlc_free(doc);
  CHECK(xmlc_visit_markup(source, size, SIZE_MAX, capture, &result, NULL) == XML_PARSE_OK);
  CHECK(result.count == 1 && result.spans[0].offset == 3 && result.spans[0].size == length + 7);
  memset(&result, 0, sizeof(result));
  CHECK(xmlc_visit_markup(source, size, size / 2, capture, &result, NULL) == XML_PARSE_LIMIT);
  CHECK(!result.count);
  free(source);
  return 0;
}

static int
edit(void) {
  const char        source[] = "<!--before--><r fill='red'><!--middle--><?pi?></r><!--after-->";
  Capture           before = {0}, after = {0};
  char              edited[sizeof(source) + 1];
  xml_doc_t        *doc;
  const xml_attr_t *fill;
  size_t            i, prefix, size = sizeof(source) - 1;

  CHECK((doc = xmlc_parse_span_checked(source, size, XML_READONLY | XML_PREFIXES, NULL, NULL)));
  CHECK((fill = xmla(doc->root, "fill")));
  CHECK(fill->valsize == 3);
  prefix = (size_t)(fill->val - source);
  memcpy(edited, source, prefix);
  memcpy(edited + prefix, "blue", 4);
  memcpy(edited + prefix + 4, fill->val + 3, size - prefix - 3);
  xmlc_free(doc);
  CHECK((doc = xmlc_parse_span_checked(edited, size + 1, XML_READONLY | XML_PREFIXES, NULL, NULL)));
  xmlc_free(doc);
  CHECK(xmlc_visit_markup(source, size, SIZE_MAX, capture, &before, NULL) == XML_PARSE_OK);
  CHECK(xmlc_visit_markup(edited, size + 1, SIZE_MAX, capture, &after, NULL) == XML_PARSE_OK);
  CHECK(before.count == 4 && after.count == before.count);
  for (i = 0; i < before.count; i++) {
    CHECK(before.spans[i].kind == after.spans[i].kind && before.spans[i].size == after.spans[i].size);
    CHECK(after.spans[i].offset == before.spans[i].offset + (before.spans[i].offset > prefix));
    CHECK(!memcmp(source + before.spans[i].offset, edited + after.spans[i].offset, before.spans[i].size));
  }
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
  memcpy(source, "<r><!--", 7);
  memset(source + 7, 'x', size - 14);
  memcpy(source + size - 7, "--></r>", 7);
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
  partial = xmlc_visit_markup(source, size, page, capture, &result, &error);
  if (mprotect(source, size, PROT_NONE)) {
    munmap(source, size);
    return 1;
  }
  zero = xmlc_visit_markup(source, size, 0, capture, &result, &error);
  munmap(source, size);
  CHECK(partial == XML_PARSE_LIMIT && zero == XML_PARSE_LIMIT && !error.offset && !result.count);
#else
  puts("guard-page prefix check unavailable on this platform");
#endif
  return 0;
}

static int
arguments(void) {
  const char *const malformed[] = {"<!--", "<?pi", "<![CDATA[", "<![bad]]>", "<!DOCTYPE r><r/>"};
  Capture           result = {0};
  xml_parse_error_t error;
  size_t            i;

  CHECK(xmlc_visit_markup(NULL, 0, 0, capture, &result, &error) == XML_PARSE_OK && !error.offset);
  CHECK(xmlc_visit_markup(NULL, 1, SIZE_MAX, capture, &result, &error) == XML_PARSE_INVALID_ARGUMENT);
  CHECK(xmlc_visit_markup("<r/>", 4, SIZE_MAX, NULL, NULL, &error) == XML_PARSE_INVALID_ARGUMENT);
  CHECK(xmlc_visit_markup("<r/>", (size_t)PTRDIFF_MAX + 1, 0, capture, &result, &error)
        == XML_PARSE_INVALID_ARGUMENT && error.offset == 0);
  for (i = 0; i < XML_ARR_LEN(malformed); i++) {
    CHECK(xmlc_visit_markup(malformed[i], strlen(malformed[i]), SIZE_MAX, capture, &result, &error)
          == XML_PARSE_INVALID_DATA);
  }
  CHECK(!result.count);
  return 0;
}

static xml_parse_status_t
emit(void *context, const xml_markup_span_t *span) {
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
    CHECK(xmlc_visit_markup(source, size, SIZE_MAX, emit, NULL, &error) == XML_PARSE_OK);
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
  CHECK(!callback_failures());
  CHECK(!long_comment());
  CHECK(!edit());
  CHECK(!guarded_prefix());
  CHECK(!arguments());
  puts("raw markup spans, byte budgets, callbacks and host edits: pass");
  return 0;
}
