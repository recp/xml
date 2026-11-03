/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <xml/call/xml.h>
#include <stdio.h>
#include <inttypes.h>

#define CHECK(test) do { \
  if (!(test)) { \
    fprintf(stderr, "check failed at line %d\n", __LINE__); \
    return 1; \
  } \
} while (0)

static xml_doc_t*
parse(bool compiled, const char *text, xml_options_t options) {
  return compiled ? xmlc_parse(text, options) : xml_parse(text, options);
}

static int
modes(void) {
  const char *source = "<x:svg d='M0 0 L1 1'><g id='one'/><g id='two'/></x:svg>";
  xml_doc_t  *doc;
  xml_t      *child;
  xml_attr_t *attr;
  char       *copy;
  size_t      size;
  unsigned    mode, compiled;
  bool        readonly, reverse, prefix;

  size = strlen(source);
  for (compiled = 0u; compiled < 2u; compiled++) {
    for (mode = 0u; mode < 8u; mode++) {
      readonly = (mode & XML_READONLY) != 0u;
      reverse = (mode & XML_REVERSE) != 0u;
      prefix = (mode & XML_PREFIXES) != 0u;
      CHECK((copy = malloc(size + 1u)));
      memcpy(copy, source, size + 1u);
      doc = parse(compiled != 0u, readonly ? source : copy, (xml_options_t)mode);
      CHECK(doc && doc->root && !doc->root->parent && !doc->root->next);
      CHECK(doc->readonly == readonly && doc->reverse == reverse && doc->sepPrefixes == prefix);
      CHECK(doc->root->tagsize == (prefix ? 3u : 5u));
      CHECK(memcmp(doc->root->tag, prefix ? "svg" : "x:svg", doc->root->tagsize) == 0);
      CHECK((attr = xmla(doc->root, "d")) && attr->valsize == 9u);
      CHECK(memcmp(attr->val, "M0 0 L1 1", 9u) == 0);
      CHECK((child = xml_xml(doc->root)) && child->next && !child->next->next);
      CHECK((attr = xmla(child, "id")) && attr->valsize == 3u);
      CHECK(memcmp(attr->val, reverse ? "two" : "one", 3u) == 0);
      if (readonly)
        CHECK(memcmp(copy, source, size + 1u) == 0);
      else
        CHECK(attr->val[attr->valsize] == '\0');

      xml_free(doc);
      free(copy);
    }
  }
  return 0;
}

static int
lengths(void) {
  size_t      lengths[] = {0u, 65535u, 65536u, 70000u, 1048576u};
  xml_doc_t  *doc;
  xml_attr_t *attr;
  char       *source;
  size_t      i, size;
  unsigned    compiled, mode;

  for (compiled = 0u; compiled < 2u; compiled++) {
    for (mode = 0u; mode < 8u; mode++) {
      for (i = 0u; i < XML_ARR_LEN(lengths); i++) {
        size = lengths[i];
        CHECK((source = malloc(size + 20u)));
        memcpy(source, "<svg d=\"", 8u);
        memset(source + 8u, '0', size);
        memcpy(source + 8u + size, "\"/>", 4u);
        doc = parse(compiled != 0u, source, (xml_options_t)mode);
        CHECK(doc && (attr = xmla(doc->root, "d")) && attr->valsize == size);
        CHECK(attr->val[size] == ((mode & XML_READONLY) ? '"' : '\0'));
        xml_free(doc);
        free(source);
      }
    }
  }
  CHECK((source = malloc(65552u)));
  source[0] = '<';
  memset(source + 1u, 'a', 65536u);
  memcpy(source + 65537u, "/>", 3u);
  CHECK(!xml_parse(source, XML_READONLY) && !xmlc_parse(source, XML_READONLY));
  memcpy(source, "<r ", 3u);
  memset(source + 3u, 'a', 65536u);
  memcpy(source + 65539u, "='x'/>", 7u);
  CHECK(!xml_parse(source, XML_READONLY) && !xmlc_parse(source, XML_READONLY));
  free(source);
  printf("attribute size=%zu bytes, uint32 limit=%" PRIu32 "\n", sizeof(xml_attr_t), UINT32_MAX);
  return 0;
}

static int
errors(void) {
  const char *invalid[] = {"", " ", "<r>", "<r><x>", "<r><x></r>", "<root></roo>",
                           "<r></root>", "<r></r", "<r/><x/>", "<r/><", "junk<r/>",
                           "<r/>junk<x/>", "<r x='unfinished>", "<r><!--unfinished"};
  const char *valid[] = {"<r/>", "<r></r>", "<r><x/></r> \n\t", "<!--p--><r/><!--e-->",
                         "<?xml version='1.0'?><r/>", "<r x = \"a \"/>", "<r\tx='a'/>",
                         "<r x='backslash\\'/>", "<r x='&quot;'/>"};
  const char *complete = "<r><x/></r>";
  xml_doc_t  *doc;
  char       *copy;
  size_t      i, size;
  unsigned    mode, compiled;

  for (compiled = 0u; compiled < 2u; compiled++) {
    for (mode = 0u; mode < 8u; mode++) {
      for (i = 0u; i < XML_ARR_LEN(invalid); i++) {
        size = strlen(invalid[i]);
        CHECK((copy = malloc(size + 1u)));
        memcpy(copy, invalid[i], size + 1u);
        CHECK(!parse(compiled != 0u, copy, (xml_options_t)mode));
        free(copy);
      }
      for (i = 0u; i < XML_ARR_LEN(valid); i++) {
        size = strlen(valid[i]);
        CHECK((copy = malloc(size + 1u)));
        memcpy(copy, valid[i], size + 1u);
        doc = parse(compiled != 0u, copy, (xml_options_t)mode);
        CHECK(doc && doc->root);
        if (i == 5u) {
          CHECK(xmla(doc->root, "x")->valsize == 2u);
          CHECK(memcmp(xmla(doc->root, "x")->val, "a ", 2u) == 0);
        }
        xml_free(doc);
        free(copy);
      }
      for (i = 0u; i < strlen(complete); i++) {
        CHECK((copy = malloc(i + 1u)));
        memcpy(copy, complete, i);
        copy[i] = '\0';
        CHECK(!parse(compiled != 0u, copy, (xml_options_t)mode));
        free(copy);
      }
    }
  }
  xml_free(NULL);
  xmlc_free(NULL);
  return 0;
}

int
main(void) {
  if (modes() || lengths() || errors())
    return 1;

  puts("inline/compiled modes, long values, complete roots, XML quotes and whitespace: pass");
  return 0;
}
