/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../include/xml/call/xml.h"
#include "../include/xml/decode.h"

typedef struct Attribute {
  const xml_attr_t *attribute, *binding;
  const char       *local, *key;
  size_t            localSize, keySize;
} Attribute;

typedef struct Work {
  const char        *cursor;
  size_t             remaining, memory;
  xml_parse_status_t status;
} Work;

typedef enum Key {
  LOCAL_NAME,
  SECONDARY_KEY
} Key;

static bool
consume(Work *work, size_t count, const char *cursor) {
  if (work->status != XML_PARSE_OK)
    return false;

  work->cursor = cursor;
  if (count > work->remaining) {
    work->status = XML_PARSE_LIMIT;
    return false;
  }
  work->remaining -= count;
  return true;
}

static const char*
local_name(const xml_attr_t *attribute) {
  const char *colon;
  size_t      size;

  if (!(colon = memchr(attribute->name, ':', attribute->namesize)))
    return NULL;

  size = (size_t)(colon - attribute->name);
  if ((size == 3 && !memcmp(attribute->name, "xml", 3))
      || (size == 5 && !memcmp(attribute->name, "xmlns", 5)))
    return NULL;

  return colon + 1;
}

static int
compare(const Attribute *a, const Attribute *b, Key key, Work *work) {
  const char *left, *right;
  size_t      leftSize, rightSize;

  if (!consume(work, 1, a->attribute->name))
    return 0;

  left      = key == LOCAL_NAME ? a->local : a->key;
  right     = key == LOCAL_NAME ? b->local : b->key;
  leftSize  = key == LOCAL_NAME ? a->localSize : a->keySize;
  rightSize = key == LOCAL_NAME ? b->localSize : b->keySize;
  if (leftSize != rightSize)
    return leftSize < rightSize ? -1 : 1;
  if (!consume(work, leftSize, a->attribute->name))
    return 0;

  return memcmp(left, right, leftSize);
}

static int
order(const Attribute *a, const Attribute *b, Key key, Work *work) {
  int result;

  result = compare(a, b, key, work);
  if (result)
    return result;

  /* source order chooses the second spelling when equal expanded names meet. */
  return (a->attribute->name > b->attribute->name) - (a->attribute->name < b->attribute->name);
}

static bool
sift(Attribute *entries, size_t root, size_t count, Key key, Work *work) {
  Attribute entry;
  size_t    child;
  int       result;

  while (root < count / 2) {
    child = root * 2 + 1;
    if (child + 1 < count) {
      result = order(entries + child, entries + child + 1, key, work);
      if (work->status != XML_PARSE_OK)
        return false;
      if (result < 0)
        child++;
    }
    result = order(entries + root, entries + child, key, work);
    if (work->status != XML_PARSE_OK)
      return false;
    if (result >= 0)
      break;

    entry          = entries[root];
    entries[root]  = entries[child];
    entries[child] = entry;
    root           = child;
  }
  return true;
}

static bool
sort(Attribute *entries, size_t count, Key key, Work *work) {
  Attribute entry;
  size_t    i;

  for (i = count / 2; i > 0; i--) {
    if (!sift(entries, i - 1, count, key, work))
      return false;
  }
  for (i = count; i > 1;) {
    entry        = entries[0];
    entries[0]   = entries[--i];
    entries[i]   = entry;
    if (!sift(entries, 0, i, key, work))
      return false;
  }
  return true;
}

static xml_parse_status_t
resolve_binding(const xml_t *element, Attribute *entry, Work *work) {
  const xml_t      *scope;
  const xml_attr_t *attribute;
  size_t            size;

  size = (size_t)(entry->local - entry->attribute->name) - 1;
  for (scope = element; scope; scope = scope->parent) {
    if (!consume(work, 1, entry->attribute->name))
      return XML_PARSE_LIMIT;

    for (attribute = scope->attr; attribute; attribute = attribute->next) {
      if (!consume(work, (size_t)attribute->namesize + 1, attribute->name))
        return XML_PARSE_LIMIT;
      if ((size_t)attribute->namesize == size + 6
          && !memcmp(attribute->name, "xmlns:", 6)
          && !memcmp(attribute->name + 6, entry->attribute->name, size)) {
        entry->binding = attribute;
        return XML_PARSE_OK;
      }
    }
  }
  work->cursor = entry->attribute->name;
  return XML_PARSE_INVALID_DATA;
}

static xml_parse_status_t
resolve_group(const xml_t *element, Attribute *entries, size_t count, Work *work) {
  const xml_t      *scope;
  const xml_attr_t *attribute;
  Attribute        *entry;
  size_t            missing = count, first, last, middle, size, prefixSize, i;
  int               result;

  /* reuse the secondary key for prefix lookup; URI keys are supplied below. */
  for (i = 0; i < count; i++) {
    entries[i].key     = entries[i].attribute->name;
    entries[i].keySize = (size_t)(entries[i].local - entries[i].key) - 1;
  }
  if (!sort(entries, count, SECONDARY_KEY, work))
    return work->status;

  /* prefixes within one local-name group are distinct after checked parsing. */
  for (scope = element; scope; scope = scope->parent) {
    if (!consume(work, 1, entries[0].attribute->name))
      return XML_PARSE_LIMIT;
    for (attribute = scope->attr; attribute; attribute = attribute->next) {
      if (!consume(work, (size_t)attribute->namesize + 1, attribute->name))
        return XML_PARSE_LIMIT;
      if (attribute->namesize <= 6 || memcmp(attribute->name, "xmlns:", 6))
        continue;

      size  = attribute->namesize - 6;
      first = 0;
      last  = count;
      while (first < last) {
        middle = first + (last - first) / 2;
        entry  = entries + middle;
        if (!consume(work, 1, entry->attribute->name))
          return XML_PARSE_LIMIT;
        prefixSize = entry->keySize;
        if (prefixSize != size) {
          result = prefixSize < size ? -1 : 1;
        } else {
          if (!consume(work, size, entry->attribute->name))
            return XML_PARSE_LIMIT;
          result = memcmp(entry->key, attribute->name + 6, size);
        }
        if (result < 0) {
          first = middle + 1;
        } else if (result > 0) {
          last = middle;
        } else {
          if (!entry->binding) {
            entry->binding = attribute;
            if (!--missing)
              return XML_PARSE_OK;
          }
          break;
        }
      }
    }
  }
  for (i = 0; i < count; i++) {
    if (!entries[i].binding) {
      work->cursor = entries[i].attribute->name;
      break;
    }
  }
  return XML_PARSE_INVALID_DATA;
}

static xml_parse_status_t
check_group(const xml_t *element, Attribute *entries, size_t count, Work *work,
            const char **duplicate) {
  char               local[128];
  xml_parse_error_t  error;
  char              *buffer = local, *output;
  const xml_attr_t  *binding;
  size_t             i, bytes = 0;
  xml_parse_status_t status;

  if (count > 32) {
    status = resolve_group(element, entries, count, work);
    if (status != XML_PARSE_OK)
      return status;
  }
  for (i = 0; i < count; i++) {
    if (count <= 32) {
      status = resolve_binding(element, entries + i, work);
      if (status != XML_PARSE_OK)
        return status;
    }

    binding = entries[i].binding;
    if (!consume(work, binding->valsize, binding->val))
      return XML_PARSE_LIMIT;

    entries[i].key     = binding->val;
    entries[i].keySize = binding->valsize;
    if (!memchr(binding->val, '&', binding->valsize)
        && !memchr(binding->val, '\r', binding->valsize)
        && !memchr(binding->val, '\n', binding->valsize)
        && !memchr(binding->val, '\t', binding->valsize))
      continue;

    if (!consume(work, binding->valsize, binding->val))
      return XML_PARSE_LIMIT;
    status = xml_decode_span(binding->val, binding->valsize, XML_DECODE_ATTRIBUTE,
                              NULL, SIZE_MAX, &entries[i].keySize, &error);
    if (status != XML_PARSE_OK) {
      work->cursor = binding->val + error.offset;
      return status;
    }
    if (entries[i].keySize > PTRDIFF_MAX - bytes)
      return XML_PARSE_RANGE;

    bytes         += entries[i].keySize;
    entries[i].key = NULL;
  }

  if (bytes > sizeof(local)) {
    if (bytes > work->memory)
      return XML_PARSE_LIMIT;
    if (!(buffer = malloc(bytes)))
      return XML_PARSE_OUT_OF_MEMORY;
    work->memory -= bytes;
  }

  status = XML_PARSE_OK;
  output = buffer;
  for (i = 0; i < count; i++) {
    if (entries[i].key)
      continue;

    binding = entries[i].binding;
    if (!consume(work, binding->valsize, binding->val)) {
      status = XML_PARSE_LIMIT;
      goto cleanup;
    }
    status = xml_decode_span(binding->val, binding->valsize, XML_DECODE_ATTRIBUTE,
                              output, entries[i].keySize, NULL, &error);
    if (status != XML_PARSE_OK) {
      work->cursor = binding->val + error.offset;
      goto cleanup;
    }
    entries[i].key = output;
    output       += entries[i].keySize;
  }

  if (!sort(entries, count, SECONDARY_KEY, work)) {
    status = work->status;
    goto cleanup;
  }
  for (i = 1; i < count; i++) {
    if (!compare(entries + i - 1, entries + i, SECONDARY_KEY, work)
        && (!*duplicate || entries[i].attribute->name < *duplicate))
      *duplicate = entries[i].attribute->name;
    if (work->status != XML_PARSE_OK) {
      status = work->status;
      goto cleanup;
    }
  }

cleanup:
  if (buffer != local) {
    free(buffer);
    work->memory += bytes;
  }
  return status;
}

static xml_parse_status_t
check_attributes(const xml_t *element, Work *work) {
  Attribute          local[8];
  Attribute         *entries = local;
  const xml_attr_t  *attribute;
  const char        *name, *duplicate = NULL;
  size_t             count = 0, bytes = 0, i, begin, end;
  xml_parse_status_t status = XML_PARSE_OK;

  for (attribute = element->attr; attribute; attribute = attribute->next) {
    if (!consume(work, (size_t)attribute->namesize + 1, attribute->name))
      return XML_PARSE_LIMIT;
    if (!(name = local_name(attribute)))
      continue;

    if (count < XML_ARR_LEN(local))
      local[count] = (Attribute){attribute, NULL, name, NULL,
                                 attribute->namesize - (size_t)(name - attribute->name), 0};
    count++;
  }
  if (count < 2)
    return XML_PARSE_OK;

  if (count > XML_ARR_LEN(local)) {
    if (count > PTRDIFF_MAX / sizeof(*entries))
      return XML_PARSE_RANGE;

    bytes = count * sizeof(*entries);
    if (bytes > work->memory)
      return XML_PARSE_LIMIT;
    if (!(entries = malloc(bytes)))
      return XML_PARSE_OUT_OF_MEMORY;
    work->memory -= bytes;

    i = 0;
    for (attribute = element->attr; attribute; attribute = attribute->next) {
      if (!consume(work, (size_t)attribute->namesize + 1, attribute->name)) {
        status = XML_PARSE_LIMIT;
        goto cleanup;
      }
      if (!(name = local_name(attribute)))
        continue;
      entries[i++] = (Attribute){attribute, NULL, name, NULL,
                                 attribute->namesize - (size_t)(name - attribute->name), 0};
    }
  }

  if (!sort(entries, count, LOCAL_NAME, work)) {
    status = work->status;
    goto cleanup;
  }
  for (begin = 0; begin < count; begin = end) {
    for (end = begin + 1; end < count; end++) {
      if (compare(entries + begin, entries + end, LOCAL_NAME, work))
        break;
      if (work->status != XML_PARSE_OK) {
        status = work->status;
        goto cleanup;
      }
    }
    if (work->status != XML_PARSE_OK) {
      status = work->status;
      goto cleanup;
    }
    if (end - begin < 2)
      continue;

    status = check_group(element, entries + begin, end - begin, work, &duplicate);
    if (status != XML_PARSE_OK)
      goto cleanup;
  }
  if (duplicate) {
    work->cursor = duplicate;
    status       = XML_PARSE_INVALID_DATA;
  }

cleanup:
  if (entries != local) {
    free(entries);
    work->memory += bytes;
  }
  return status;
}

XML_EXPORT
xml_parse_status_t
xmlc_validate_expanded_attributes(const xml_doc_t  *document,
                                  size_t            max_work,
                                  size_t            max_memory,
                                  const char      **invalid) {
  Work               work = {NULL, max_work, max_memory, XML_PARSE_OK};
  const xml_t       *node;
  const char        *cursor;
  xml_parse_status_t status;

  if (invalid)
    *invalid = NULL;
  if (!document || !document->root || !document->sepPrefixes
      || document->root->type != XML_ELEMENT)
    return XML_PARSE_INVALID_ARGUMENT;

  node = document->root;
  while (node) {
    cursor = node->type == XML_ELEMENT ? node->tag : node->val;
    if (!consume(&work, 1, cursor)) {
      status = XML_PARSE_LIMIT;
      goto err;
    }
    if (node->type == XML_ELEMENT) {
      if (node->attr) {
        status = check_attributes(node, &work);
        if (status != XML_PARSE_OK)
          goto err;
      }
      if (node->val) {
        node = node->val;
        continue;
      }
    }
    while (!node->next) {
      if (!consume(&work, 1, cursor)) {
        status = XML_PARSE_LIMIT;
        goto err;
      }
      if (!(node = node->parent))
        return XML_PARSE_OK;
    }
    node = node->next;
  }
  return XML_PARSE_OK;

err:
  if (invalid)
    *invalid = work.cursor;
  return status;
}
