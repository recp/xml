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
  beginattr = 5
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
  return limit ? (char *)memchr(p, c, (size_t)(limit - p)) : strchr(p, c);
}

XML_INLINE
xml_doc_t*
xml__parse(const char        * __restrict contents,
           xml_options_t                 options,
           const char                   *limit,
           xml_parse_error_t            *error) {
  xml_t        tmproot;
  xml_doc_t   *doc;
  xml_t       *obj, *parent, *val;
  xml_attr_t  *attr;
  const char  *tag, *s;
  xml_position pos;

  xml_parse_status_t status = XML_PARSE_INVALID_DATA;

  uint32_t     remaining;
  char        *p, *end, c, quote;
  bool         foundQuote, reverse, sepPrefixes, readonly, complete = false;
  
  if (!contents || (c = *contents) == '\0')
    return NULL;
  
  if (!(doc = calloc(1, sizeof(*doc)))) {
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

  doc->reverse     = reverse;
  doc->readonly    = readonly;
  doc->sepPrefixes = sepPrefixes;

  tmproot.readonly = readonly;
  tmproot.reverse  = reverse;
  
  ((xml_mem_t *)doc->memroot)->capacity = XML_MEM_PAGE;
  
  do {
  again:
    /* child */
    switch (c) {
        /* trim */
      case ' ':
      case '\r':
      case '\n':
      case '\t':
        do {
          c = xml__parse_char(++p, limit, 0);
        } while (c == ' ' || c == '\r' || c == '\n' || c == '\t');

        if (c == '\0')
          goto done;

        goto again;
      case '<': { /* TODO: what if we get << or <<!--  ->TAG>... here? */
        if (pos == begintag)
          goto err;

        pos = begintag;
        
        if (!readonly) {
          *p = '\0';
          if ((c = xml__parse_char(++p, limit, 0)) != '\0')
            goto again;;
        }
        break;
      }
      case '!': { /* Comment + CTADA */
        char c1, c2;
        
        /* comments, skip for now. */
        if (xml__parse_char(p, limit, 1) == '-' && xml__parse_char(p, limit, 2) == '-') {
          p  = p + 2;
          c  = xml__parse_char(p, limit, 0);
          c2 = '\0';

          do {
            if (c == '\0')
              goto err;

            c1 = c2;
            c2 = c;
            c  = xml__parse_char(++p, limit, 0);
            
          } while (!(c1 == '-' && c2 == '-' && c == '>'));

          pos = beginel;
        }
        
        /* CDATA or similar data, skip for now. */
        else if (xml__parse_char(p, limit, 1) == '[') {
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
        }
        break;
      }
      case '?':
        /* skip XML header e.g. version line */
        if (pos == begintag) {
          /* skip to value  */
          while (c != '>') {
            if (c == '\0')
              goto err;
            c = xml__parse_char(++p, limit, 0);
          }

          pos = 0;
        }
        break;
      case '/':
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
        if (pos == endtag) {
          /* switch parent back */
          if (!parent)
            goto err;
          
          if (!reverse) {
            obj->val  = obj->next;
            obj->next = NULL;
          }
          
          obj    = parent;
          parent = parent->parent;
          pos    = endel;
          complete = parent == NULL;
        } else {
          pos = beginel;
        }
        
        if (!readonly) {
          *p = '\0';
          if ((c = xml__parse_char(++p, limit, 0)) != '\0')
            goto again;;
        }
        break;
      default: {
        switch (pos) {
          case begintag:
            if (complete)
              goto err;

            /* switch parent */
            parent        = obj;
            obj           = xml__impl_calloc(doc, sizeof(xml_t));
            if (!obj)
              goto oom;

            obj->type     = XML_ELEMENT;
            obj->readonly = readonly;
            obj->reverse  = reverse;
            
            /* parent must not be NULL */
            
            if (!reverse) {
              if (!parent->next)
                parent->next = obj;
              else
                xml_xml(parent)->next = obj;
            } else {
              obj->next = parent->val;
            }
            
            parent->val = obj;
            obj->parent = parent;
            obj->tag    = p;

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
            
            if (c == '>') {
              pos = beginel;
            } else if (c == '/') {
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
            attr = xml__impl_calloc(doc, sizeof(xml_attr_t));
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
            if (obj == &tmproot)
              goto err;

            val           = xml__impl_calloc(doc, sizeof(xml_t));
            if (!val)
              goto oom;
            val->type     = XML_STRING;
            
            val->readonly = readonly;
            val->reverse  = reverse;
            
            /* parent must not be NULL */

            if (!reverse) {
              if (!obj->next)
                obj->next = val;
               else
                xml_xml(obj)->next = val;
            } else {
              val->next = obj->val;
            }

            obj->val    = val;
            val->parent = obj;
            val->val    = (void *)s;

            p = xml__parse_find(p, '<', limit);
            if (!p)
              goto err;
            
            if ((size_t)(p - (char *)s) > UINT32_MAX)
              goto range;

            val->valsize = (uint32_t)(p - (char *)s);
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
  
  doc->root = tmproot.val;
  if (error) {
    error->status = XML_PARSE_OK;
    error->offset = (size_t)(limit - contents);
  }
  return doc;

oom:
  status = XML_PARSE_OUT_OF_MEMORY;
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
xml_parse(const char * __restrict contents, xml_options_t options) {
  return xml__parse(contents, options, NULL, NULL);
}

XML_INLINE
xml_doc_t*
xml_parse_span(const char        * __restrict contents,
               size_t                        size,
               xml_options_t                 options,
               xml_parse_error_t            *error) {
  const char *nul;

  if (error) {
    error->status = XML_PARSE_INVALID_ARGUMENT;
    error->offset = 0;
  }
  if (!contents || size > PTRDIFF_MAX
      || (options & ~(XML_REVERSE | XML_PREFIXES | XML_READONLY)))
    return NULL;

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
  return xml__parse(contents, options, contents + size, error);
}

#endif /* xml_impl_parse_h */
