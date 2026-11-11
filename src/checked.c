/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include <stdbool.h>
#include <stddef.h>

static bool
xml__parse_check_name(const char *text, size_t size, bool qualified, size_t *offset);

static bool
xml__parse_check_value(const char *text, size_t size, bool attribute, size_t *offset);

static bool
xml__parse_check_chars(const char *text, size_t size, size_t *offset);

static bool
xml__parse_check_pi(const char *text, size_t size, bool initial, size_t *offset);

struct xml_t;
struct xml_parse_budget_t;

static int
xml__parse_check_attributes(const struct xml_t *element,
                            const struct xml_parse_budget_t *budget,
                            const char **duplicate);

/* specialize the same parser at lexical token boundaries. */
#define XML__PARSE_CHECKED
#include "../include/xml/name.h"
#include "../include/xml/call/xml.h"

static const struct {
  const char *name;
  size_t      size;
} declarationFields[] = {{"version", 7}, {"encoding", 8}, {"standalone", 10}};

static bool
xml__parse_check_name(const char *text, size_t size, bool qualified, size_t *offset) {
  xml_parse_error_t  error;
  xml_parse_status_t status;

  status  = xml_validate_name_span(text, size, qualified ? XML_QNAME : XML_NAME, &error);
  *offset = error.offset;
  return status == XML_PARSE_OK;
}

static bool
xml__parse_check_value(const char *text, size_t size, bool attribute, size_t *offset) {
  xml_parse_error_t  error;
  xml_parse_status_t status;

  status = xml_decode_span(text, size, attribute ? XML_DECODE_ATTRIBUTE : XML_DECODE_TEXT,
                            NULL, SIZE_MAX, NULL, &error);
  *offset = error.offset;
  return status == XML_PARSE_OK;
}

static bool
xml__parse_check_chars(const char *text, size_t size, size_t *offset) {
  size_t   i = 0, count;
  uint32_t c;

  while (i < size) {
    c = (unsigned char)text[i];
    if (c < 0x80) {
      if (!xml__decode_char(c))
        break;
      i++;
    } else {
      count = xml__decode_utf8((const unsigned char *)text + i, size - i, &c);
      if (!count)
        break;
      i += count;
    }
  }
  *offset = i;
  return i == size;
}

static bool
check_declaration(const char *text, size_t size, size_t *offset) {
  size_t   i = 3, start, end;
  unsigned field;
  char     quote;

  for (field = 0; field < XML_ARR_LEN(declarationFields); field++) {
    if (field && i == size)
      return true;
    if (i == size || !xml__ascii_space(text[i]))
      goto err;

    do {
      i++;
    } while (i < size && xml__ascii_space(text[i]));
    if (i == size) {
      if (field)
        return true;
      goto err;
    }

    /* encoding is optional before standalone. */
    if (field == 1 && text[i] == 's')
      field = 2;
    if (size - i < declarationFields[field].size
        || memcmp(text + i, declarationFields[field].name, declarationFields[field].size))
      goto err;

    i += declarationFields[field].size;
    while (i < size && xml__ascii_space(text[i]))
      i++;
    if (i == size || text[i] != '=')
      goto err;

    i++;
    while (i < size && xml__ascii_space(text[i]))
      i++;
    if (i == size || (text[i] != '\'' && text[i] != '"'))
      goto err;

    quote = text[i++];
    start = i;
    while (i < size && text[i] != quote)
      i++;
    if (i == size)
      goto err;

    end = i;
    if (field == 0) {
      if (end - start < 3 || text[start] != '1' || text[start + 1] != '.') {
        i = start;
        goto err;
      }
      for (i = start + 2; i < end; i++) {
        if (text[i] < '0' || text[i] > '9')
          goto err;
      }
    } else if (field == 1) {
      /* this entry parses UTF-8 bytes and performs no transcoding. */
      if (end - start != 5 || (text[start] | 32) != 'u'
          || (text[start + 1] | 32) != 't' || (text[start + 2] | 32) != 'f'
          || text[start + 3] != '-' || text[start + 4] != '8') {
        i = start;
        goto err;
      }
    } else if (!((end - start == 3 && !memcmp(text + start, "yes", 3))
                 || (end - start == 2 && !memcmp(text + start, "no", 2)))) {
      i = start;
      goto err;
    }
    i = end + 1;
  }

  while (i < size && xml__ascii_space(text[i]))
    i++;
  if (i == size)
    return true;

err:
  *offset = i;
  return false;
}

static bool
xml__parse_check_pi(const char *text, size_t size, bool initial, size_t *offset) {
  size_t end = 0;

  while (end < size && !xml__ascii_space(text[end]))
    end++;
  if (!xml__parse_check_name(text, end, false, offset))
    return false;

  if (end == 3 && (text[0] | 32) == 'x'
      && (text[1] | 32) == 'm' && (text[2] | 32) == 'l') {
    *offset = 0;
    if (!initial || memcmp(text, "xml", 3))
      return false;

    return check_declaration(text, size, offset);
  }
  return xml__parse_check_chars(text, size, offset);
}

static int
attribute_order(const xml_attr_t *a, const xml_attr_t *b) {
  size_t size;
  int    order;

  size  = a->namesize < b->namesize ? a->namesize : b->namesize;
  order = memcmp(a->name, b->name, size);
  if (order)
    return order;
  if (a->namesize != b->namesize)
    return a->namesize < b->namesize ? -1 : 1;

  /* names share the borrowed source array; source order breaks exact ties. */
  return (a->name > b->name) - (a->name < b->name);
}

static bool
same_attribute(const xml_attr_t *a, const xml_attr_t *b) {
  return a->namesize == b->namesize && !memcmp(a->name, b->name, a->namesize);
}

static void
sift_attributes(const xml_attr_t **entries, size_t root, size_t count) {
  const xml_attr_t *entry;
  size_t           child;

  while (root < count / 2) {
    child = root * 2 + 1;
    if (child + 1 < count && attribute_order(entries[child], entries[child + 1]) < 0)
      child++;
    if (attribute_order(entries[root], entries[child]) >= 0)
      break;

    entry          = entries[root];
    entries[root]  = entries[child];
    entries[child] = entry;
    root           = child;
  }
}

static int
many_attributes(const xml_t *element, const xml_parse_budget_t *budget,
                 size_t count, const char **duplicate) {
  const xml_attr_t *local[32];
  const xml_attr_t **entries = local;
  const xml_attr_t *attribute, *entry;
  size_t           i, bytes;
  int              status = XML_PARSE_OK;

  if (count > PTRDIFF_MAX / sizeof(*entries))
    return XML_PARSE_RANGE;

  bytes = count * sizeof(*entries);
  if (count > XML_ARR_LEN(local)) {
    if (budget && bytes > budget->bytes)
      return XML_PARSE_LIMIT;
    if (!(entries = malloc(bytes)))
      return XML_PARSE_OUT_OF_MEMORY;
  }

  attribute = element->attr;
  for (i = 0; i < count; i++) {
    entries[i] = attribute;
    attribute  = attribute->next;
  }

  /* heapsort has bounded O(n log n) comparisons and leaves token order intact. */
  for (i = count / 2; i > 0; i--)
    sift_attributes(entries, i - 1, count);
  for (i = count; i > 1;) {
    entry          = entries[0];
    entries[0]     = entries[--i];
    entries[i]     = entry;
    sift_attributes(entries, 0, i);
  }

  for (i = 1; i < count; i++) {
    if (same_attribute(entries[i - 1], entries[i])
        && (!*duplicate || entries[i]->name < *duplicate))
      *duplicate = entries[i]->name;
  }
  if (*duplicate)
    status = XML_PARSE_INVALID_DATA;
  if (entries != local)
    free(entries);
  return status;
}

static int
xml__parse_check_attributes(const xml_t *element, const xml_parse_budget_t *budget,
                            const char **duplicate) {
  const xml_attr_t *attribute, *previous;
  size_t           count = 0;

  *duplicate = NULL;
  if (!element->attr || !element->attr->next)
    return XML_PARSE_OK;

  for (attribute = element->attr; attribute; attribute = attribute->next)
    count++;
  if (count > 8)
    return many_attributes(element, budget, count, duplicate);

  for (attribute = element->attr; attribute; attribute = attribute->next) {
    for (previous = attribute->next; previous; previous = previous->next) {
      if (same_attribute(attribute, previous)
          && (!*duplicate || attribute->name < *duplicate))
        *duplicate = attribute->name;
    }
  }
  return *duplicate ? XML_PARSE_INVALID_DATA : XML_PARSE_OK;
}

XML_EXPORT
xml_doc_t*
xmlc_parse_span_checked(const char               * __restrict contents,
                        size_t                               size,
                        xml_options_t                        options,
                        const xml_parse_limits_t            *limits,
                        xml_parse_error_t                   *error) {
  return xml_parse_span_limited(contents, size,
                                (xml_options_t)(options | XML_PRESERVE_TEXT), limits, error);
}
