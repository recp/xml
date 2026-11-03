/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <xml/call/xml.h>
#include <xml/print.h>
#include <stdio.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

typedef struct Token {
  const char *value;
  bool        cdata;
  bool        element;
} Token;

typedef struct Fixture {
  const char *source;
  Token       tokens[8];
  size_t      count;
} Fixture;

static xml_doc_t*
parse(unsigned api, char *source, size_t size, unsigned mode, xml_parse_error_t *error) {
  const xml_parse_limits_t limits = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};
  xml_options_t           options;

  options = (xml_options_t)(mode | XML_PRESERVE_TEXT);
  switch (api) {
    case 0: return xml_parse(source, options);
    case 1: return xmlc_parse(source, options);
    case 2: return xml_parse_span(source, size, options, error);
    case 3: return xmlc_parse_span(source, size, options, error);
    case 4: return xml_parse_span_limited(source, size, options, &limits, error);
    default: return xmlc_parse_span_limited(source, size, options, &limits, error);
  }
}

static int
fixtures(void) {
  const Fixture fixtures[] = {
    {"<r> \t\r\n </r>", {{" \t\r\n ", false, false}}, 1},
    {"<r xml:space='preserve'>  </r>", {{"  ", false, false}}, 1},
    {"<r>!/? > &amp;</r>", {{"!/? > &amp;", false, false}}, 1},
    {"<r>a<![CDATA[]]><![CDATA[b]]>c</r>",
      {{"a", false, false}, {"", true, false}, {"b", true, false}, {"c", false, false}}, 4},
    {"<r> a <![CDATA[x<&amp;]]> b <!--skip--> c <?pi > still?> d <x/> end </r>",
      {{" a ", false, false}, {"x<&amp;", true, false}, {" b ", false, false},
       {" c ", false, false}, {" d ", false, false}, {"x", false, true}, {" end ", false, false}}, 7},
    {"<r><![CDATA[a]] ]>b]]></r>", {{"a]] ]>b", true, false}}, 1},
    {"<r><![CDATA[a]]]></r>", {{"a]", true, false}}, 1},
    {"<?xml version='1.0'?><!---valid--><r><?pi?>tail</r>", {{"tail", false, false}}, 1}
  };
  xml_parse_error_t error;
  xml_doc_t        *doc;
  xml_t            *node;
  const xml_t      *string;
  const Token      *token;
  char             *source;
  size_t            i, j, size, length, sum;
  unsigned          api, mode;

  for (api = 0; api < 6; api++) {
    for (mode = 0; mode < 8; mode++) {
      for (i = 0; i < XML_ARR_LEN(fixtures); i++) {
        size = strlen(fixtures[i].source);
        CHECK((source = malloc(size + (api < 2))));
        memcpy(source, fixtures[i].source, size);
        if (api < 2)
          source[size] = '\0';
        doc = parse(api, source, size, mode, &error);
        CHECK(doc && doc->root && !doc->root->cdata);
        CHECK(api < 2 || (error.status == XML_PARSE_OK && error.offset == size));
        node = xml_xml(doc->root);
        string = xmls(doc->root);
        sum = 1;
        for (j = 0; j < fixtures[i].count; j++) {
          token = &fixtures[i].tokens[(mode & XML_REVERSE) ? fixtures[i].count - j - 1 : j];
          length = strlen(token->value);
          CHECK(node && node->parent == doc->root && node->cdata == token->cdata);
          CHECK(node->readonly == !!(mode & XML_READONLY) && node->reverse == !!(mode & XML_REVERSE));
          if (token->element) {
            CHECK(node->type == XML_ELEMENT && node->tagsize == length);
            CHECK(!memcmp(node->tag, token->value, length) && !node->val);
          } else {
            CHECK(node->type == XML_STRING && node->valsize == length);
            CHECK(!memcmp(node->val, token->value, length));
            CHECK(node == string);
            string = xmls_next(string);
            sum += length;
            if (!(mode & XML_READONLY))
              CHECK(((const char *)node->val)[length] == '\0');
          }
          node = node->next;
        }
        CHECK(!node && !string && xmls_sumlen(doc->root) == sum);
        if (mode & XML_READONLY)
          CHECK(!memcmp(source, fixtures[i].source, size));
        xml_free(doc);
        free(source);
      }
    }
  }
  return 0;
}

static int
boundaries(void) {
  const char *valid[] = {"<r><![CDATA[]]></r>", "<r><![CDATA[x<&y]]> after</r>",
                         "<!--comment--><r>before<?pi > ?>after</r>"};
  const char *invalid[] = {"<![CDATA[x]]><r/>", "<r/><![CDATA[x]]>",
                           "<r><![word[x]]></r>", "<r><![CDATA[x] ></r>",
                           "<r><![CDATA[x]></r>", "<r><![CDATA[x]]</r>",
                           "<r><!--x--y--></r>", "<r><!--x---></r>",
                           "<r><?pi > missing</r>", "<r><?pi?</r>",
                           "<!DOCTYPE r><r/>", "<r><!unsupported></r>",
                           "<r/>!", "?<r/>"};
  xml_parse_error_t error;
  xml_doc_t        *doc;
  char             *source;
  size_t            i, size, length;
  unsigned          api, mode;

  for (api = 0; api < 6; api++) {
    for (mode = 0; mode < 8; mode++) {
      for (i = 0; i < XML_ARR_LEN(valid); i++) {
        size = strlen(valid[i]);
        for (length = 0; length < size; length++) {
          CHECK((source = malloc(length + 1)));
          memcpy(source, valid[i], length);
          source[length] = '\0';
          CHECK(!parse(api, source, length, mode, &error));
          CHECK(api < 2 || (error.status == XML_PARSE_INVALID_DATA && error.offset <= length));
          free(source);
          if (api >= 2 && length) {
            CHECK((source = malloc(length)));
            memcpy(source, valid[i], length);
            CHECK(!parse(api, source, length, mode, &error));
            CHECK(error.status == XML_PARSE_INVALID_DATA && error.offset <= length);
            free(source);
          }
        }
      }
      for (i = 0; i < XML_ARR_LEN(invalid); i++) {
        size = strlen(invalid[i]);
        CHECK((source = malloc(size + (api < 2))));
        memcpy(source, invalid[i], size);
        if (api < 2)
          source[size] = '\0';
        doc = parse(api, source, size, mode, &error);
        CHECK(!doc);
        CHECK(api < 2 || (error.status == XML_PARSE_INVALID_DATA && error.offset <= size));
        free(source);
      }
    }
  }
  return 0;
}

static int
long_cdata(void) {
  const size_t lengths[] = {0, 65535, 65536, 1048576};
  xml_parse_error_t error;
  xml_doc_t        *doc;
  const xml_t      *text;
  char             *source;
  size_t            i, length, size;
  unsigned          api, mode;

  for (api = 0; api < 6; api++) {
    for (mode = 0; mode < 8; mode++) {
      for (i = 0; i < XML_ARR_LEN(lengths); i++) {
        length = lengths[i];
        size = length + 19;
        CHECK((source = malloc(size + (api < 2))));
        memcpy(source, "<r><![CDATA[", 12);
        memset(source + 12, 'z', length);
        memcpy(source + 12 + length, "]]></r>", 7);
        if (api < 2)
          source[size] = '\0';
        doc = parse(api, source, size, mode, &error);
        CHECK(doc && (text = xmls(doc->root)) && text->cdata && text->valsize == length);
        CHECK(text->val == source + 12 && !text->next);
        if (!length)
          CHECK(xml_bool(doc->root, 9) == 9 && xml_i32(doc->root, 9) == 9);
        CHECK(!length || (((const char *)text->val)[0] == 'z' && ((const char *)text->val)[length - 1] == 'z'));
        xml_free(doc);
        free(source);
      }
    }
  }
  return 0;
}

static int
nesting(void) {
  const char       *values[] = {" inner ", "x<y", "b", " tail "};
  xml_parse_error_t error;
  const char       *input = "<r> pre <a> inner <![CDATA[x<y]]><b/> tail </a> post </r>";
  xml_doc_t        *doc;
  xml_t            *first, *a, *node;
  char             *source;
  size_t            size, i, index, length;
  unsigned          api, mode;

  size = strlen(input);
  for (api = 0; api < 6; api++) {
    for (mode = 0; mode < 8; mode++) {
      CHECK((source = malloc(size + 1)));
      memcpy(source, input, size + 1);
      CHECK((doc = parse(api, source, size, mode, &error)));
      CHECK((first = xml_xml(doc->root)) && (a = xml_elem(doc->root, "a")));
      CHECK(first->next == a && a->next && !a->next->next);
      CHECK(xml_val_eq(first, (mode & XML_REVERSE) ? " post " : " pre "));
      CHECK(xml_val_eq(a->next, (mode & XML_REVERSE) ? " pre " : " post "));
      node = xml_xml(a);
      for (i = 0; i < 4; i++) {
        index = (mode & XML_REVERSE) ? 3 - i : i;
        length = strlen(values[index]);
        CHECK(node && node->parent == a && node->cdata == (index == 1));
        if (index == 2) {
          CHECK(node->type == XML_ELEMENT && xml_tag_eq(node, "b") && !node->val);
        } else {
          CHECK(node->type == XML_STRING && xml_val_eqsz(node, values[index], length));
        }
        node = node->next;
      }
      CHECK(!node);
      xml_free(doc);
      free(source);
    }
  }
  return 0;
}

static int
printing(void) {
  char              output[128];
  xml_parse_error_t error;
  const char       *input = "<r> <![CDATA[<&amp;]]> \t<x/> tail </r>";
  const char       *expected = "<r> <![CDATA[<&amp;]]> \t<x /> tail </r>";
  xml_doc_t        *doc;
  FILE             *file;
  char             *source;
  size_t            size, written;
  unsigned          api, mode;

  size = strlen(input);
  for (api = 0; api < 6; api++) {
    for (mode = 0; mode < 8; mode += 2) {
      CHECK((source = malloc(size + 1)));
      memcpy(source, input, size + 1);
      CHECK((doc = parse(api, source, size, mode, &error)) && (file = tmpfile()));
      xml_print_ex(file, doc->root, 0);
      CHECK(!ferror(file) && !fflush(file) && !fseek(file, 0, SEEK_SET));
      written = fread(output, 1, sizeof(output), file);
      CHECK(written == strlen(expected) && !memcmp(output, expected, written));
      CHECK(!fclose(file));
      xml_free(doc);
      free(source);
    }
  }
  return 0;
}

int
main(void) {
  CHECK(!fixtures());
  CHECK(!boundaries());
  CHECK(!long_cdata());
  CHECK(!nesting());
  CHECK(!printing());
  printf("raw text and CDATA: pass; node=%zu attribute=%zu document=%zu bytes\n",
         sizeof(xml_t), sizeof(xml_attr_t), sizeof(xml_doc_t));
  return 0;
}
