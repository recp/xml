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
    {"<?xml version='1.0'?><svg xmlns='http://www.w3.org/2000/svg'><text>A&amp;&#x1F642;</text></svg>", true},
    {"<r a='x' b=\"y\"> text &#9; <![CDATA[<&unknown;]]><!--x]]>y--><?p x]]>y?></r>", true},
    {"<r><ns:item ns:a='&amp; &lt; &gt; &quot; &apos; &#9; &#10; &#13;'/></r>", true},
    {"\xef\xbb\xbf<r>\r\n<e/> </r>", true},
    {"<\xcf\x80 \xc3\xa9='v'>\xf0\x9f\x99\x82</\xcf\x80>", true},
    {"<r a= 'x'/>", true},
    {"<1r/>", false},
    {"<r a='&unknown;'/>", false},
    {"<r a='&#xD800;'/>", false},
    {"<r>bad&#0;</r>", false},
    {"<r>literal]]></r>", false},
    {"<r a='<'/>", false},
    {"<r 'a'='1'/>", false},
    {"<r a=`x`/>", false},
    {"<r a=x/>", false},
    {"<r a='x'b='y'/>", false},
    {"<r a b='x'/>", false},
    {"< r/>", false},
    {"<r/ >", false},
    {"<r a='x' / >", false},
    {"<r\xc0\x80/>", false},
    {"<r>bad\x01</r>", false},
    {"<r><![CDATA[\x01]]></r>", false},
    {"<r><!--\xc0\x80--></r>", false},
    {"<r><?1p?></r>", false},
    {"<r><?p \x01?></r>", false},
    {"<r a='\xc0\x80'/>", false},
  };
  const xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_parse_error_t error;
  xml_doc_t        *doc;
  char             *input;
  size_t            i, size, prefix;
  unsigned          mode, bounded;

  for (mode = 0; mode < 16; mode++) {
    for (bounded = 0; bounded < 2; bounded++) {
      for (i = 0; i < XML_ARR_LEN(fixtures); i++) {
        size = strlen(fixtures[i].source);
        CHECK((input = malloc(size)));
        memcpy(input, fixtures[i].source, size);
        doc = xmlc_parse_span_checked(input, size, (xml_options_t)mode,
                                      bounded ? &limits : NULL, &error);
        if (!!doc != fixtures[i].valid)
          fprintf(stderr, "fixture %zu mode %u bounded %u status %u offset %zu\n",
                  i, mode, bounded, (unsigned)error.status, error.offset);
        CHECK(!!doc == fixtures[i].valid && error.offset <= size);
        CHECK(error.status == (fixtures[i].valid ? XML_PARSE_OK : XML_PARSE_INVALID_DATA));
        if (doc)
          CHECK(error.offset == size && doc->root);
        if (mode & XML_READONLY)
          CHECK(!memcmp(input, fixtures[i].source, size));
        xmlc_free(doc);
        free(input);
      }
    }
  }

  /* every source prefix is passed without a terminator or readable suffix. */
  for (i = 0; i < XML_ARR_LEN(fixtures); i++) {
    size = strlen(fixtures[i].source);
    for (prefix = 0; prefix <= size; prefix++) {
      CHECK((input = malloc(prefix ? prefix : 1)));
      memcpy(input, fixtures[i].source, prefix);
      doc = xmlc_parse_span_checked(input, prefix, XML_READONLY | XML_PREFIXES, &limits, &error);
      CHECK(error.offset <= prefix && !memcmp(input, fixtures[i].source, prefix));
      xmlc_free(doc);
      free(input);
    }
  }
  return 0;
}

static int
offsets(void) {
  const char *const sources[] = {
    "<1r/>", "<r a='&bad;'/>", "<r>ok\x01</r>", "<r a='v'b='x'/>",
    "< r/>", "<r/ >", "<r><?1p?></r>", "\xef\xbb\xbf<r a='&bad;'/>",
  };
  const size_t offsets[] = {1, 6, 5, 8, 1, 2, 5, 9};
  xml_parse_error_t error;
  xml_doc_t        *doc;
  size_t            i;

  for (i = 0; i < XML_ARR_LEN(sources); i++) {
    doc = xmlc_parse_span_checked(sources[i], strlen(sources[i]), XML_READONLY, NULL, &error);
    CHECK(!doc && error.status == XML_PARSE_INVALID_DATA && error.offset == offsets[i]);
  }
  return 0;
}

static int
names_and_arguments(void) {
  const char *const names[] = {"<:r/>", "<a:b:c/>", "<r a:b:c='v'/>"};
  xml_parse_error_t error;
  xml_doc_t        *doc;
  size_t            i;

  for (i = 0; i < XML_ARR_LEN(names); i++) {
    doc = xmlc_parse_span_checked(names[i], strlen(names[i]), XML_READONLY, NULL, &error);
    CHECK(doc && error.status == XML_PARSE_OK);
    xmlc_free(doc);
    doc = xmlc_parse_span_checked(names[i], strlen(names[i]), XML_READONLY | XML_PREFIXES, NULL, &error);
    CHECK(!doc && error.status == XML_PARSE_INVALID_DATA);
  }
  CHECK(!xmlc_parse_span_checked(NULL, 0, XML_READONLY, NULL, &error));
  CHECK(error.status == XML_PARSE_INVALID_ARGUMENT && error.offset == 0);
  CHECK(!xmlc_parse_span_checked("<r/>", 4, (xml_options_t)16, NULL, &error));
  CHECK(error.status == XML_PARSE_INVALID_ARGUMENT && error.offset == 0);
  return 0;
}

static int
limits(void) {
  const char         source[] = "<r a='v'><x/>text<![CDATA[v]]></r>";
  xml_parse_limits_t limits = {sizeof(source) - 1, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_parse_error_t  error;
  xml_doc_t         *doc;

  CHECK((doc = xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY, &limits, &error)));
  CHECK(xml_xml(doc->root) && xmls(doc->root));
  xmlc_free(doc);
  limits.max_input_bytes--;
  CHECK(!xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY, &limits, &error));
  CHECK(error.status == XML_PARSE_LIMIT && error.offset == 0);
  limits.max_input_bytes = SIZE_MAX;
  limits.max_depth       = 1;
  CHECK(!xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY, &limits, &error));
  CHECK(error.status == XML_PARSE_LIMIT);
  limits.max_depth  = SIZE_MAX;
  limits.max_tokens = 1;
  CHECK(!xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY, &limits, &error));
  CHECK(error.status == XML_PARSE_LIMIT);
  limits.max_tokens       = SIZE_MAX;
  limits.max_memory_bytes = 0;
  CHECK(!xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY, &limits, &error));
  CHECK(error.status == XML_PARSE_LIMIT && error.offset == 0);
  return 0;
}

int
main(void) {
  CHECK(!fixtures());
  CHECK(!offsets());
  CHECK(!names_and_arguments());
  CHECK(!limits());
  puts("checked names, values, delimiters, source bounds and resource caps: pass");
  return 0;
}
