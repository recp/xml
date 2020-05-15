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

XML_INLINE
xml_doc_t*
xml_parse(const char * __restrict contents, xml_options_t options) {
  xml_doc_t   *doc;
  xml_t       *obj, *parent, *val;
  xml_attr_t  *attr;
  const char  *tag, *s;
  xml_t        tmproot;
  xml_position pos;
  char        *p, *end, c, quote;
  bool         foundQuote, reverse, sepPrefixes, readonly;
  
  if (!contents || (c = *contents) == '\0')
    return NULL;
  
  doc            = calloc(1, sizeof(*doc));
  doc->memroot   = calloc(1, sizeof(xml_mem_t) + XML_MEM_PAGE);
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
        break;
      case '<': { /* TODO: what if we get << or <<!--  ->TAG>... here? */
        if (pos == begintag)
          goto err;

        pos = begintag;
        
        if (!readonly) {
          *p = '\0';
          if ((c = *++p) != '\0')
            goto again;;
        }
        break;
      }
      case '!': { /* Comment + CTADA */
        char c1, c2;
        
        /* comments, skip for now. */
        if (p[1] != '\0' && p[1] == '-' && p[2] == '-') {
          p  = p + 2;
          c  = *p;
          c2 = '\0';

          do {
            if (c == '\0')
              goto err;

            c1 = c2;
            c2 = c;
            c  = *++p;
            
          } while (!(c1 == '-' && c2 == '-' && c == '>'));

          pos = beginel;
        }
        
        /* CDATA or similar data, skip for now. */
        else if (p[1] != '\0' && p[1] == '[') {
          p  = p + 2;
          c  = *p;
          c2 = '\0';
          
          do {
            if (c == '\0')
              goto err;

            c1 = c2;
            c2 = c;
            c  = *++p;

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
            c = *++p;
          }

          ++p;
          pos = 0;
        }
        break;
      case '/':
        switch (pos) {
          case begintag:
            /* end xml tag */
            tag = obj->tag;
            c   = *++p;
            
            do {
              if (c == '\0')
                goto err;
              
              if (!tag || c != *tag++)
                goto err;
              
              c = *++p;
            } while (c != ' ' && c != '>');
            
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
        } else {
          pos = beginel;
        }
        
        if (!readonly) {
          *p = '\0';
          if ((c = *++p) != '\0')
            goto again;;
        }
        break;
      default: {
        switch (pos) {
          case begintag:
            /* switch parent */
            parent        = obj;
            obj           = xml__impl_calloc(doc, sizeof(xml_t));
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
                  obj->prefixsize = (uint32_t)(p - obj->prefix);
                  obj->tag        = p + 1;
                }
              }

              c = *++p;
            } while (c != ' ' && c != '/' && c != '>');
            
            obj->tagsize = (int)(p - obj->tag);
            
            if (c == '>') {
              pos = beginel;
            } else if (c == '/') {
              pos = endtag;
              
              if (!readonly) {
                *p = '\0';
                if ((c = *++p) != '\0')
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
            
            s = p + 1;
            p--;
            break;
          case beginattr:
            attr = xml__impl_calloc(doc, sizeof(xml_attr_t));
            
            /* attrib key */
            if ((foundQuote = (c == '"' || c == '\'' || c == '`'))) {
              quote            = c;
              attr->namequote  = c;
              c                = *++p;
            }

            attr->name = end = p;
            
            if (foundQuote) {
              while (c != quote) {
                if (c == '\0')
                  goto err;
                
                /* espace */
                if (c != '\\') {
                  if (c != ' ' && c != '\r' && c != '\n' && c != '\t')
                    end = p + 1;
                } else {
                  /* c = *++p; */
                  ++p;
                }
                
                c = *++p;
              }
              
              /* skip trailing quote */
              c = *++p;
            } else {
              while (c != '=') {
                if (c == '\0')
                  goto err;
                
                if (c != ' ' && c != '\r'  && c != '\n' && c != '\t')
                  end = p + 1;
                
                c = *++p;
              }
            }
            
            attr->namesize = (int)(end - attr->name);
            if (!readonly)
              *end = '\0';

            if (attr->name == NULL || c  == '\0')
              goto err;
            
            /* attrib value */
            
            /* skip to value  */
            while (c != '=') {
              if (c == '\0')
                goto err;
              c = *++p;
            }
            
            /* skip trailing equal */
            c = *++p;

            if ((foundQuote = (c == '"' || c == '\'' || c == '`'))) {
              quote          = c;
              attr->valquote = c;
              c              = *++p;
            }
            
            attr->val = end = p;
            
            if (foundQuote) {
              while (c != quote) {
                if (c == '\0')
                  goto err;
                
                /* espace */
                if (c != '\\') {
                  if (c != ' ' && c != '\r' && c != '\n' && c != '\t')
                    end = p + 1;
                } else {
                  /* c = *++p; */
                  ++p;
                }
                
                c = *++p;
              }
              
              /* skip trailing quote */
              /* c = *++p; */
              ++p;
            } else {
              while (c != '>' && c != '/') {
                if (c == '\0')
                  goto err;
                
                if (c != ' ' && c != '\r' && c != '\n' && c != '\t')
                  end = p + 1;
                
                c = *++p;
              }
            }
            
            attr->valsize = (int)(end - attr->val);
            if (!readonly)
              *end = '\0';

            c          = *p;
            attr->next = obj->attr;
            obj->attr  = attr;
            attr       = NULL;
            s          = p + 1;
            
            goto again;
          case beginel:
          case endel:
            val           = xml__impl_calloc(doc, sizeof(xml_t));
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

            while (c != '<') {
              if (c == '\0')
                goto err;
              c = *++p;
            }
            
            val->valsize = (int)(p - (char *)s);
            p--;
            break;
          default:
            break;
        }
        break;
      } /* switch->default */
    } /* switch */
  } while ((c = *p) != '\0' && (c = *++p) != '\0');
  
err:
  
  if (attr)
    memset(attr, 0, sizeof(xml_attr_t));

  if (tmproot.val) {
    ((xml_t *)tmproot.val)->parent = NULL;
    ((xml_t *)tmproot.val)->next   = NULL;
  }
  
  doc->root = tmproot.val;
  return doc;
}

#endif /* xml_impl_parse_h */
