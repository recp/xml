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

static int
fixtures(void) {
  const char *const invalid[] = {
    "<>", "<><r/>", "<r/><>", "<><r/><>", "<r><></r>", "<r>\n<>\n</r>",
    "<r><x/><></r>", "<r><>/><y/></r>", "<r><>x/><y/></r>",
    "<r><x/><>/></r>", "<r><x/><>y/></r>",
    "<r> a <>/> b </r>", "<r> a <>x/> b </r>",
    "<r><>!--a-b--></r>", "<r><>--a-b--></r>",
    "<r><>?pi a:b?></r>", "<r><>pi a:b?></r>",
    "<r xmlns='urn:u' xmlns:p='urn:p'><>:n p:a='v'/></r>",
    "<r xmlns='urn:u' xmlns:p='urn:p'><>p:n p:a='v'/></r>",
  };
  const char *const valid[] = {
    "<r/>", "<r></r>", "<r>></r>", "<r>&lt;></r>", "<r a='&lt;>'/>",
    "<r><![CDATA[<>]]></r>", "<!--<>--><r/>", "<r/><!--<>-->",
    "<r><?pi <> ?></r>", "<?pi <> ?><r/>", "<r><x/></r>", "<r></r >",
  };
  const xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_parse_error_t error;
  xml_doc_t        *doc;
  char             *source;
  size_t            i, size, prefix, offset;
  unsigned          mode, bounded;

  for (mode = 0; mode < 16; mode++) {
    for (bounded = 0; bounded < 2; bounded++) {
      for (i = 0; i < XML_ARR_LEN(invalid); i++) {
        size   = strlen(invalid[i]);
        offset = (size_t)(strstr(invalid[i], "<>") - invalid[i]) + 1;
        CHECK((source = malloc(size)));
        memcpy(source, invalid[i], size);
        CHECK(!xmlc_parse_span_checked(source, size, (xml_options_t)mode, bounded ? &limits : NULL, &error));
        CHECK(error.status == XML_PARSE_INVALID_DATA && error.offset == offset);
        if (mode & XML_READONLY)
          CHECK(!memcmp(source, invalid[i], size));
        free(source);
      }
      for (i = 0; i < XML_ARR_LEN(valid); i++) {
        size = strlen(valid[i]);
        CHECK((source = malloc(size)));
        memcpy(source, valid[i], size);
        CHECK((doc = xmlc_parse_span_checked(source, size, (xml_options_t)mode, bounded ? &limits : NULL, &error)));
        CHECK(error.status == XML_PARSE_OK && error.offset == size && doc->root);
        if (mode & XML_READONLY)
          CHECK(!memcmp(source, valid[i], size));
        xmlc_free(doc);
        free(source);
      }
    }
  }

  /* a prefix can be complete before later bad markup; still enforce its bound. */
  for (i = 0; i < XML_ARR_LEN(invalid); i++) {
    size = strlen(invalid[i]);
    for (prefix = 0; prefix <= size; prefix++) {
      CHECK((source = malloc(prefix ? prefix : 1)));
      memcpy(source, invalid[i], prefix);
      doc = xmlc_parse_span_checked(source, prefix, XML_READONLY | XML_PREFIXES, &limits, &error);
      CHECK(error.offset <= prefix && !memcmp(source, invalid[i], prefix));
      xmlc_free(doc);
      free(source);
    }
  }
  return 0;
}

static int
limits(void) {
  const char         source[] = "<r><></r>";
  xml_parse_limits_t limits = {sizeof(source) - 2, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_parse_error_t  error;

  CHECK(!xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY, &limits, &error));
  CHECK(error.status == XML_PARSE_LIMIT && error.offset == 0);
  limits.max_input_bytes  = SIZE_MAX;
  limits.max_memory_bytes = 0;
  CHECK(!xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY, &limits, &error));
  CHECK(error.status == XML_PARSE_LIMIT && error.offset == 0);
  limits.max_memory_bytes = SIZE_MAX;
  CHECK(!xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY, &limits, &error));
  CHECK(error.status == XML_PARSE_INVALID_DATA && error.offset == 4);
  return 0;
}

static int
oracle(void) {
  const xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  char             input[32769];
  xml_parse_error_t error;
  xml_doc_t        *doc;
  char            *source;
  size_t           size, i;
  unsigned         mode, bounded, byte;

  while (scanf("%u %u %32768s", &mode, &bounded, input) == 3) {
    size = input[0] == '-' ? 0 : strlen(input) / 2;
    CHECK((source = malloc(size ? size : 1)));
    for (i = 0; i < size; i++) {
      CHECK(sscanf(input + i * 2, "%2x", &byte) == 1);
      source[i] = (char)byte;
    }
    doc = xmlc_parse_span_checked(source, size, (xml_options_t)mode, bounded ? &limits : NULL, &error);
    printf("%u %zu\n", (unsigned)error.status, error.offset);
    xmlc_free(doc);
    free(source);
  }
  return 0;
}

int
main(int argc, char **argv) {
  if (argc == 2 && !strcmp(argv[1], "--oracle"))
    return oracle();
  CHECK(!fixtures());
  CHECK(!limits());
  puts("empty tag rejection, markup contexts, exact offsets and resource precedence: pass");
  return 0;
}
