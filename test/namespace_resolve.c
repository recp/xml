/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <xml/call/xml.h>
#include <xml/call/decode.h>
#include <stdio.h>

#define CHECK(test) do { \
  if (!(test)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #test); \
    return 1; \
  } \
} while (0)

typedef struct Fixture {
  const char *source, *attribute, *local, *uri;
  unsigned    depth, sibling;
} Fixture;

static const xml_t*
child(const xml_t *element, unsigned index) {
  const xml_t *node;
  unsigned     count = 0;

  if (element->reverse) {
    for (node = element->val; node; node = node->next) {
      count += node->type == XML_ELEMENT;
    }
    if (index >= count)
      return NULL;
    index = count - index - 1;
  }

  for (node = element->val; node; node = node->next) {
    if (node->type == XML_ELEMENT) {
      if (!index)
        return node;
      index--;
    }
  }
  return NULL;
}

static bool
empty(const xml_resolved_name_t *name) {
  return !name->local && !name->local_size && !name->namespace_value && !name->namespace_size;
}

static int
fixtures(void) {
  const Fixture fixtures[] = {
    {"<r/>", NULL, "r", "", 0, 0},
    {"<r xmlns='u'/>", NULL, "r", "u", 0, 0},
    {"<p:r xmlns:p='u'/>", NULL, "r", "u", 0, 0},
    {"<r xmlns='u'><n/></r>", NULL, "n", "u", 1, 0},
    {"<r xmlns='u'><n xmlns='v'/></r>", NULL, "n", "v", 1, 0},
    {"<r xmlns='u'><n xmlns=''><x/></n></r>", NULL, "x", "", 2, 0},
    {"<r xmlns='u'><n xmlns='v'/><n/></r>", NULL, "n", "u", 1, 1},
    {"<r xmlns:p='u'><p:n xmlns:p='v'/><p:n/></r>", NULL, "n", "u", 1, 1},
    {"<r xmlns:p='u'><p:n xmlns:p='v'><p:x/></p:n></r>", NULL, "x", "v", 2, 0},
    {"<r xmlns='u' a='v'/>", "a", "a", "", 0, 0},
    {"<r xmlns='u' xmlns:p='v' p:a='x'/>", "p:a", "a", "v", 0, 0},
    {"<r xmlns:p='u'><n xmlns:p='v' p:a='x'/></r>", "p:a", "a", "v", 1, 0},
    {"<r xmlns:p='u&amp;v'><p:n/></r>", NULL, "n", "u&v", 1, 0},
    {"<r xmlns:p='u\r\nx'><p:n/></r>", NULL, "n", "u x", 1, 0},
    {"<r xmlns:p='u&#9;x'><p:n/></r>", NULL, "n", "u\tx", 1, 0},
    {"<r xmlns:p='http://x/%61'><p:n/></r>", NULL, "n", "http://x/%61", 1, 0},
    {"<r xmlns:p='http://x/A'><p:n/></r>", NULL, "n", "http://x/A", 1, 0},
    {"<xml:r/>", NULL, "r", "http://www.w3.org/XML/1998/namespace", 0, 0},
    {"<r xml:lang='en'/>", "xml:lang", "lang", "http://www.w3.org/XML/1998/namespace", 0, 0},
    {"<r xmlns:xml='http://www.w3.org/XML/1998/namesp&#97;ce' xml:lang='en'/>",
     "xml:lang", "lang", "http://www.w3.org/XML/1998/namespace", 0, 0},
    {"<r xmlns:XML='u' XML:a='v'/>", "XML:a", "a", "u", 0, 0},
    {"<r xmlns='u'/>", "xmlns", "xmlns", "http://www.w3.org/2000/xmlns/", 0, 0},
    {"<r xmlns:p='u'/>", "xmlns:p", "p", "http://www.w3.org/2000/xmlns/", 0, 0},
    {"<xmlns/>", NULL, "xmlns", "", 0, 0},
    {"<r xmlns:\xcf\x80='u'><\xcf\x80:\xc3\xa9/></r>", NULL, "\xc3\xa9", "u", 1, 0},
    {"<r xmlns:\xcf\x80='u'><n \xcf\x80:\xc3\xa9='v'/></r>", "\xcf\x80:\xc3\xa9", "\xc3\xa9", "u", 1, 0},
    {"<r xmlns:p='u'>text<![CDATA[x]]><?pi?><p:n/></r>", NULL, "n", "u", 1, 0},
  };
  const xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_resolved_name_t name;
  char               decoded[128];
  xml_doc_t         *doc;
  const xml_t       *node;
  const xml_attr_t  *attribute;
  const char        *invalid;
  char              *input, *copy;
  size_t             i, size, decodedSize, cap;
  unsigned           mode, bounded, depth;

  for (mode = 0; mode < 16; mode++) {
    for (bounded = 0; bounded < 2; bounded++) {
      for (i = 0; i < XML_ARR_LEN(fixtures); i++) {
        size = strlen(fixtures[i].source);
        CHECK((input = malloc(size)));
        CHECK((copy = malloc(size)));
        memcpy(input, fixtures[i].source, size);
        CHECK((doc = xmlc_parse_span_checked(input, size, (xml_options_t)(mode | XML_PREFIXES),
                                             bounded ? &limits : NULL, NULL)));
        CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, NULL) == XML_PARSE_OK);
        memcpy(copy, input, size);

        node = doc->root;
        for (depth = 0; depth < fixtures[i].depth; depth++) {
          CHECK((node = child(node, depth + 1 == fixtures[i].depth ? fixtures[i].sibling : 0)));
        }
        attribute = fixtures[i].attribute ? xmla(node, fixtures[i].attribute) : NULL;
        CHECK(!fixtures[i].attribute || attribute);
        CHECK(xmlc_resolve_name(node, attribute, SIZE_MAX, &name, &invalid) == XML_PARSE_OK && !invalid);
        CHECK(name.local_size == strlen(fixtures[i].local));
        CHECK(!memcmp(name.local, fixtures[i].local, name.local_size));
        CHECK(name.local >= input && name.local + name.local_size <= input + size);
        CHECK(xmlc_decode_span(name.namespace_value, name.namespace_size, XML_DECODE_ATTRIBUTE,
                                decoded, sizeof(decoded), &decodedSize, NULL) == XML_PARSE_OK);
        if (decodedSize != strlen(fixtures[i].uri) || memcmp(decoded, fixtures[i].uri, decodedSize))
          fprintf(stderr, "fixture %zu mode %u bounded %u URI %.*s\n",
                  i, mode, bounded, (int)decodedSize, decoded);
        CHECK(decodedSize == strlen(fixtures[i].uri) && !memcmp(decoded, fixtures[i].uri, decodedSize));
        CHECK(decodedSize || (!name.namespace_value && !name.namespace_size));

        for (cap = 0; cap < 1024; cap++) {
          if (xmlc_resolve_name(node, attribute, cap, &name, &invalid) == XML_PARSE_OK)
            break;
          CHECK(empty(&name) && invalid >= input && invalid < input + size);
        }
        CHECK(cap && cap < 1024 && !invalid);
        CHECK(xmlc_resolve_name(node, attribute, cap - 1, &name, &invalid) == XML_PARSE_LIMIT && empty(&name));
        CHECK(xmlc_resolve_name(node, attribute, cap, &name, &invalid) == XML_PARSE_OK && !invalid);
        CHECK(xmlc_resolve_name(node, attribute, cap + 1, &name, NULL) == XML_PARSE_OK);
        CHECK(!memcmp(copy, input, size));
        xmlc_free(doc);
        free(copy);
        free(input);
      }
    }
  }
  return 0;
}

static int
long_edit(void) {
  const char         before[] = "<r xmlns:p='", after[] = "'><p:n p:a=''/></r>";
  xml_resolved_name_t name;
  xml_doc_t         *doc;
  const xml_t       *node;
  char              *source, *value;
  size_t             length = 65536, size;

  size = length + sizeof(before) + sizeof(after) - 2;
  CHECK((source = malloc(size)));
  memcpy(source, before, sizeof(before) - 1);
  value = source + sizeof(before) - 1;
  memset(value, 'u', length);
  memcpy(value + length, after, sizeof(after) - 1);
  CHECK((doc = xmlc_parse_span_checked(source, size, XML_READONLY | XML_PREFIXES, NULL, NULL)));
  CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, NULL) == XML_PARSE_OK);
  CHECK((node = child(doc->root, 0)));
  CHECK(xmlc_resolve_name(node, NULL, 18, &name, NULL) == XML_PARSE_OK);
  CHECK(name.namespace_value == value && name.namespace_size == length);
  CHECK(!memcmp(name.namespace_value, value, length));

  value[length - 1] = 'v';
  CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, NULL) == XML_PARSE_OK);
  CHECK(xmlc_resolve_name(node, xmla(node, "p:a"), 18, &name, NULL) == XML_PARSE_OK);
  CHECK(name.namespace_value == value && name.namespace_value[length - 1] == 'v');
  CHECK(name.namespace_size == length);
  xmlc_free(doc);
  free(source);
  return 0;
}

static int
deep_scope(void) {
  xml_resolved_name_t name;
  char               source[8192];
  xml_doc_t         *doc;
  const xml_t       *node;
  size_t             i, size;

  size = (size_t)snprintf(source, sizeof(source), "<r xmlns:p='u'>");
  for (i = 0; i < 512; i++) {
    memcpy(source + size, "<p:n>", 5);
    size += 5;
  }
  for (i = 0; i < 512; i++) {
    memcpy(source + size, "</p:n>", 6);
    size += 6;
  }
  memcpy(source + size, "</r>", 4);
  size += 4;
  CHECK((doc = xmlc_parse_span_checked(source, size, XML_READONLY | XML_PREFIXES, NULL, NULL)));
  CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, NULL) == XML_PARSE_OK);
  node = doc->root;
  for (i = 0; i < 512; i++) {
    CHECK((node = child(node, 0)));
  }
  CHECK(xmlc_resolve_name(node, NULL, 524, &name, NULL) == XML_PARSE_LIMIT && empty(&name));
  CHECK(xmlc_resolve_name(node, NULL, 525, &name, NULL) == XML_PARSE_OK);
  CHECK(name.namespace_size == 1 && name.namespace_value[0] == 'u');
  xmlc_free(doc);
  return 0;
}

static int
arguments(void) {
  const char         source[] = "<p:r/>";
  xml_resolved_name_t name;
  xml_t              text = {.type = XML_STRING};
  xml_doc_t         *doc;
  const char        *invalid = "sentinel";

  memset(&name, 0xff, sizeof(name));
  CHECK(xmlc_resolve_name(NULL, NULL, SIZE_MAX, &name, &invalid) == XML_PARSE_INVALID_ARGUMENT);
  CHECK(empty(&name) && !invalid);
  CHECK(xmlc_resolve_name(&text, NULL, SIZE_MAX, &name, &invalid) == XML_PARSE_INVALID_ARGUMENT);
  CHECK(empty(&name) && !invalid);
  CHECK(xmlc_resolve_name(NULL, NULL, SIZE_MAX, NULL, NULL) == XML_PARSE_INVALID_ARGUMENT);

  /* a defensive lookup failure does not replace whole-tree binding validation. */
  CHECK((doc = xmlc_parse_span_checked(source, sizeof(source) - 1, XML_READONLY | XML_PREFIXES, NULL, NULL)));
  CHECK(xmlc_validate_namespace_bindings(doc, SIZE_MAX, NULL) == XML_PARSE_INVALID_DATA);
  CHECK(xmlc_resolve_name(doc->root, NULL, SIZE_MAX, &name, &invalid) == XML_PARSE_INVALID_DATA);
  CHECK(empty(&name) && invalid == doc->root->tag);
  CHECK(xmlc_resolve_name(doc->root, NULL, SIZE_MAX, NULL, &invalid) == XML_PARSE_INVALID_ARGUMENT && !invalid);
  xmlc_free(doc);
  return 0;
}

int
main(void) {
  CHECK(!fixtures());
  CHECK(!long_edit());
  CHECK(!deep_scope());
  CHECK(!arguments());
  puts("resolved local/namespace spans, scope, defaults, edits and work caps: pass");
  return 0;
}
