/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <xml/call/decode.h>
#include <xml/call/xml.h>
#include <stdio.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

typedef struct Fixture {
  const char       *input, *output;
  xml_decode_mode_t mode;
} Fixture;

typedef struct Invalid {
  const char       *input;
  size_t            size, offset;
  xml_decode_mode_t mode;
} Invalid;

static xml_parse_status_t
decode(unsigned api, const char *input, size_t size, xml_decode_mode_t mode,
       char *output, size_t capacity, size_t *written, xml_parse_error_t *error) {
  return api ? xmlc_decode_span(input, size, mode, output, capacity, written, error)
             : xml_decode_span(input, size, mode, output, capacity, written, error);
}

static int
check(const char *input, const char *expected, xml_decode_mode_t mode) {
  xml_parse_error_t  error;
  char             *source, *output;
  size_t            size, length, written, cap;
  unsigned          api;

  size   = strlen(input);
  length = strlen(expected);
  CHECK((source = malloc(size + 1)) && (output = malloc(length + 2)));
  for (api = 0; api < 2; api++) {
    memcpy(source, input, size + 1);
    written = SIZE_MAX;
    CHECK(decode(api, source, size, mode, NULL, SIZE_MAX, &written, &error) == XML_PARSE_OK);
    CHECK(written == length && length <= size && error.status == XML_PARSE_OK && error.offset == size);
    for (cap = 0; cap <= length; cap++) {
      memset(output, 0xa5, length + 2);
      written = SIZE_MAX;
      if (cap < length) {
        CHECK(decode(api, source, size, mode, output + 1, cap, &written, &error) == XML_PARSE_LIMIT);
        CHECK(written == SIZE_MAX && error.status == XML_PARSE_LIMIT && error.offset <= size);
        CHECK(decode(api, source, size, mode, NULL, cap, NULL, NULL) == XML_PARSE_LIMIT);
      } else {
        CHECK(decode(api, source, size, mode, output + 1, cap, &written, &error) == XML_PARSE_OK);
        CHECK(written == length && !memcmp(output + 1, expected, length));
      }
      CHECK((unsigned char)output[0] == 0xa5 && (unsigned char)output[cap + 1] == 0xa5);
      CHECK(!memcmp(source, input, size + 1));
    }
    CHECK(decode(api, source, size, mode, source, length, &written, NULL) == XML_PARSE_OK);
    CHECK(written == length && !memcmp(source, expected, length) && source[size] == '\0');
  }
  free(source);
  free(output);
  return 0;
}

static int
fixtures(void) {
  const Fixture fixtures[] = {
    {"", "", XML_DECODE_TEXT},
    {"plain  \t\nvalue", "plain  \t\nvalue", XML_DECODE_TEXT},
    {"&amp;&lt;&gt;&apos;&quot;", "&<>'\"", XML_DECODE_TEXT},
    {"&amp;&lt;&gt;&apos;&quot;", "&<>'\"", XML_DECODE_ATTRIBUTE},
    {"&amp;&lt;&gt;&apos;&quot;", "&amp;&lt;&gt;&apos;&quot;", XML_DECODE_CDATA},
    {"&amp;lt;&#38;#13;", "&lt;&#13;", XML_DECODE_TEXT},
    {"a\r\nb\rc\n\rd\r", "a\nb\nc\n\nd\n", XML_DECODE_TEXT},
    {"a\r\nb\rc\n\rd\r", "a b c  d ", XML_DECODE_ATTRIBUTE},
    {"a\r\nb\rc\n\rd\r", "a\nb\nc\n\nd\n", XML_DECODE_CDATA},
    {" \t&#9;\r\n&#10;&#13; ", "  \t \n\r ", XML_DECODE_ATTRIBUTE},
    {"&#13;\n\r&#10;", "\r\n\n\n", XML_DECODE_TEXT},
    {"\r&#13;", "\n&#13;", XML_DECODE_CDATA},
    {"&#65;&#x41;&#x000041;&#00065;", "AAAA", XML_DECODE_TEXT},
    {"&#x7f;&#x80;&#2047;&#x800;", "\x7f\xc2\x80\xdf\xbf\xe0\xa0\x80", XML_DECODE_TEXT},
    {"&#xD7ff;&#xe000;&#xfffd;", "\xed\x9f\xbf\xee\x80\x80\xef\xbf\xbd", XML_DECODE_TEXT},
    {"&#65536;&#x1fffe;&#1114111;", "\xf0\x90\x80\x80\xf0\x9f\xbf\xbe\xf4\x8f\xbf\xbf", XML_DECODE_TEXT},
    {"T\xc3\xbcrk\xc3\xa7\x65 \xf0\x9f\x99\x82", "T\xc3\xbcrk\xc3\xa7\x65 \xf0\x9f\x99\x82", XML_DECODE_TEXT},
    {"\xc2\x85\xe2\x80\xa8", "\xc2\x85\xe2\x80\xa8", XML_DECODE_ATTRIBUTE},
    {"<![CDATA[&unknown;", "<![CDATA[&unknown;", XML_DECODE_CDATA},
    {"]]&gt;", "]]>", XML_DECODE_TEXT},
    {"&#93;]>", "]]>", XML_DECODE_TEXT},
    {"]]>", "]]>", XML_DECODE_ATTRIBUTE},
    {"[]]]x", "[]]]x", XML_DECODE_CDATA}
  };
  size_t i;

  for (i = 0; i < XML_ARR_LEN(fixtures); i++)
    CHECK(!check(fixtures[i].input, fixtures[i].output, fixtures[i].mode));
  return 0;
}

static int
invalid(void) {
#define BAD(s, m, o) {s, sizeof(s) - 1, o, m}
  const Invalid invalid[] = {
    BAD("&", XML_DECODE_TEXT, 0), BAD("&unknown;", XML_DECODE_TEXT, 0),
    BAD("&AMP;", XML_DECODE_TEXT, 0), BAD("ok&#;", XML_DECODE_TEXT, 2),
    BAD("&#x;", XML_DECODE_TEXT, 0), BAD("&#X41;", XML_DECODE_TEXT, 0),
    BAD("&#-1;", XML_DECODE_TEXT, 0), BAD("&#+1;", XML_DECODE_TEXT, 0),
    BAD("&# 1;", XML_DECODE_TEXT, 0), BAD("&#1 ;", XML_DECODE_TEXT, 0),
    BAD("&#xfg;", XML_DECODE_TEXT, 0), BAD("&#0;", XML_DECODE_TEXT, 0),
    BAD("&#x1f;", XML_DECODE_TEXT, 0), BAD("&#xD800;", XML_DECODE_TEXT, 0),
    BAD("&#xDFFF;", XML_DECODE_TEXT, 0), BAD("&#xfffe;", XML_DECODE_TEXT, 0),
    BAD("&#xffff;", XML_DECODE_TEXT, 0), BAD("&#1114112;", XML_DECODE_TEXT, 0),
    BAD("&#99999999999999999999999;", XML_DECODE_TEXT, 0),
    BAD("&#xfffffffffffffffffffff;", XML_DECODE_TEXT, 0),
    BAD("x<", XML_DECODE_TEXT, 1), BAD("x<", XML_DECODE_ATTRIBUTE, 1),
    BAD("a]]>", XML_DECODE_TEXT, 1), BAD("a]]>", XML_DECODE_CDATA, 1),
    BAD("x\0y", XML_DECODE_TEXT, 1), BAD("x\x01", XML_DECODE_ATTRIBUTE, 1),
    BAD("x\x0b", XML_DECODE_CDATA, 1), BAD("\xc0\x80", XML_DECODE_TEXT, 0),
    BAD("\xc1\xbf", XML_DECODE_CDATA, 0), BAD("\x80", XML_DECODE_TEXT, 0),
    BAD("\xc2", XML_DECODE_TEXT, 0), BAD("\xc2\x41", XML_DECODE_ATTRIBUTE, 0),
    BAD("\xe0\x80\x80", XML_DECODE_TEXT, 0), BAD("\xed\xa0\x80", XML_DECODE_TEXT, 0),
    BAD("\xed\xbf\xbf", XML_DECODE_CDATA, 0), BAD("\xef\xbf\xbe", XML_DECODE_TEXT, 0),
    BAD("\xef\xbf\xbf", XML_DECODE_ATTRIBUTE, 0), BAD("\xf0\x80\x80\x80", XML_DECODE_TEXT, 0),
    BAD("\xf4\x90\x80\x80", XML_DECODE_CDATA, 0), BAD("\xf5\x80\x80\x80", XML_DECODE_TEXT, 0),
    BAD("\xfe", XML_DECODE_TEXT, 0), BAD("\xff", XML_DECODE_TEXT, 0)
  };
#undef BAD
  xml_parse_error_t  error;
  char             *input, *output;
  size_t            i, written;
  unsigned          api;

  for (api = 0; api < 2; api++) {
    for (i = 0; i < XML_ARR_LEN(invalid); i++) {
      CHECK((input = malloc(invalid[i].size)) && (output = malloc(invalid[i].size)));
      memcpy(input, invalid[i].input, invalid[i].size);
      written = SIZE_MAX;
      CHECK(decode(api, input, invalid[i].size, invalid[i].mode, output,
                   invalid[i].size, &written, &error) == XML_PARSE_INVALID_DATA);
      CHECK(written == SIZE_MAX && error.status == XML_PARSE_INVALID_DATA && error.offset == invalid[i].offset);
      CHECK(decode(api, input, invalid[i].size, invalid[i].mode, input,
                   invalid[i].size, NULL, NULL) == XML_PARSE_INVALID_DATA);
      free(input);
      free(output);
    }
  }
  return 0;
}

static int
boundaries(void) {
  const char *references[] = {"&amp;", "&lt;", "&gt;", "&apos;", "&quot;", "&#65;", "&#x10ffff;"};
  const char *utf8[] = {"\xc2\x80", "\xe0\xa0\x80", "\xf4\x8f\xbf\xbf"};
  xml_parse_error_t  error;
  char             *input;
  size_t            i, j, written, size;
  unsigned          api, mode;

  for (api = 0; api < 2; api++) {
    for (mode = 0; mode < 3; mode++) {
      CHECK(decode(api, NULL, 0, (xml_decode_mode_t)mode, NULL, 0, &written, &error) == XML_PARSE_OK);
      CHECK(written == 0 && error.offset == 0);
      CHECK(decode(api, NULL, 1, (xml_decode_mode_t)mode, NULL, 0, NULL, &error) == XML_PARSE_INVALID_ARGUMENT);
      CHECK(decode(api, "x", SIZE_MAX, (xml_decode_mode_t)mode, NULL, 0, NULL, &error) == XML_PARSE_INVALID_ARGUMENT);
      CHECK(decode(api, "x", 1, (xml_decode_mode_t)3, NULL, 0, NULL, &error) == XML_PARSE_INVALID_ARGUMENT);
      CHECK(decode(api, "x", 1, (xml_decode_mode_t)-1, NULL, 0, NULL, &error) == XML_PARSE_INVALID_ARGUMENT);
      for (i = 0; i < XML_ARR_LEN(utf8); i++) {
        for (j = 1; j < strlen(utf8[i]); j++) {
          CHECK((input = malloc(j)));
          memcpy(input, utf8[i], j);
          CHECK(decode(api, input, j, (xml_decode_mode_t)mode, NULL, SIZE_MAX, NULL, NULL) == XML_PARSE_INVALID_DATA);
          free(input);
        }
      }
      for (i = 0; i < XML_ARR_LEN(references); i++) {
        size = strlen(references[i]);
        for (j = 1; j < size; j++) {
          CHECK((input = malloc(j)));
          memcpy(input, references[i], j);
          CHECK(decode(api, input, j, (xml_decode_mode_t)mode, NULL, SIZE_MAX, NULL, NULL)
                == (mode == XML_DECODE_CDATA ? XML_PARSE_OK : XML_PARSE_INVALID_DATA));
          free(input);
        }
      }
    }
  }
  return 0;
}

static int
long_values(void) {
  const size_t sizes[] = {65535, 65536, 1048576};
  char        *input, *output;
  size_t       i, j, written;
  unsigned     api;

  for (i = 0; i < XML_ARR_LEN(sizes); i++) {
    CHECK((input = malloc(sizes[i])) && (output = malloc(sizes[i])));
    for (api = 0; api < 2; api++) {
      memset(input, 'x', sizes[i]);
      CHECK(decode(api, input, sizes[i], XML_DECODE_TEXT, output, sizes[i], &written, NULL) == XML_PARSE_OK);
      CHECK(written == sizes[i] && !memcmp(input, output, written));
      memset(input, '0', sizes[i]);
      input[0] = '&';
      input[1] = '#';
      input[sizes[i] - 2] = '9';
      input[sizes[i] - 1] = ';';
      CHECK(decode(api, input, sizes[i], XML_DECODE_ATTRIBUTE, output, 1, &written, NULL) == XML_PARSE_OK);
      CHECK(written == 1 && output[0] == '\t');
      for (j = 0; j + 5 <= sizes[i]; j += 5)
        memcpy(input + j, "&amp;", 5);
      memset(input + j, 'x', sizes[i] - j);
      CHECK(decode(api, input, sizes[i], XML_DECODE_TEXT, input, sizes[i], &written, NULL) == XML_PARSE_OK);
      CHECK(written == j / 5 + sizes[i] - j);
      for (j = 0; j < sizes[i] / 5; j++)
        CHECK(input[j] == '&');
    }
    free(input);
    free(output);
  }
  return 0;
}

static int
integration(void) {
  char                     input[] = "<r a=' \t&#9;\r\n&amp;&#x1f642;'> hi\r\n&amp;<![CDATA[ \r<&amp; ]]></r>";
  char                     output[64], source[sizeof(input)], saved[sizeof(input)];
  const xml_parse_limits_t limits = {sizeof(input), 65568, 4, 1};
  xml_doc_t               *doc;
  xml_t                   *node;
  xml_attr_t              *attr;
  size_t                   written;
  unsigned                 mode, api;

  for (mode = 8; mode < 16; mode++) {
    for (api = 0; api < 2; api++) {
      memcpy(source, input, sizeof(input));
      doc = xmlc_parse_span_limited(source, sizeof(input) - 1,
                                    (xml_options_t)mode, &limits, NULL);
      memcpy(saved, source, sizeof(source));
      CHECK(doc && (attr = xmla(doc->root, "a")));
      CHECK(decode(api, attr->val, attr->valsize, XML_DECODE_ATTRIBUTE,
                   output, sizeof(output), &written, NULL) == XML_PARSE_OK);
      CHECK(written == 9 && !memcmp(output, "  \t &\xf0\x9f\x99\x82", written));
      for (node = xml_xml(doc->root); node; node = node->next) {
        CHECK(node->type == XML_STRING);
        CHECK(decode(api, node->val, node->valsize,
                     node->cdata ? XML_DECODE_CDATA : XML_DECODE_TEXT,
                     output, sizeof(output), &written, NULL) == XML_PARSE_OK);
        CHECK(node->cdata ? (written == 9 && !memcmp(output, " \n<&amp; ", written))
                          : (written == 5 && !memcmp(output, " hi\n&", written)));
      }
      CHECK(!memcmp(saved, source, sizeof(source)));
      xml_free(doc);
    }
  }
  return 0;
}

static int
oracle(void) {
  char                line[8192], input[4096], output[4096];
  xml_parse_error_t   error;
  size_t              size, written, i, length;
  xml_parse_status_t  status;
  unsigned            api, mode, byte;

  while (fgets(line, sizeof(line), stdin)) {
    CHECK(sscanf(line, "%u %u", &api, &mode) == 2 && api < 2 && mode < 3);
    CHECK(strlen(line) >= 4 && line[1] == ' ' && line[3] == ' ');
    length = strcspn(line + 4, "\r\n");
    CHECK(length % 2 == 0 && length / 2 <= sizeof(input));
    size = length / 2;
    for (i = 0; i < size; i++) {
      CHECK(sscanf(line + 4 + i * 2, "%2x", &byte) == 1);
      input[i] = (char)byte;
    }
    status = decode(api, input, size, (xml_decode_mode_t)mode, output, sizeof(output), &written, &error);
    printf("%u %zu ", (unsigned)status, error.offset);
    if (status == XML_PARSE_OK) {
      for (i = 0; i < written; i++)
        printf("%02x", (unsigned char)output[i]);
    }
    putchar('\n');
  }
  CHECK(!ferror(stdin));
  return 0;
}

int
main(int argc, char **argv) {
  if (argc == 2 && !strcmp(argv[1], "--oracle"))
    return oracle();

  CHECK(!fixtures());
  CHECK(!invalid());
  CHECK(!boundaries());
  CHECK(!long_values());
  CHECK(!integration());
  puts("bounded UTF-8 entity/EOL/attribute decoding: pass");
  return 0;
}
