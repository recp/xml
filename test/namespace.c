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
  bool        valid;
} Fixture;

static int
fixtures(void) {
  const Fixture fixtures[] = {
    {"<svg/>", true},
    {"<svg xmlns='http://www.w3.org/2000/svg'><g fill='red'/></svg>", true},
    {"<s:svg xmlns:s='http://www.w3.org/2000/svg'><s:g/></s:svg>", true},
    {"<r xmlns:p='u'><p:n p:a='v'/></r>", true},
    {"<p:r p:a='v' xmlns:p='u'/>", true},
    {"<p:r xmlns:p='u'><p:n xmlns:p='v'><p:z/></p:n><p:n/></p:r>", true},
    {"<r xmlns='u'><n xmlns=''><x/></n><n/></r>", true},
    {"<xml:r xml:lang='en'/>", true},
    {"<r xmlns:xml='http://www.w3.org/XML/1998/namespace' xml:space='preserve'/>", true},
    {"<r xmlns:xml='http://www.w3.org/XML/1998/namesp&#97;ce'><xml:n/></r>", true},
    {"<r xmlns:p='u&amp;v'><p:n/></r>", true},
    {"<r xmlns:XML='u'><XML:n XML:a='v'/></r>", true},
    {"<r xmlns:xmlStuff='u'><xmlStuff:n/></r>", true},
    {"<xmlns xmlns='u'/>", true},
    {"<r xmlns:p='u' xmlns:q='u' p:a='1' q:b='2'/>", true},
    {"<r xmlns:p='u' p:a='v' a='w'/>", true},
    {"<r xmlns:p='u'><p:n/>text<![CDATA[x]]><!--c--><?pi x?></r>", true},
    {"<\xcf\x80:r xmlns:\xcf\x80='u'><\xcf\x80:n/></\xcf\x80:r>", true},
    {"<r xmlns:p='http://www.w3.org/XML/1998/%6Eamespace'><p:n/></r>", true},
    {"<r xmlns:p='http://www.w3.org/xml/1998/namespace'><p:n/></r>", true},
    {"<r xmlns:p='urn:u\r\nx'><p:n/></r>", true},
    {"<p:r/>", false},
    {"<r p:a='v'/>", false},
    {"<r><x xmlns:p='u'/><p:y/></r>", false},
    {"<r><p:x xmlns:p='u'/><p:y/></r>", false},
    {"<r xmlns:p='u'><p:n/><q:x/></r>", false},
    {"<p:r xmlns:p='u'><p:n xmlns:p=''/></p:r>", false},
    {"<r xmlns:p='u'><n xmlns:p=''><p:x/></n></r>", false},
    {"<r xmlns:xml='u'/>", false},
    {"<r xmlns:xml=''/>", false},
    {"<r xmlns:p='http://www.w3.org/XML/1998/namespace'/>", false},
    {"<r xmlns='http://www.w3.org/XML/1998/namespace'/>", false},
    {"<r xmlns:p='http://www.w3.org/2000/xmlns/'/>", false},
    {"<r xmlns='http://www.w3.org/2000/xmlns/'/>", false},
    {"<r xmlns:xmlns='u'/>", false},
    {"<r xmlns:xmlns='http://www.w3.org/2000/xmlns/'/>", false},
    {"<xmlns:r/>", false},
    {"<r xmlns:p=''/>", false},
    {"<r xmlns:xml='http://www.w3.org/XML/1998/namespacE'/>", false},
    {"<r xmlns:p='http://www.w3.org/XML/1998/namesp&#97;ce'/>", false},
    {"<XML:r/>", false},
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
      memcpy(copy, input, size);
      status = xmlc_validate_namespace_bindings(doc, SIZE_MAX, &invalid);
      if ((status == XML_PARSE_OK) != fixtures[i].valid)
        fprintf(stderr, "fixture %zu mode %u status %u\n", i, mode, (unsigned)status);
      CHECK(status == (fixtures[i].valid ? XML_PARSE_OK : XML_PARSE_INVALID_DATA));
      CHECK(fixtures[i].valid ? !invalid : invalid >= input && invalid < input + size);
      CHECK(!memcmp(copy, input, size));
      xmlc_free(doc);
      free(copy);
      free(input);
    }
  }
  return 0;
}

static int
limits(void) {
  const char *const sources[] = {
    "<r/>", "<p:r xmlns:p='u'><p:n p:a='v'/><p:n/></p:r>",
    "<r xmlns:p='u'><n xmlns:p='v'><p:x/></n><p:y/></r>",
  };
  xml_doc_t         *doc;
  const char        *invalid;
  char              *input;
  size_t             i, size, cap;
  xml_parse_status_t status;
  unsigned           mode;

  for (mode = 0; mode < 16; mode++) {
    for (i = 0; i < XML_ARR_LEN(sources); i++) {
      size = strlen(sources[i]);
      CHECK((input = malloc(size)));
      memcpy(input, sources[i], size);
      CHECK((doc = xmlc_parse_span_checked(input, size, (xml_options_t)(mode | XML_PREFIXES), NULL, NULL)));
      for (cap = 0; cap < 4096; cap++) {
        status = xmlc_validate_namespace_bindings(doc, cap, &invalid);
        if (status == XML_PARSE_OK)
          break;
        CHECK(status == XML_PARSE_LIMIT && invalid >= input && invalid < input + size);
      }
      CHECK(cap > 0 && cap < 4096 && !invalid);
      CHECK(xmlc_validate_namespace_bindings(doc, cap - 1, NULL) == XML_PARSE_LIMIT);
      CHECK(xmlc_validate_namespace_bindings(doc, cap + 1, NULL) == XML_PARSE_OK);
      if (!i)
        CHECK(cap == 2);
      xmlc_free(doc);
      free(input);
    }
  }
  return 0;
}

static int
offsets(void) {
  const char *const sources[] = {
    "<p:r/>", "<r p:a='v'/>", "<r><x xmlns:p='u'/><p:y/></r>",
    "<r xmlns:xml='u'/>", "<r xmlns:p=''/>", "<r xmlns:xmlns='u'/>", "<xmlns:r/>",
  };
  const char *const markers[] = {"p:r", "p:a", "p:y", "u", "'/>", "xmlns='u'", "xmlns:r"};
  xml_doc_t  *doc;
  const char *invalid;
  char       *input;
  size_t      i, size;
  unsigned    mode;

  for (mode = 0; mode < 16; mode++) {
    for (i = 0; i < XML_ARR_LEN(sources); i++) {
      size = strlen(sources[i]);
      CHECK((input = malloc(size)));
      memcpy(input, sources[i], size);
      CHECK((doc = xmlc_parse_span_checked(input, size, (xml_options_t)(mode | XML_PREFIXES), NULL, NULL)));
      CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, &invalid) == XML_PARSE_INVALID_DATA);
      CHECK(invalid == input + (strstr(sources[i], markers[i]) - sources[i]));
      xmlc_free(doc);
      free(input);
    }
  }
  return 0;
}

static int
long_values(void) {
  const char prefix[] = "<xml:r xmlns:xml='http://www.w3.org/XML/1998/namesp&#";
  const char suffix[] = "97;ce'/>";
  xml_doc_t  *doc;
  const char *invalid;
  char       *input;
  size_t      zeros = 65536, size;

  size = sizeof(prefix) - 1 + zeros + sizeof(suffix) - 1;
  CHECK((input = malloc(size)));
  memcpy(input, prefix, sizeof(prefix) - 1);
  memset(input + sizeof(prefix) - 1, '0', zeros);
  memcpy(input + sizeof(prefix) - 1 + zeros, suffix, sizeof(suffix) - 1);
  CHECK((doc = xmlc_parse_span_checked(input, size, XML_READONLY | XML_PREFIXES, NULL, NULL)));
  CHECK(xmlc_validate_namespace_bindings(doc, zeros, &invalid) == XML_PARSE_LIMIT);
  CHECK(invalid && invalid >= input && invalid < input + size);
  CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, &invalid) == XML_PARSE_OK && !invalid);
  xmlc_free(doc);
  free(input);
  return 0;
}

static int
arguments_and_scope(void) {
  const char *const sources[] = {
    "<r xmlns:p='u' xmlns:q='u' p:a='1' q:a='2'/>", "<?pi?><r/>",
  };
  xml_doc_t  *doc;
  const char *invalid = sources[0];
  size_t      i;

  CHECK(xmlc_validate_namespace_bindings(NULL, SIZE_MAX, &invalid) == XML_PARSE_INVALID_ARGUMENT);
  CHECK(!invalid);
  CHECK((doc = xmlc_parse_span_checked("<r/>", 4, XML_READONLY, NULL, NULL)));
  CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, &invalid) == XML_PARSE_INVALID_ARGUMENT);
  CHECK(!invalid);
  xmlc_free(doc);

  /* binding validation does not claim expanded uniqueness. */
  for (i = 0; i < XML_ARR_LEN(sources); i++) {
    CHECK((doc = xmlc_parse_span_checked(sources[i], strlen(sources[i]), XML_READONLY | XML_PREFIXES, NULL, NULL)));
    CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, &invalid) == XML_PARSE_OK && !invalid);
    xmlc_free(doc);
  }
  return 0;
}

int
main(void) {
  CHECK(!fixtures());
  CHECK(!limits());
  CHECK(!offsets());
  CHECK(!long_values());
  CHECK(!arguments_and_scope());
  puts("namespace bindings, inherited scope, reserved URIs, work limits and unchanged bytes: pass");
  return 0;
}
