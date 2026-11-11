/*
* Copyright (c), Recep Aslantas.
*
* MIT License (MIT), http://opensource.org/licenses/MIT
* Full license can be found in the LICENSE file
*/

#ifndef xml_impl_parse_h
#define xml_impl_parse_h

#include "../xml.h"
#include "impl_mem.h"

typedef enum xml_position {
  unknown   = 0,
  begintag  = 1,
  endtag    = 2,
  beginel   = 3,
  endel     = 4,
  beginattr = 5,
  content   = 6
} xml_position;

/* a null limit retains the existing null-terminated parser fast paths. */
XML_INLINE
char
xml__parse_char(const char *p, const char *limit, size_t offset) {
  if (limit && offset >= (size_t)(limit - p))
    return '\0';

  return p[offset];
}

XML_INLINE
char*
xml__parse_find(const char *p, int c, const char *limit) {
  return limit ? (char *)memchr(p, c, (size_t)(limit - p)) : (char *)strchr(p, c);
}

XML_INLINE
xml_t*
xml__parse_text(xml_doc_t *doc, xml_t *parent, const char *text,
                bool readonly, bool reverse, xml_parse_budget_t *budget) {
  xml_t *value;

  if (!(value = (xml_t *)xml__impl_calloc_budget(doc, sizeof(*value), budget)))
    return NULL;

  value->type     = XML_STRING;
  value->readonly = readonly;
  value->reverse  = reverse;
  value->cdata    = false;

  if (!reverse) {
    if (!parent->next)
      parent->next = value;
    else
      xml_xml(parent)->next = value;
  } else {
    value->next = (xml_t *)parent->val;
  }
  parent->val   = value;
  value->parent = parent;
  value->val    = (void *)text;
  return value;
}

XML_INLINE
xml_doc_t*
xml__parse(const char        * __restrict contents,
           xml_options_t                 options,
           const char                   *limit,
           xml_parse_error_t            *error,
           xml_parse_budget_t           *budget) {
  xml_t        tmproot;
  xml_doc_t   *doc;
  xml_t       *obj, *parent, *val;
  xml_attr_t  *attr;
  const char  *tag, *s;
  xml_position pos;

  xml_parse_status_t status = XML_PARSE_INVALID_DATA;

#if defined(XML__PARSE_CHECKED)
  const char  *checkedStart;
  size_t       invalidOffset;
  xml_parse_status_t attributeStatus;
#endif

  uint32_t     remaining;
  char        *p, *end, c, quote;
  bool         foundQuote, reverse, sepPrefixes, readonly, preserve, complete = false;
  
  if (!contents || (c = *contents) == '\0')
    return NULL;
  
  if (budget) {
    if (budget->bytes < sizeof(*doc) + sizeof(xml_mem_t) + XML_MEM_PAGE) {
      if (error)
        error->status = XML_PARSE_LIMIT;
      return NULL;
    }
    budget->bytes -= sizeof(*doc) + sizeof(xml_mem_t) + XML_MEM_PAGE;
  }

  if (!(doc = (xml_doc_t *)calloc(1, sizeof(*doc)))) {
    if (error)
      error->status = XML_PARSE_OUT_OF_MEMORY;
    return NULL;
  }

  if (!(doc->memroot = calloc(1, sizeof(xml_mem_t) + XML_MEM_PAGE))) {
    free(doc);
    if (error)
      error->status = XML_PARSE_OUT_OF_MEMORY;
    return NULL;
  }
  p              = (char *)contents;
#if defined(XML__PARSE_CHECKED)
  if (limit && (size_t)(limit - p) >= 3 && !memcmp(p, "\xef\xbb\xbf", 3)) {
    p += 3;
    c  = xml__parse_char(p, limit, 0);
  }
#endif

  memset(&tmproot, 0, sizeof(tmproot));
  tmproot.type   = XML_ELEMENT;

  s              = NULL;
  attr           = NULL;
  tag            = NULL;
  parent         = &tmproot;
  obj            = parent;
  pos            = unknown;
  quote          = '"';

  reverse        = options & XML_REVERSE;
  sepPrefixes    = options & XML_PREFIXES;
  readonly       = options & XML_READONLY;
  preserve       = options & XML_PRESERVE_TEXT;

  doc->reverse     = reverse;
  doc->readonly    = readonly;
  doc->sepPrefixes = sepPrefixes;

  tmproot.readonly = readonly;
  tmproot.reverse  = reverse;
  
  ((xml_mem_t *)doc->memroot)->capacity = XML_MEM_PAGE;
  
  do {
  again:
    if (preserve && pos == content && c != '<') {
      s = p;
      goto text;
    }
    if (preserve && obj == &tmproot && pos != begintag
        && c != '<' && !xml__ascii_space(c))
      goto err;

    /* child */
    switch (c) {
        /* trim */
      case ' ':
      case '\r':
      case '\n':
      case '\t':
#if defined(XML__PARSE_CHECKED)
        if (pos == begintag)
          goto err;
#endif
        do {
          c = xml__parse_char(++p, limit, 0);
        } while (c == ' ' || c == '\r' || c == '\n' || c == '\t');

        if (c == '\0')
          goto done;

        goto again;
      case '<': { /* TODO: what if we get << or <<!--  ->TAG>... here? */
        if (pos == begintag)
          goto err;
#if defined(XML__PARSE_CHECKED)
        if (pos == beginattr || pos == endtag)
          goto err;
#endif

        pos = begintag;
        
        /* advance bounded readonly cursors without revisiting this byte. */
        if (!readonly || limit) {
          if (!readonly)
            *p = '\0';

          if ((c = xml__parse_char(++p, limit, 0)) != '\0')
            goto again;
        }
        break;
      }
      case '!': { /* comments and CDATA */
        char c1, c2;

        if (preserve && pos != begintag)
          goto err;
        
        /* comments, skip for now. */
        if (xml__parse_char(p, limit, 1) == '-' && xml__parse_char(p, limit, 2) == '-') {
          p += preserve ? 3 : 2;
          c  = xml__parse_char(p, limit, 0);
          c2 = '\0';
#if defined(XML__PARSE_CHECKED)
          checkedStart = p;
#endif

          do {
            if (c == '\0')
              goto err;

            c1 = c2;
            c2 = c;
            c  = xml__parse_char(++p, limit, 0);
            if (preserve && c1 == '-' && c2 == '-' && c != '>')
              goto err;
            
          } while (!(c1 == '-' && c2 == '-' && c == '>'));

#if defined(XML__PARSE_CHECKED)
          if (!xml__parse_check_chars(checkedStart, (size_t)(p - checkedStart) - 2, &invalidOffset)) {
            p = (char *)checkedStart + invalidOffset;
            goto err;
          }
#endif
          pos = beginel;
          if (preserve)
            pos = obj == &tmproot ? unknown : content;
        }
        
        /* CDATA or similar data, skip for now. */
        else if (xml__parse_char(p, limit, 1) == '[') {
          if (preserve) {
            if (obj == &tmproot)
              goto err;

            if (limit) {
              if ((size_t)(limit - p) < 8 || memcmp(p, "![CDATA[", 8))
                goto err;
            } else if (strncmp(p, "![CDATA[", 8)) {
              goto err;
            }

            s = p + 8;
            p = (char *)s;
            for (;;) {
              if (!(end = xml__parse_find(p, ']', limit)))
                goto err;

              if (xml__parse_char(end, limit, 1) == ']'
                  && xml__parse_char(end, limit, 2) == '>')
                break;

              p = end + 1;
            }
            if ((size_t)(end - s) > UINT32_MAX)
              goto range;

#if defined(XML__PARSE_CHECKED)
            if (!xml__parse_check_chars(s, (size_t)(end - s), &invalidOffset)) {
              p = (char *)s + invalidOffset;
              goto err;
            }
#endif
            val = xml__parse_text(doc, obj, s, readonly, reverse, budget);
            if (!val)
              goto oom;

            val->valsize = (uint32_t)(end - s);
            val->cdata   = true;
            if (!readonly)
              *end = '\0';

            p   = end + 2;
            pos = content;
            break;
          }

          p  = p + 2;
          c  = xml__parse_char(p, limit, 0);
          c2 = '\0';
          
          do {
            if (c == '\0')
              goto err;

            c1 = c2;
            c2 = c;
            c  = xml__parse_char(++p, limit, 0);

          } while (!(c1 == ']' && c2 == ']' && c == '>')
                   && !(c1 == ']' && c2 == ' ' && c == '>'));

          pos = beginel;
        } else if (preserve) {
          goto err;
        }
        break;
      }
      case '?':
        /* skip XML header e.g. version line */
        if (pos == begintag) {
          if (preserve) {
            p++;
#if defined(XML__PARSE_CHECKED)
            checkedStart = p;
#endif
            for (;;) {
              if (!(end = xml__parse_find(p, '?', limit)))
                goto err;

              if (xml__parse_char(end, limit, 1) == '>')
                break;

              p = end + 1;
            }
#if defined(XML__PARSE_CHECKED)
            if (!xml__parse_check_pi(checkedStart, (size_t)(end - checkedStart),
                                     checkedStart - contents == 2
                                       || (checkedStart - contents == 5 && !memcmp(contents, "\xef\xbb\xbf", 3)),
                                     sepPrefixes,
                                     &invalidOffset)) {
              p = (char *)checkedStart + invalidOffset;
              goto err;
            }
#endif
            p   = end + 1;
            pos = obj == &tmproot ? unknown : content;
            break;
          }

          /* skip to value  */
          while (c != '>') {
            if (c == '\0')
              goto err;
            c = xml__parse_char(++p, limit, 0);
          }

          pos = unknown;
        } else if (preserve) {
          goto err;
        }
        break;
      case '/':
#if defined(XML__PARSE_CHECKED)
        if (pos != begintag && xml__parse_char(p, limit, 1) != '>')
          goto err;
        if (pos == beginattr) {
          attributeStatus = (xml_parse_status_t)xml__parse_check_attributes(obj, budget, &checkedStart);
          if (attributeStatus != XML_PARSE_OK) {
            status = attributeStatus;
            if (checkedStart)
              p = (char *)checkedStart;
            goto err;
          }
        }
#endif
        switch (pos) {
          case begintag:
            /* end xml tag */
            tag = obj->tag;
            c   = xml__parse_char(++p, limit, 0);

            if (sepPrefixes && obj->prefix && obj->prefixsize > 0) {
              const char *prefix;
              uint32_t    prefixsize;

              prefix     = obj->prefix;
              prefixsize = obj->prefixsize;
              do {
                if (c == '\0')
                  goto err;

                if (c != *prefix++)
                  goto err;

                c = xml__parse_char(++p, limit, 0);
              } while (--prefixsize > 0);

              if (c != ':')
                goto err;

              c = xml__parse_char(++p, limit, 0);
            }
            
            remaining = obj->tagsize;
            if (limit) {
              if (!tag || !remaining || (size_t)(limit - p) <= remaining)
                goto err;

              if (memcmp(p, tag, remaining))
                goto err;

              p += remaining;
              c  = *p;
              if (!xml__ascii_space(c) && c != '>')
                goto err;
            } else {
              do {
                if (c == '\0')
                  goto err;

                if (!tag || !remaining || c != *tag++)
                  goto err;

                remaining--;
                c = xml__parse_char(++p, limit, 0);
              } while (!xml__ascii_space(c) && c != '>');
              if (remaining)
                goto err;
            }
#if defined(XML__PARSE_CHECKED)
            while (xml__ascii_space(c))
              c = xml__parse_char(++p, limit, 0);
            if (c != '>')
              goto err;
#endif
            
            s = p + 1;
            p--;
            pos = endtag;
            break;
          case beginel:
          case beginattr:
            pos = endtag;
            break;
          default:
            break;
        }

        break;
      case '>':
#if defined(XML__PARSE_CHECKED)
        if (pos == begintag)
          goto err;
        if (pos == beginattr) {
          attributeStatus = (xml_parse_status_t)xml__parse_check_attributes(obj, budget, &checkedStart);
          if (attributeStatus != XML_PARSE_OK) {
            status = attributeStatus;
            if (checkedStart)
              p = (char *)checkedStart;
            goto err;
          }
        }
#endif
        if (pos == endtag) {
          /* switch parent back */
          if (!parent)
            goto err;
          
          if (!reverse) {
            obj->val  = obj->next;
            obj->next = NULL;
          }
          
          if (budget)
            budget->depth++;

          obj    = parent;
          parent = parent->parent;
          pos    = endel;
          complete = parent == NULL;
        } else {
          pos = beginel;
        }
        
        if (preserve && obj != &tmproot)
          pos = content;

        if (!readonly || limit) {
          if (!readonly)
            *p = '\0';

          if ((c = xml__parse_char(++p, limit, 0)) != '\0')
            goto again;
        }
        break;
      default: {
        switch (pos) {
          case begintag:
            if (complete)
              goto err;

            if (budget) {
              if (!budget->depth)
                goto exhausted;
              budget->depth--;
            }

            /* switch parent */
            parent        = obj;
            obj           = (xml_t *)xml__impl_calloc_budget(doc, sizeof(xml_t), budget);
            if (!obj)
              goto oom;

            obj->type     = XML_ELEMENT;
            obj->readonly = readonly;
            obj->reverse  = reverse;
            obj->cdata    = false;
            
            /* parent must not be NULL */
            
            if (!reverse) {
              if (!parent->next)
                parent->next = obj;
              else
                xml_xml(parent)->next = obj;
            } else {
              obj->next = (xml_t *)parent->val;
            }
            
            parent->val = obj;
            obj->parent = parent;
            obj->tag    = p;
#if defined(XML__PARSE_CHECKED)
            checkedStart = p;
#endif

            do {
              if (c == '\0')
                goto err;
              
              if (sepPrefixes) {
                if (c == ':') {
                  obj->prefix     = obj->tag;
                  if ((size_t)(p - obj->prefix) > UINT16_MAX)
                    goto range;

                  obj->prefixsize = (uint16_t)(p - obj->prefix);
                  obj->tag        = p + 1;
                }
              }

              c = xml__parse_char(++p, limit, 0);
            } while (!xml__ascii_space(c) && c != '/' && c != '>');
            
            if ((size_t)(p - obj->tag) > UINT16_MAX)
              goto range;

            obj->tagsize = (uint16_t)(p - obj->tag);
#if defined(XML__PARSE_CHECKED)
            if (!xml__parse_check_name(checkedStart, (size_t)(p - checkedStart), sepPrefixes, &invalidOffset)) {
              p = (char *)checkedStart + invalidOffset;
              goto err;
            }
#endif

            if (c == '>') {
              pos = beginel;
            } else if (c == '/') {
#if defined(XML__PARSE_CHECKED)
              if (xml__parse_char(p, limit, 1) != '>')
                goto err;
#endif
              pos = endtag;
              
              if (!readonly) {
                *p = '\0';
                if ((c = xml__parse_char(++p, limit, 0)) != '\0')
                  goto again;;
              }
            } else {
              pos = beginattr;
            }
            
            /*
             TODO: obly opens if error is handled
             if (c  == '\0')
             goto err;
             */
            
            if (limit && p == limit)
              goto err;

            s = p + 1;
            p--;
            break;
          case beginattr:
#if defined(XML__PARSE_CHECKED)
            if (c == '"' || c == '\'' || c == '`')
              goto err;
#endif
            attr = (xml_attr_t *)xml__impl_calloc_budget(doc, sizeof(xml_attr_t), budget);
            if (!attr)
              goto oom;
            
            /* attrib key */
            if ((foundQuote = (c == '"' || c == '\'' || c == '`'))) {
              quote            = c;
              attr->namequote  = c;
              c                = xml__parse_char(++p, limit, 0);
            }

            attr->name = end = p;
            
            if (foundQuote) {
              char *quoteEnd;

              quoteEnd = xml__parse_find(p, quote, limit);
              if (!quoteEnd)
                goto err;

              end = xml__rtrim_ascii(p, quoteEnd);
              p   = quoteEnd + 1;
              c   = xml__parse_char(p, limit, 0);
            } else {
              while (c != '=') {
                if (c == '\0')
                  goto err;
                
                if (c != ' ' && c != '\r'  && c != '\n' && c != '\t')
                  end = p + 1;
                
                c = xml__parse_char(++p, limit, 0);
              }
            }
            
            if ((size_t)(end - attr->name) > UINT16_MAX)
              goto range;

            attr->namesize = (uint16_t)(end - attr->name);
#if defined(XML__PARSE_CHECKED)
            if (!xml__parse_check_name(attr->name, attr->namesize, sepPrefixes, &invalidOffset)) {
              p = (char *)attr->name + invalidOffset;
              goto err;
            }
#endif
            if (!readonly)
              *end = '\0';

            if (attr->name == NULL || c  == '\0')
              goto err;
            
            /* attrib value */
            
            /* skip to value  */
            while (c != '=') {
              if (c == '\0')
                goto err;
              c = xml__parse_char(++p, limit, 0);
            }
            
            /* skip trailing equal */
            c = xml__parse_char(++p, limit, 0);
            while (xml__ascii_space(c))
              c = xml__parse_char(++p, limit, 0);

#if defined(XML__PARSE_CHECKED)
            if (c != '"' && c != '\'')
              goto err;
#endif
            if ((foundQuote = (c == '"' || c == '\'' || c == '`'))) {
              quote          = c;
              attr->valquote = c;
              c              = xml__parse_char(++p, limit, 0);
            }
            
            attr->val = end = p;
            
            if (foundQuote) {
              char *quoteEnd;

              quoteEnd = xml__parse_find(p, quote, limit);
              if (!quoteEnd)
                goto err;

              end = quoteEnd;
              p   = quoteEnd + 1;
            } else {
              while (c != '>' && c != '/') {
                if (c == '\0')
                  goto err;
                
                if (c != ' ' && c != '\r' && c != '\n' && c != '\t')
                  end = p + 1;
                
                c = xml__parse_char(++p, limit, 0);
              }
            }
            
            if ((size_t)(end - attr->val) > UINT32_MAX)
              goto range;

            attr->valsize = (uint32_t)(end - attr->val);
#if defined(XML__PARSE_CHECKED)
            if (!xml__parse_check_value(attr->val, attr->valsize, true, &invalidOffset)) {
              p = (char *)attr->val + invalidOffset;
              goto err;
            }
            if (!xml__ascii_space(xml__parse_char(p, limit, 0))
                && xml__parse_char(p, limit, 0) != '/' && xml__parse_char(p, limit, 0) != '>')
              goto err;
#endif
            if (!readonly)
              *end = '\0';

            if (limit && p == limit)
              goto err;

            c          = xml__parse_char(p, limit, 0);
            attr->next = obj->attr;
            obj->attr  = attr;
            attr       = NULL;
            s          = p + 1;
            
            goto again;
          case beginel:
          case endel:
          text:
            if (obj == &tmproot)
              goto err;

            val = xml__parse_text(doc, obj, s, readonly, reverse, budget);
            if (!val)
              goto oom;

            p = xml__parse_find(p, '<', limit);
            if (!p)
              goto err;
            
            if ((size_t)(p - (char *)s) > UINT32_MAX)
              goto range;

#if defined(XML__PARSE_CHECKED)
            if (!xml__parse_check_value(s, (size_t)(p - s), false, &invalidOffset)) {
              p = (char *)s + invalidOffset;
              goto err;
            }
#endif
            val->valsize = (uint32_t)(p - s);
            p--;
            break;
          default:
            goto err;
        }
        break;
      } /* switch->default */
    } /* switch */
  } while ((c = xml__parse_char(p, limit, 0)) != '\0' && (c = xml__parse_char(++p, limit, 0)) != '\0');
  
done:
  if (!complete || obj != &tmproot || parent || pos == begintag || pos == endtag || pos == beginattr)
    goto err;

  if (tmproot.val) {
    ((xml_t *)tmproot.val)->parent = NULL;
    ((xml_t *)tmproot.val)->next   = NULL;
  }
  
  doc->root = (xml_t *)tmproot.val;
  if (error) {
    error->status = XML_PARSE_OK;
    error->offset = (size_t)(limit - contents);
  }
  return doc;

exhausted:
  status = XML_PARSE_LIMIT;
  goto err;
oom:
  status = budget && budget->exhausted ? XML_PARSE_LIMIT : XML_PARSE_OUT_OF_MEMORY;
  goto err;
range:
  status = XML_PARSE_RANGE;
err:
  if (error) {
    error->status = status;
    error->offset = (size_t)((p ? p : limit) - contents);
  }
  xml_free(doc);
  return NULL;
}

XML_INLINE
xml_doc_t*
xml__parse_span(const char        * __restrict contents,
                size_t                        size,
                xml_options_t                 options,
                xml_parse_error_t            *error,
                const xml_parse_limits_t     *limits) {
  xml_parse_budget_t budget;
  const char        *nul;

  if (error) {
    error->status = XML_PARSE_INVALID_ARGUMENT;
    error->offset = 0;
  }
  if (!contents || size > PTRDIFF_MAX
      || (options & ~(XML_REVERSE | XML_PREFIXES | XML_READONLY | XML_PRESERVE_TEXT)))
    return NULL;

  if (limits && size > limits->max_input_bytes) {
    if (error)
      error->status = XML_PARSE_LIMIT;
    return NULL;
  }

  if (error)
    error->status = XML_PARSE_INVALID_DATA;

  if (!size)
    return NULL;

  /* vectorized scans below must not skip an embedded null byte. */
  if ((nul = (const char *)memchr(contents, '\0', size))) {
    if (error)
      error->offset = (size_t)(nul - contents);
    return NULL;
  }
  if (limits) {
    budget.bytes     = limits->max_memory_bytes;
    budget.tokens    = limits->max_tokens;
    budget.depth     = limits->max_depth;
    budget.exhausted = false;
  }
  return xml__parse(contents, options, contents + size, error, limits ? &budget : NULL);
}

/* isolate optional text semantics from the default parser hot loops. */
#if defined(_MSC_VER)
static __declspec(noinline)
#else
static __attribute__((noinline, unused))
#endif
xml_doc_t*
xml__parse_preserved(const char *contents, xml_options_t options,
                     size_t size, xml_parse_error_t *error, bool bounded) {
  options = (xml_options_t)(options | XML_PRESERVE_TEXT);
  if (bounded)
    return xml__parse_span(contents, size, options, error, NULL);

  return xml__parse(contents, options, NULL, NULL, NULL);
}

XML_INLINE
xml_doc_t*
xml_parse(const char * __restrict contents, xml_options_t options) {
  if (options & XML_PRESERVE_TEXT)
    return xml__parse_preserved(contents, options, 0, NULL, false);

  return xml__parse(contents, (xml_options_t)(options & ~XML_PRESERVE_TEXT), NULL, NULL, NULL);
}

XML_INLINE
xml_doc_t*
xml_parse_span(const char        * __restrict contents,
               size_t                        size,
               xml_options_t                 options,
               xml_parse_error_t            *error) {
  if (options & XML_PRESERVE_TEXT)
    return xml__parse_preserved(contents, options, size, error, true);

  return xml__parse_span(contents, size, (xml_options_t)(options & ~XML_PRESERVE_TEXT), error, NULL);
}

/* keep the optional text state out of the ordinary limited parser loop. */
#if defined(_MSC_VER)
static __declspec(noinline)
#else
static __attribute__((noinline, unused))
#endif
xml_doc_t*
xml__parse_limited_text(const char *contents, size_t size,
                        xml_options_t options, const xml_parse_limits_t *limits,
                        xml_parse_error_t *error) {
  return xml__parse_span(contents, size, (xml_options_t)(options | XML_PRESERVE_TEXT), error, limits);
}

XML_INLINE
xml_doc_t*
xml_parse_span_limited(const char               * __restrict contents,
                       size_t                               size,
                       xml_options_t                        options,
                       const xml_parse_limits_t            *limits,
                       xml_parse_error_t                   *error) {
  if (!limits)
    return xml_parse_span(contents, size, options, error);

  if (options & XML_PRESERVE_TEXT)
    return xml__parse_limited_text(contents, size, options, limits, error);

  return xml__parse_span(contents, size, (xml_options_t)(options & ~XML_PRESERVE_TEXT), error, limits);
}

#endif /* xml_impl_parse_h */
