/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <xml/call/xml.h>
#include <xml/call/decode.h>
#include <xml/call/name.h>
#include <xml/print.h>
#include <stdio.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

extern "C" {
void
xml_test_layout(size_t *layout);

xml_doc_t*
xml_test_parse(char *source, size_t size, xml_options_t options, xml_parse_error_t *error);

void
xml_test_free(xml_doc_t *document);
}

static void
found(xml_t *object, void *context) {
  xml_t **result;

  result  = (xml_t **)context;
  *result = object;
}

static int
layout(void) {
  const size_t expected[] = {
    sizeof(xml_t), sizeof(xml_attr_t), sizeof(xml_doc_t), sizeof(xml_mem_t),
    sizeof(xml_parse_limits_t), sizeof(xml_parse_error_t), sizeof(xml_markup_span_t),
    offsetof(xml_t, val), offsetof(xml_t, valsize), offsetof(xml_t, tagsize),
    offsetof(xml_attr_t, valsize), offsetof(xml_attr_t, namesize),
    offsetof(xml_doc_t, root), offsetof(xml_mem_t, size), offsetof(xml_mem_t, capacity)
  };
  size_t actual[XML_ARR_LEN(expected)];

  xml_test_layout(actual);
  CHECK(!memcmp(actual, expected, sizeof(expected)));
  return 0;
}

static int
tree(xml_doc_t *document) {
  xml_t       *item, *longname, *empty, *called = NULL;
  xml_attr_t  *attribute;
  const xml_t *text;
  int          userdata = 0;
  xml_objmap_t map[] = {
    XML_OBJMAP_FN("item", found, &called),
    XML_OBJMAP_OBJ("longname", &userdata)
  };

  CHECK(document && document->root);
  CHECK(xml_tag_eq(document->root, "r"));
  CHECK((item = xml_elem(document->root, "item")));
  CHECK((longname = xml_elem_sz(document->root, "longname", 8)));
  CHECK((empty = xml_elem(document->root, "empty")));
  CHECK(xml_valtype(empty) == XML_UNKOWN);
  CHECK(xml_valtype(item) == XML_STRING);
  CHECK(xml_elem_packed4(document->root, xml__pack4("item", 4), 4) == item);
  CHECK(xml_elem_packed8(document->root, xml__pack8("longname", 8), 8) == longname);
  CHECK(xml_elem_packed(document->root, xml__pack8("longname", 8), 8) == longname);
  CHECK((attribute = xmla(item, "a")) && xmla_i32(attribute, -1) == 1);
  CHECK((text = xmls(item)));
  CHECK(xml_val_eq(text, "one") && xml_val_eqsz(text, "one", 3));
  CHECK(xml_val_eq_packed4(text, xml__pack4("one", 3), 3));
  CHECK(xml_val_eq_packed(text, xml__pack4("one", 3), 3));
  CHECK(!xml_val_eq_packed4(text, xml__pack4("two", 3), 3));
  CHECK((text = xmls(longname)));
  CHECK(xml_val_eq_packed8(text, xml__pack8("12345678", 8), 8));
  CHECK(xml_val_eq_packed(text, xml__pack8("12345678", 8), 8));
  CHECK(xml_i64(longname, -1) == 12345678 && xmls_sumlen(item) == 4);
  CHECK(xmls(document->root));
  xml_objmap_call(document->root, map, XML_ARR_LEN(map), NULL);
  CHECK(called == item && map[0].object == item && map[1].object == longname);
  CHECK(map[0].userdata == NULL && map[1].userdata == &userdata);
  CHECK(map[0].foundFunc.param == &called && map[1].foundFunc.func == NULL);
  return 0;
}

static int
parsers(void) {
  const char source[] = "<r a='v'>before<item a='1'>one</item>"
                        "<longname>12345678</longname><empty/>after<![CDATA[x<y]]></r>";
  xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_parse_error_t  error;
  xml_doc_t         *document;
  char              input[sizeof(source)];
  size_t            size;
  unsigned          api, mode;
  xml_options_t     options;

  size = sizeof(source) - 1;
  for (api = 0; api < 8; api++) {
    for (mode = 0; mode < 16; mode++) {
      memcpy(input, source, sizeof(source));
      options = (xml_options_t)mode;
      switch (api) {
        case 0: document = xml_parse(input, options); break;
        case 1: document = xmlc_parse(input, options); break;
        case 2: document = xml_parse_span(input, size, options, &error); break;
        case 3: document = xmlc_parse_span(input, size, options, &error); break;
        case 4: document = xml_parse_span_limited(input, size, options, &limits, &error); break;
        case 5: document = xmlc_parse_span_limited(input, size, options, &limits, &error); break;
        case 6: document = xmlc_parse_span_checked(input, size, options, &limits, &error); break;
        default: document = xml_test_parse(input, size, options, &error); break;
      }
      CHECK(document && !tree(document));
      CHECK(document->readonly == !!(mode & XML_READONLY));
      CHECK(document->reverse == !!(mode & XML_REVERSE));
      CHECK(document->sepPrefixes == !!(mode & XML_PREFIXES));
      CHECK(document->root->readonly == !!(mode & XML_READONLY));
      CHECK(document->root->reverse == !!(mode & XML_REVERSE));
      if (api >= 2)
        CHECK(error.status == XML_PARSE_OK && error.offset == size);
      if (mode & XML_READONLY)
        CHECK(!memcmp(input, source, sizeof(source)));

      /* release every allocation family through each language/library owner. */
      switch ((api + mode) % 3) {
        case 0: xml_free(document); break;
        case 1: xmlc_free(document); break;
        default: xml_test_free(document); break;
      }
    }
  }

  limits.max_tokens = 0;
  CHECK(!xml_parse_span_limited(source, size, XML_READONLY, &limits, &error));
  CHECK(error.status == XML_PARSE_LIMIT);
  CHECK(!xmlc_parse_span_checked(source, size, XML_READONLY, &limits, &error));
  CHECK(error.status == XML_PARSE_LIMIT);
  return 0;
}

static int
pages(void) {
  xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_parse_error_t  error;
  xml_doc_t         *document;
  xml_mem_t         *page;
  xml_t             *node;
  char              *source;
  size_t             size, i, count, bytes;

  size = 7 + 2000 * 7;
  CHECK((source = (char *)malloc(size)));
  memcpy(source, "<r>", 3);
  for (i = 0; i < 2000; i++)
    memcpy(source + 3 + 7 * i, "<item/>", 7);
  memcpy(source + size - 4, "</r>", 4);

  document = xml_parse_span_limited(source, size, XML_READONLY, &limits, &error);
  CHECK(document && error.status == XML_PARSE_OK);
  bytes = sizeof(*document);
  count = 0;
  for (page = (xml_mem_t *)document->memroot; page; page = page->next) {
    bytes += sizeof(*page) + page->capacity;
    count++;
  }
  CHECK(count == 2);
  count = 0;
  for (node = xml_xml(document->root); node; node = node->next) {
    CHECK(xml_tag_eq(node, "item") && node->parent == document->root);
    CHECK((uintptr_t)node % alignof(xml_t) == 0);
    count++;
  }
  CHECK(count == 2000);
  xml_test_free(document);

  limits.max_memory_bytes = bytes - 1;
  CHECK(!xml_parse_span_limited(source, size, XML_READONLY, &limits, &error));
  CHECK(error.status == XML_PARSE_LIMIT);
  limits.max_memory_bytes++;
  document = xml_parse_span_limited(source, size, XML_READONLY, &limits, &error);
  CHECK(document && error.status == XML_PARSE_OK);
  xmlc_free(document);
  free(source);
  return 0;
}

static xml_parse_status_t
markup(void *context, const xml_markup_span_t *span) {
  size_t *count;

  count = (size_t *)context;
  if (span->size != 8 || span->kind != XML_MARKUP_COMMENT)
    return XML_PARSE_INVALID_DATA;
  (*count)++;
  return XML_PARSE_OK;
}

static int
compiled(void) {
  const char source[] = "<s:svg xmlns:s='http://www.w3.org/2000/svg'><!--a--></s:svg>";
  xml_resolved_name_t name;
  xml_parse_error_t  error;
  xml_doc_t         *document;
  const char        *invalid;
  char              output[64];
  size_t            size, count = 0, decoded = 0;

  size = sizeof(source) - 1;
  document = xmlc_parse_span_checked(source, size,
                                     (xml_options_t)(XML_PREFIXES | XML_READONLY), NULL, &error);
  CHECK(document);
  CHECK(xmlc_validate_namespace_bindings(document, SIZE_MAX, &invalid) == XML_PARSE_OK);
  CHECK(xmlc_validate_expanded_attributes(document, SIZE_MAX, SIZE_MAX, &invalid) == XML_PARSE_OK);
  CHECK(xmlc_resolve_name(document->root, NULL, SIZE_MAX, &name, &invalid) == XML_PARSE_OK);
  CHECK(name.local_size == 3 && !memcmp(name.local, "svg", 3));
  CHECK(xmlc_decode_span(name.namespace_value, name.namespace_size, XML_DECODE_ATTRIBUTE,
                          output, sizeof(output), &decoded, &error) == XML_PARSE_OK);
  CHECK(decoded == 26 && !memcmp(output, "http://www.w3.org/2000/svg", 26));
  CHECK(xmlc_visit_markup(source, size, size, markup, &count, &error) == XML_PARSE_OK);
  CHECK(count == 1);
  CHECK(xmlc_validate_name_span("s:svg", 5, XML_QNAME, &error) == XML_PARSE_OK);
  CHECK(xml_validate_name_span("s:svg", 5, XML_QNAME, &error) == XML_PARSE_OK);
  CHECK(xml_decode_span("a&amp;b", 7, XML_DECODE_TEXT,
                         output, sizeof(output), &decoded, &error) == XML_PARSE_OK);
  CHECK(decoded == 3 && !memcmp(output, "a&b", 3));
  xmlc_free(document);
  return 0;
}

static int
printing(void) {
  const char source[] = "<r>text<![CDATA[x<y]]><empty/></r>";
  const char expected[] = "<r>text<![CDATA[x<y]]><empty /></r>";
  xml_doc_t *document;
  FILE      *stream;
  char       output[sizeof(expected)];
  size_t     size;

  CHECK((document = xml_parse(source, (xml_options_t)(XML_READONLY | XML_PRESERVE_TEXT))));
  CHECK((stream = tmpfile()));
  xml_print_uglify(stream, document->root);
  CHECK(!fflush(stream) && !fseek(stream, 0, SEEK_SET));
  size = fread(output, 1, sizeof(output), stream);
  CHECK(size == sizeof(expected) - 1 && !memcmp(output, expected, size));
  CHECK(!ferror(stream) && !fclose(stream));
  xml_free(document);
  return 0;
}

int
main(void) {
  CHECK(!layout());
  CHECK(!parsers());
  CHECK(!pages());
  CHECK(!compiled());
  CHECK(!printing());
  puts("C/C++ layout, 128 parser modes, shared ownership and compiled helpers pass");
  return 0;
}
