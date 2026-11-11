/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../include/xml/call/xml.h"
#include "../include/xml/decode.h"

static const char xmlNamespace[]   = "http://www.w3.org/XML/1998/namespace";
static const char xmlnsNamespace[] = "http://www.w3.org/2000/xmlns/";

typedef struct Work {
  const xml_attr_t *rootBinding;
  const char       *cursor;
  size_t            remaining;
} Work;

static bool
consume(Work *work, size_t count, const char *cursor) {
  work->cursor = cursor;
  if (count > work->remaining)
    return false;

  work->remaining -= count;
  return true;
}

static bool
declaration(const xml_attr_t *attribute, const char **prefix, size_t *size) {
  if (attribute->namesize == 5 && !memcmp(attribute->name, "xmlns", 5)) {
    *prefix = NULL;
    *size   = 0;
    return true;
  }
  if (attribute->namesize > 6 && !memcmp(attribute->name, "xmlns:", 6)) {
    *prefix = attribute->name + 6;
    *size   = attribute->namesize - 6;
    return true;
  }
  return false;
}

static xml_parse_status_t
check_declaration(const xml_attr_t *attribute, const char *prefix, size_t size, Work *work) {
  char               value[sizeof(xmlNamespace)];
  xml_parse_error_t  error;
  size_t            length;
  xml_parse_status_t status;
  bool              xml;

  if (size == 5 && !memcmp(prefix, "xmlns", 5)) {
    work->cursor = prefix;
    return XML_PARSE_INVALID_DATA;
  }
  if (!consume(work, attribute->valsize, attribute->val))
    return XML_PARSE_LIMIT;

  status = xml_decode_span(attribute->val, attribute->valsize, XML_DECODE_ATTRIBUTE,
                            NULL, SIZE_MAX, &length, &error);
  if (status != XML_PARSE_OK) {
    work->cursor = attribute->val + error.offset;
    return status;
  }

  xml = size == 3 && !memcmp(prefix, "xml", 3);
  if ((!length && size) || (xml && length != sizeof(xmlNamespace) - 1)) {
    work->cursor = attribute->val;
    return XML_PARSE_INVALID_DATA;
  }
  if (length != sizeof(xmlNamespace) - 1 && length != sizeof(xmlnsNamespace) - 1)
    return XML_PARSE_OK;

  if (!consume(work, attribute->valsize, attribute->val))
    return XML_PARSE_LIMIT;

  status = xml_decode_span(attribute->val, attribute->valsize, XML_DECODE_ATTRIBUTE,
                            value, sizeof(value), &length, &error);
  if (status != XML_PARSE_OK) {
    work->cursor = attribute->val + error.offset;
    return status;
  }
  if ((length == sizeof(xmlNamespace) - 1
       && !memcmp(value, xmlNamespace, length)) != xml
      || (length == sizeof(xmlnsNamespace) - 1 && !memcmp(value, xmlnsNamespace, length))) {
    work->cursor = attribute->val;
    return XML_PARSE_INVALID_DATA;
  }
  return XML_PARSE_OK;
}

static xml_parse_status_t
check_prefix(const xml_t *element, const char *prefix, size_t size, Work *work) {
  const xml_t      *scope;
  const xml_attr_t *attribute;

  if (!consume(work, size, prefix))
    return XML_PARSE_LIMIT;
  if (size == 3 && !memcmp(prefix, "xml", 3))
    return XML_PARSE_OK;
  if (size == 5 && !memcmp(prefix, "xmlns", 5)) {
    work->cursor = prefix;
    return XML_PARSE_INVALID_DATA;
  }

  /* a root binding stays bound: valid descendants cannot undeclare a prefix.
   * this proves boundness only, not the URI selected by a shadowing declaration.
   */
  if (work->rootBinding && (size_t)work->rootBinding->namesize == size + 6
      && !memcmp(work->rootBinding->name + 6, prefix, size))
    return XML_PARSE_OK;

  for (scope = element; scope; scope = scope->parent) {
    if (!consume(work, 1, prefix))
      return XML_PARSE_LIMIT;

    for (attribute = scope->attr; attribute; attribute = attribute->next) {
      if (!consume(work, (size_t)attribute->namesize + 1, attribute->name))
        return XML_PARSE_LIMIT;
      if ((size_t)attribute->namesize == size + 6
          && !memcmp(attribute->name, "xmlns:", 6)
          && !memcmp(attribute->name + 6, prefix, size)) {
        if (!scope->parent)
          work->rootBinding = attribute;
        return XML_PARSE_OK;
      }
    }
  }
  work->cursor = prefix;
  return XML_PARSE_INVALID_DATA;
}

static xml_parse_status_t
check_element(const xml_t *element, Work *work) {
  const xml_attr_t *attribute, *first = NULL;
  const char       *prefix, *colon;
  size_t            size;
  xml_parse_status_t status;

  /* validate all local declarations before resolving any use on this element. */
  for (attribute = element->attr; attribute; attribute = attribute->next) {
    if (!consume(work, (size_t)attribute->namesize + 1, attribute->name))
      return XML_PARSE_LIMIT;
    if (!declaration(attribute, &prefix, &size)) {
      if (!first && memchr(attribute->name, ':', attribute->namesize))
        first = attribute;
      continue;
    }

    status = check_declaration(attribute, prefix, size, work);
    if (status != XML_PARSE_OK)
      return status;
  }

  if (element->prefixsize) {
    status = check_prefix(element, element->prefix, element->prefixsize, work);
    if (status != XML_PARSE_OK)
      return status;
  }
  for (attribute = first; attribute; attribute = attribute->next) {
    if (!consume(work, (size_t)attribute->namesize + 1, attribute->name))
      return XML_PARSE_LIMIT;
    if (declaration(attribute, &prefix, &size))
      continue;

    if (!(colon = memchr(attribute->name, ':', attribute->namesize)))
      continue;

    status = check_prefix(element, attribute->name, (size_t)(colon - attribute->name), work);
    if (status != XML_PARSE_OK)
      return status;
  }
  return XML_PARSE_OK;
}

XML_EXPORT
xml_parse_status_t
xmlc_validate_namespace_bindings(const xml_doc_t *document, size_t max_work, const char **invalid) {
  Work               work = {NULL, NULL, max_work};
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
      status = check_element(node, &work);
      if (status != XML_PARSE_OK)
        goto err;
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

XML_EXPORT
xml_parse_status_t
xmlc_resolve_name(const xml_t          *element,
                  const xml_attr_t     *attribute,
                  size_t                max_work,
                  xml_resolved_name_t  *name,
                  const char          **invalid) {
  xml_resolved_name_t resolved = {0};
  Work               work = {NULL, NULL, max_work};
  const xml_t       *scope;
  const xml_attr_t  *binding;
  const char        *prefix, *declared, *cursor, *colon;
  size_t             size, declaredSize, nameSize;
  xml_parse_status_t status = XML_PARSE_OK;

  if (invalid)
    *invalid = NULL;
  if (name)
    *name = resolved;
  if (!element || element->type != XML_ELEMENT || !name)
    return XML_PARSE_INVALID_ARGUMENT;

  cursor = attribute ? attribute->name : element->tag;
  if (attribute)
    nameSize = attribute->namesize;
  else
    nameSize = (size_t)element->prefixsize + element->tagsize + (element->prefixsize != 0);
  if (!consume(&work, nameSize + 1, cursor)) {
    status = XML_PARSE_LIMIT;
    goto err;
  }

  if (attribute) {
    resolved.local      = attribute->name;
    resolved.local_size = attribute->namesize;
    colon               = memchr(attribute->name, ':', attribute->namesize);
    prefix              = attribute->name;
    size                = colon ? (size_t)(colon - prefix) : 0;
    if (colon) {
      resolved.local      = colon + 1;
      resolved.local_size -= size + 1;
    }
  } else {
    resolved.local      = element->tag;
    resolved.local_size = element->tagsize;
    prefix              = element->prefix;
    size                = element->prefixsize;
  }

  if (size == 3 && !memcmp(prefix, "xml", 3)) {
    resolved.namespace_value = xmlNamespace;
    resolved.namespace_size  = sizeof(xmlNamespace) - 1;
  } else if ((size == 5 && !memcmp(prefix, "xmlns", 5))
             || (attribute && !size && nameSize == 5 && !memcmp(cursor, "xmlns", 5))) {
    resolved.namespace_value = xmlnsNamespace;
    resolved.namespace_size  = sizeof(xmlnsNamespace) - 1;
  } else if (size || !attribute) {
    for (scope = element; scope; scope = scope->parent) {
      if (!consume(&work, 1, cursor)) {
        status = XML_PARSE_LIMIT;
        goto err;
      }
      for (binding = scope->attr; binding; binding = binding->next) {
        if (!consume(&work, (size_t)binding->namesize + 1, binding->name)) {
          status = XML_PARSE_LIMIT;
          goto err;
        }
        if (!declaration(binding, &declared, &declaredSize) || declaredSize != size
            || (size && memcmp(declared, prefix, size)))
          continue;

        if (binding->valsize) {
          resolved.namespace_value = binding->val;
          resolved.namespace_size  = binding->valsize;
        }
        goto done;
      }
    }
    if (size) {
      work.cursor = cursor;
      status      = XML_PARSE_INVALID_DATA;
      goto err;
    }
  }

done:
  *name = resolved;
  return XML_PARSE_OK;

err:
  if (invalid)
    *invalid = work.cursor;
  return status;
}
