/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <xml/call/name.h>
#include <xml/call/xml.h>
#include <stdio.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

typedef struct Fixture {
  const char *text;
  size_t      offsets[3];
  unsigned    accepted;
} Fixture;

static xml_parse_status_t
validate(unsigned api, const char *text, size_t size, xml_name_kind_t kind,
         xml_parse_error_t *error) {
  return api ? xmlc_validate_name_span(text, size, kind, error)
             : xml_validate_name_span(text, size, kind, error);
}

static int
fixtures(void) {
  const Fixture fixtures[] = {
    {"", {0, 0, 0}, 0}, {"svg", {0, 0, 0}, 7}, {"_", {0, 0, 0}, 7},
    {"xml", {0, 0, 0}, 7}, {"xmlns", {0, 0, 0}, 7},
    {"xml:lang", {0, 3, 0}, 5}, {"xmlns:svg", {0, 5, 0}, 5},
    {"undeclared:r", {0, 10, 0}, 5}, {"a-b.c_09", {0, 0, 0}, 7},
    {":", {0, 0, 0}, 1}, {":a", {0, 0, 0}, 1}, {"a:", {0, 1, 2}, 1},
    {"a:b:c", {0, 1, 3}, 1}, {"a::b", {0, 1, 2}, 1}, {"a:0", {0, 1, 2}, 1},
    {"1a", {0, 0, 0}, 0}, {"-a", {0, 0, 0}, 0}, {".a", {0, 0, 0}, 0},
    {" a", {0, 0, 0}, 0}, {"a ", {1, 1, 1}, 0}, {"a\tb", {1, 1, 1}, 0},
    {"a&#98;", {1, 1, 1}, 0}, {"a/b", {1, 1, 1}, 0},
    {"Türkçe", {0, 0, 0}, 7}, {"中:🙂", {0, 3, 0}, 5},
    {"\xc2\xb7", {0, 0, 0}, 0}, {"a\xc2\xb7", {0, 0, 0}, 7},
    {"\xcc\x80", {0, 0, 0}, 0}, {"a\xcc\x80", {0, 0, 0}, 7},
    {"\xcd\xbe", {0, 0, 0}, 0}, {"\xcd\xbf", {0, 0, 0}, 7},
    {"a\xe2\x80\xbf", {0, 0, 0}, 7}, {"\xe2\x80\xbf", {0, 0, 0}, 0},
    {"\xef\xbf\xbd", {0, 0, 0}, 7}, {"\xef\xbf\xbe", {0, 0, 0}, 0},
    {"\xf3\xaf\xbf\xbf", {0, 0, 0}, 7}, {"\xf3\xb0\x80\x80", {0, 0, 0}, 0},
    {"x\xc0\x80", {1, 1, 1}, 0}, {"x\xed\xa0\x80", {1, 1, 1}, 0},
    {"x\xf4\x90\x80\x80", {1, 1, 1}, 0}, {"x\xf0\x9f\x99", {1, 1, 1}, 0}
  };
  xml_parse_error_t  error;
  char             *input;
  size_t            i, size;
  unsigned          api, kind;
  bool              accepted;

  for (i = 0; i < XML_ARR_LEN(fixtures); i++) {
    size = strlen(fixtures[i].text);
    CHECK((input = malloc(size ? size : 1)));
    memcpy(input, fixtures[i].text, size);
    for (api = 0; api < 2; api++) {
      for (kind = 0; kind < 3; kind++) {
        accepted = (fixtures[i].accepted & (1u << kind)) != 0;
        CHECK(validate(api, input, size, (xml_name_kind_t)kind, &error)
              == (accepted ? XML_PARSE_OK : XML_PARSE_INVALID_DATA));
        CHECK(error.status == (accepted ? XML_PARSE_OK : XML_PARSE_INVALID_DATA));
        CHECK(error.offset == (accepted ? size : fixtures[i].offsets[kind]));
        CHECK(!memcmp(input, fixtures[i].text, size));
      }
    }
    free(input);
  }
  return 0;
}

static int
boundaries(void) {
  const char *unicode[] = {"é", "中", "🙂"};
  const size_t sizes[] = {65535, 65536, 1048576};
  xml_parse_error_t  error;
  char             *input;
  size_t            i, j, size;
  unsigned          api, kind;

  for (api = 0; api < 2; api++) {
    CHECK(validate(api, NULL, 1, XML_NAME, &error) == XML_PARSE_INVALID_ARGUMENT);
    CHECK(error.offset == 0 && error.status == XML_PARSE_INVALID_ARGUMENT);
    CHECK(validate(api, "x", SIZE_MAX, XML_NAME, &error) == XML_PARSE_INVALID_ARGUMENT);
    CHECK(validate(api, "x", 1, (xml_name_kind_t)-1, &error) == XML_PARSE_INVALID_ARGUMENT);
    CHECK(validate(api, "x", 1, (xml_name_kind_t)3, &error) == XML_PARSE_INVALID_ARGUMENT);
    for (kind = 0; kind < 3; kind++) {
      CHECK(validate(api, NULL, 0, (xml_name_kind_t)kind, &error) == XML_PARSE_INVALID_DATA);
      CHECK(error.offset == 0);
      CHECK(validate(api, "a\0b", 3, (xml_name_kind_t)kind, &error) == XML_PARSE_INVALID_DATA);
      CHECK(error.offset == 1);
      for (i = 0; i < XML_ARR_LEN(unicode); i++) {
        for (j = 1; j < strlen(unicode[i]); j++) {
          CHECK((input = malloc(j)));
          memcpy(input, unicode[i], j);
          CHECK(validate(api, input, j, (xml_name_kind_t)kind, &error) == XML_PARSE_INVALID_DATA);
          CHECK(error.offset == 0);
          free(input);
        }
      }
      for (i = 0; i < XML_ARR_LEN(sizes); i++) {
        size = sizes[i];
        CHECK((input = malloc(size)));
        memset(input, 'a', size);
        CHECK(validate(api, input, size, (xml_name_kind_t)kind, &error) == XML_PARSE_OK);
        CHECK(error.offset == size);
        input[size - 1] = ' ';
        CHECK(validate(api, input, size, (xml_name_kind_t)kind, &error) == XML_PARSE_INVALID_DATA);
        CHECK(error.offset == size - 1);
        free(input);
      }
    }
  }
  return 0;
}

static int
integration(void) {
  const char input[] = "<svg:r xmlns:svg='urn:svg' xml:lang='tr'><svg:🙂 data-x='1'/></svg:r>";
  char        source[sizeof(input)];
  xml_doc_t  *doc;
  xml_t      *node;
  xml_attr_t *attr;
  unsigned    mode, api, count;

  for (mode = 0; mode < 16; mode++) {
    memcpy(source, input, sizeof(input));
    CHECK((doc = xmlc_parse_span(source, sizeof(input) - 1, (xml_options_t)mode, NULL)));
    count = 0;
    for (node = doc->root; node; node = xml_xml(node)) {
      count++;
      for (api = 0; api < 2; api++) {
        CHECK(validate(api, node->tag, node->tagsize,
                       mode & XML_PREFIXES ? XML_NCNAME : XML_QNAME, NULL) == XML_PARSE_OK);
        if (node->prefix)
          CHECK(validate(api, node->prefix, node->prefixsize, XML_NCNAME, NULL) == XML_PARSE_OK);
        for (attr = node->attr; attr; attr = attr->next) {
          CHECK(validate(api, attr->name, attr->namesize, XML_QNAME, NULL) == XML_PARSE_OK);
        }
      }
    }
    CHECK(count == 2);
    xml_free(doc);
  }
  return 0;
}

static int
oracle(void) {
  char                line[8192], input[4096];
  xml_parse_error_t   error;
  size_t              size, i, length;
  xml_parse_status_t  status;
  unsigned            api, kind, byte;

  while (fgets(line, sizeof(line), stdin)) {
    CHECK(strlen(line) >= 4 && line[1] == ' ' && line[3] == ' ');
    CHECK(sscanf(line, "%u %u", &api, &kind) == 2 && api < 2 && kind < 3);
    length = strcspn(line + 4, "\r\n");
    CHECK(length % 2 == 0 && length / 2 <= sizeof(input));
    size = length / 2;
    for (i = 0; i < size; i++) {
      CHECK(sscanf(line + 4 + i * 2, "%2x", &byte) == 1);
      input[i] = (char)byte;
    }
    status = validate(api, input, size, (xml_name_kind_t)kind, &error);
    printf("%u %zu\n", (unsigned)status, error.offset);
  }
  CHECK(!ferror(stdin));
  return 0;
}

int
main(int argc, char **argv) {
  if (argc == 2 && !strcmp(argv[1], "--oracle"))
    return oracle();

  CHECK(!fixtures());
  CHECK(!boundaries());
  CHECK(!integration());
  puts("bounded XML Name/NCName/QName validation: pass");
  return 0;
}
