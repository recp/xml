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
    {"<?xml version=\"1.0\"?><r/>", true},
    {"<?xml \tversion \r\n= '1.0' \t?><r/>", true},
    {"<?xml version='1.0' encoding='UTF-8'?><r/>", true},
    {"<?xml version='1.0' encoding=\"uTf-8\" standalone='yes' ?><r/>", true},
    {"<?xml version='1.0' standalone='no'?><r/>", true},
    {"<?xml version='1.0' standalone=\"yes\"?><r/>", true},
    {"<?xml version='1.1'?><r/>", true},
    {"<?xml version='1.00'?><r/>", true},
    {"<?xml version='1.123'?><r/>", true},
    {"\xef\xbb\xbf<?xml version='1.0' encoding='UTF-8'?><r/>", true},
    {"<?p?><r><?p x?y &unknown; <z?>text</r><?p ?>", true},
    {"<?xml-stylesheet type='text/css' href='x.css'?><r/>", true},
    {"<r><?XML-stylesheet?></r>", true},
    {"<?\xcf\x80 data?><r/>", true},
    {"<?xml?><r/>", false},
    {"<?xml ?><r/>", false},
    {"<?XML version='1.0'?><r/>", false},
    {"<?xMl version='1.0'?><r/>", false},
    {"<?xml version='1.0'?>", false},
    {"<?p?>", false},
    {" <?xml version='1.0'?><r/>", false},
    {"\xef\xbb\xbf <?xml version='1.0'?><r/>", false},
    {"<!--c--><?xml version='1.0'?><r/>", false},
    {"<?p?><?xml version='1.0'?><r/>", false},
    {"<?xml version='1.0'?><?xml version='1.0'?><r/>", false},
    {"<r><?xml version='1.0'?></r>", false},
    {"<r/><?xml version='1.0'?>", false},
    {"<?xml encoding='UTF-8'?><r/>", false},
    {"<?xml standalone='yes'?><r/>", false},
    {"<?xml Version='1.0'?><r/>", false},
    {"<?xml version '1.0'?><r/>", false},
    {"<?xml version=1.0?><r/>", false},
    {"<?xml version='1.0\"?><r/>", false},
    {"<?xml version=''?><r/>", false},
    {"<?xml version='2.0'?><r/>", false},
    {"<?xml version='1.'?><r/>", false},
    {"<?xml version='1.x'?><r/>", false},
    {"<?xml version='1.0 '?><r/>", false},
    {"<?xml version='&#49;.0'?><r/>", false},
    {"<?xml version='1.0'encoding='UTF-8'?><r/>", false},
    {"<?xml version='1.0' version='1.0'?><r/>", false},
    {"<?xml version='1.0' standalone='yes' encoding='UTF-8'?><r/>", false},
    {"<?xml version='1.0' encoding='UTF-8' encoding='UTF-8'?><r/>", false},
    {"<?xml version='1.0' encoding=UTF-8?><r/>", false},
    {"<?xml version='1.0' standalone='Yes'?><r/>", false},
    {"<?xml version='1.0' standalone='true'?><r/>", false},
    {"<?xml version='1.0' standalone=''?><r/>", false},
    {"<?xml version='1.0' standalone='yes' standalone='no'?><r/>", false},
    {"<?xml version='1.0' extra='x'?><r/>", false},
    {"<?xml version='1.0'/><r/>", false},
    {"<?p?data?><r/>", false},
    {"<r a='x' b=\"y\"> text &#9; <![CDATA[<&unknown;]]><!--x]]>y--><?p x]]>y?></r>", true},
    {"<r><ns:item ns:a='&amp; &lt; &gt; &quot; &apos; &#9; &#10; &#13;'/></r>", true},
    {"\xef\xbb\xbf<r>\r\n<e/> </r>", true},
    {"<\xcf\x80 \xc3\xa9='v'>\xf0\x9f\x99\x82</\xcf\x80>", true},
    {"<r a= 'x'/>", true},
    {"<r a='1' A='2'><n a='3'/><n a='4'/></r>", true},
    {"<r a='1' a='2'/>", false},
    {"<r a='1' a='2'></r>", false},
    {"<r xmlns='x' xmlns='y'/>", false},
    {"<r p:a='1' p:a='2'/>", false},
    {"<r a='1' b='2' a='3' b='4'/>", false},
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
encoding_limits(void) {
  const char *const encodings[] = {"UTF-16", "US-ASCII", "ISO-8859-1", "UTF8", "", "UTF-8 "};
  char              source[128];
  xml_parse_error_t  error;
  xml_doc_t         *doc;
  size_t            i, size;
  unsigned          mode;

  for (mode = 0; mode < 16; mode++) {
    for (i = 0; i < XML_ARR_LEN(encodings); i++) {
      size = (size_t)snprintf(source, sizeof(source), "<?xml version='1.0' encoding='%s'?><r/>", encodings[i]);
      doc = xmlc_parse_span_checked(source, size, (xml_options_t)mode, NULL, &error);
      CHECK(!doc && error.status == XML_PARSE_INVALID_DATA && error.offset == 30);
    }
  }
  return 0;
}

static int
attribute_lists(void) {
  const size_t counts[] = {0, 1, 2, 8, 9, 32, 33, 128, 2046};
  char             source[65536], copy[65536];
  size_t           starts[2046];
  xml_parse_error_t error;
  xml_doc_t        *doc;
  const xml_attr_t *attribute;
  size_t           c, count, i, size, name, repeated;
  unsigned         mode, duplicate;

  for (c = 0; c < XML_ARR_LEN(counts); c++) {
    count = counts[c];
    for (duplicate = 0; duplicate < 4; duplicate++) {
      if (duplicate && count < 2)
        continue;

      size     = 2;
      repeated = SIZE_MAX;
      memcpy(source, "<r", 2);
      for (i = 0; i < count; i++) {
        name = i;
        if ((duplicate == 1 && i == count - 1)
            || (duplicate >= 2 && i == count / 2)
            || (duplicate == 3 && i == count - 1)) {
          name = 0;
          if (repeated == SIZE_MAX)
            repeated = size + 1;
        }
        starts[i] = size + 1;
        size += (size_t)snprintf(source + size, sizeof(source) - size, " p:a%04zu='v'", name);
      }
      if (c & 1) {
        memcpy(source + size, ">text</r>", 9);
        size += 9;
      } else {
        memcpy(source + size, "/>", 2);
        size += 2;
      }

      for (mode = 0; mode < 16; mode++) {
        memcpy(copy, source, size);
        doc = xmlc_parse_span_checked(copy, size, (xml_options_t)mode, NULL, &error);
        CHECK(!!doc == !duplicate && error.offset <= size);
        if (duplicate) {
          CHECK(error.status == XML_PARSE_INVALID_DATA && error.offset == repeated);
        } else {
          CHECK(error.status == XML_PARSE_OK);
          attribute = doc->root->attr;
          for (i = count; i > 0; i--) {
            CHECK(attribute && attribute->name == copy + starts[i - 1]);
            attribute = attribute->next;
          }
          CHECK(!attribute);
        }
        if (mode & XML_READONLY)
          CHECK(!memcmp(copy, source, size));
        xmlc_free(doc);
      }
    }
  }
  return 0;
}

static int
scratch_limits(void) {
  char               source[16384];
  xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_parse_error_t  error;
  xml_doc_t         *doc;
  size_t            i, size = 2, owned;

  memcpy(source, "<r", 2);
  for (i = 0; i < 33; i++)
    size += (size_t)snprintf(source + size, sizeof(source) - size, " a%zu='v'", i);

  memcpy(source + size, "/>", 2);
  size += 2;
  owned = sizeof(xml_doc_t) + sizeof(xml_mem_t) + XML_MEM_PAGE;
  limits.max_memory_bytes = owned + 33 * sizeof(xml_attr_t *) - 1;
  CHECK(!xmlc_parse_span_checked(source, size, XML_READONLY, &limits, &error));
  CHECK(error.status == XML_PARSE_LIMIT);
  limits.max_memory_bytes++;
  CHECK((doc = xmlc_parse_span_checked(source, size, XML_READONLY, &limits, &error)));
  xmlc_free(doc);

  /* scratch is released before later content grows the retained arena. */
  size -= 2;
  source[size++] = '>';
  for (i = 0; i < 2000; i++) {
    memcpy(source + size, "<x/>", 4);
    size += 4;
  }
  memcpy(source + size, "</r>", 4);
  size += 4;
  limits.max_memory_bytes = owned + 65536;
  CHECK((doc = xmlc_parse_span_checked(source, size, XML_READONLY, &limits, &error)));
  xmlc_free(doc);
  return 0;
}

static int
offsets(void) {
  const char *const sources[] = {
    "<1r/>", "<r a='&bad;'/>", "<r>ok\x01</r>", "<r a='v'b='x'/>",
    "< r/>", "<r/ >", "<r><?1p?></r>", "\xef\xbb\xbf<r a='&bad;'/>",
    "<?XML version='1.0'?><r/>", "<?xml?><r/>",
    " <?xml version='1.0'?><r/>", "<r><?xml version='1.0'?></r>",
    "\xef\xbb\xbf <?xml version='1.0'?><r/>",
    "<?xml version='2.0'?><r/>", "<?xml version='1.x'?><r/>",
  };
  const size_t offsets[] = {1, 6, 5, 8, 1, 2, 5, 9, 2, 5, 3, 5, 6, 15, 17};
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
  CHECK(!encoding_limits());
  CHECK(!attribute_lists());
  CHECK(!scratch_limits());
  CHECK(!offsets());
  CHECK(!names_and_arguments());
  CHECK(!limits());
  puts("checked names, values, delimiters, source bounds and resource caps: pass");
  return 0;
}
