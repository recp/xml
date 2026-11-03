/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <xml/call/xml.h>
#include <stdio.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

static xml_doc_t*
parse(bool compiled, const char *text, size_t size,
      xml_options_t options, xml_parse_error_t *error) {
  return compiled ? xmlc_parse_span(text, size, options, error)
                  : xml_parse_span(text, size, options, error);
}

static bool
same_tree(const xml_t *a, const xml_t *b) {
  const xml_attr_t *left, *right;

  for (; a && b; a = a->next, b = b->next) {
    if (a->type != b->type || a->readonly != b->readonly || a->reverse != b->reverse
        || a->tagsize != b->tagsize || a->prefixsize != b->prefixsize
        || a->valsize != b->valsize || !!a->parent != !!b->parent)
      return false;

    if ((a->tagsize && memcmp(a->tag, b->tag, a->tagsize))
        || (a->prefixsize && memcmp(a->prefix, b->prefix, a->prefixsize)))
      return false;

    for (left = a->attr, right = b->attr; left && right;
         left = left->next, right = right->next) {
      if (left->namesize != right->namesize || left->valsize != right->valsize
          || left->namequote != right->namequote || left->valquote != right->valquote
          || (left->namesize && memcmp(left->name, right->name, left->namesize))
          || (left->valsize && memcmp(left->val, right->val, left->valsize)))
        return false;
    }
    if (left || right)
      return false;

    if (a->type == XML_ELEMENT) {
      if (!same_tree(xml_xml(a), xml_xml(b)))
        return false;
    } else if (a->valsize && memcmp(a->val, b->val, a->valsize)) {
      return false;
    }
  }
  return !a && !b;
}

static int
boundaries(void) {
  const char *sources[] = {
    "<r/>", "<x:r></x:r>", "<r a='quoted value'>text<c/></r>",
    "<r 'name'=`value`/>",
    "<?xml version='1.0'?><!--before--><r><![CDATA[data]]></r>",
    "<r a=''/>", "<r> \r\n\t</r>", "<r a = \"spaces \" ></r>"
  };
  xml_parse_error_t error;
  xml_doc_t        *doc;
  char             *span;
  size_t            i, size, length;
  unsigned          compiled, mode;

  for (compiled = 0; compiled < 2; compiled++) {
    for (mode = 0; mode < 8; mode++) {
      for (i = 0; i < XML_ARR_LEN(sources); i++) {
        size = strlen(sources[i]);
        for (length = 0; length <= size; length++) {
          /* exact allocations expose even a single-byte overread to ASan. */
          CHECK((span = malloc(length ? length : 1)));
          memcpy(span, sources[i], length);
          memset(&error, 0xff, sizeof(error));
          doc = parse(compiled, span, length, (xml_options_t)mode, &error);
          CHECK(error.offset <= length);
          if (length == size) {
            if (!doc)
              fprintf(stderr, "api=%u mode=%u source=%s status=%u offset=%zu\n",
                      compiled, mode, sources[i], error.status, error.offset);
            CHECK(doc && error.status == XML_PARSE_OK && error.offset == size);
            CHECK(doc->root && doc->root->tagsize == ((i == 1 && !(mode & XML_PREFIXES)) ? 3 : 1));
            xml_free(doc);
          } else {
            CHECK(!doc && error.status == XML_PARSE_INVALID_DATA);
          }
          if (mode & XML_READONLY)
            CHECK(memcmp(span, sources[i], length) == 0);
          free(span);
        }
      }
    }
  }
  return 0;
}

static int
errors(void) {
  char              suffix[] = "<r/><ignored/>";
  char              embedded[] = "<r a='x\0y'/>";
  xml_parse_error_t error;
  xml_doc_t        *doc;
  char             *source;
  size_t            i, size;
  unsigned          compiled, mode, kind;

  for (compiled = 0; compiled < 2; compiled++) {
    CHECK(!parse(compiled, NULL, 0, XML_READONLY, &error));
    CHECK(error.status == XML_PARSE_INVALID_ARGUMENT && error.offset == 0);
    CHECK(!parse(compiled, "<r/>", SIZE_MAX, XML_READONLY, &error));
    CHECK(error.status == XML_PARSE_INVALID_ARGUMENT && error.offset == 0);
    CHECK(!parse(compiled, "<r/>", 4, (xml_options_t)8, &error));
    CHECK(error.status == XML_PARSE_INVALID_ARGUMENT && error.offset == 0);
    CHECK(!parse(compiled, "", 0, XML_READONLY, &error));
    CHECK(error.status == XML_PARSE_INVALID_DATA && error.offset == 0);

    for (mode = 0; mode < 8; mode++) {
      memcpy(suffix, "<r/><ignored/>", sizeof(suffix));
      doc = parse(compiled, suffix, 4, (xml_options_t)mode, &error);
      CHECK(doc && error.status == XML_PARSE_OK && error.offset == 4);
      CHECK(memcmp(suffix + 4, "<ignored/>", sizeof(suffix) - 4) == 0);
      xml_free(doc);
      CHECK(!parse(compiled, embedded, sizeof(embedded) - 1, (xml_options_t)mode, &error));
      CHECK(error.status == XML_PARSE_INVALID_DATA && error.offset == 7);
      CHECK(memcmp(embedded, "<r a='x\0y'/>", sizeof(embedded)) == 0);

      /* embedded nulls cannot terminate a span early, even after the root. */
      for (i = 0; i < sizeof(suffix); i++) {
        memcpy(suffix, "<r/><ignored/>", sizeof(suffix));
        suffix[i] = '\0';
        CHECK(!parse(compiled, suffix, sizeof(suffix), (xml_options_t)mode, &error));
        CHECK(error.status == XML_PARSE_INVALID_DATA && error.offset == i);
      }
      for (kind = 0; kind < 3; kind++) {
        size = (size_t)UINT16_MAX + 1;
        CHECK((source = malloc(size + 10)));
        if (kind == 2) {
          memcpy(source, "<r ", 3);
          memset(source + 3, 'a', size);
          memcpy(source + 3 + size, "='x'/>", 6);
          size += 9;
        } else {
          source[0] = '<';
          memset(source + 1, 'a', size);
          memcpy(source + 1 + size, kind ? ":r/>" : "/>", kind ? 4 : 2);
          size += kind ? 5 : 3;
        }
        CHECK(!parse(compiled, source, size, (xml_options_t)mode, &error));
        CHECK(error.status == XML_PARSE_RANGE && error.offset <= size);
        free(source);
      }
    }
  }
  return 0;
}

static int
mutations(void) {
  const char *sources[] = {
    "<r/>", "<x:r a='text'>value</x:r>", "<r><!--x--></r>",
    "<r><![CDATA[x]]></r>", "<?x?><r><c/></r>", "<r a=unquoted/>",
    "<r>lead<a/>middle<b>deep<c/>tail</b>end</r>"
  };
  const char        changes[] = "<>/'\"`!?[]= :&\n\t";
  xml_parse_error_t error;
  xml_doc_t        *old, *doc;
  char             *span, *terminated;
  size_t            i, position, change, size;
  unsigned          compiled, mode;

  for (i = 0; i < XML_ARR_LEN(sources); i++) {
    size = strlen(sources[i]);
    CHECK((span = malloc(size)) && (terminated = malloc(size + 1)));
    for (position = 0; position < size; position++) {
      for (change = 0; change < sizeof(changes) - 1; change++) {
        for (compiled = 0; compiled < 2; compiled++) {
          for (mode = 0; mode < 8; mode++) {
            memcpy(span, sources[i], size);
            span[position] = changes[change];
            memcpy(terminated, span, size);
            terminated[size] = '\0';
            old = xml_parse(terminated, (xml_options_t)mode);
            doc = parse(compiled, span, size, (xml_options_t)mode, &error);
            CHECK((old != NULL) == (doc != NULL));
            CHECK(same_tree(old ? old->root : NULL, doc ? doc->root : NULL));
            CHECK(error.status == (doc ? XML_PARSE_OK : XML_PARSE_INVALID_DATA));
            CHECK(error.offset <= size);
            xml_free(old);
            xml_free(doc);
          }
        }
      }
    }
    free(terminated);
    free(span);
  }
  return 0;
}

int
main(void) {
  CHECK(!boundaries());
  CHECK(!errors());
  CHECK(!mutations());
  puts("bounded input, truncation, null bytes and error statuses: pass");
  return 0;
}
