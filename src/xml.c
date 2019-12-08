/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../include/xml/xml.h"
#include "../include/xml/impl/impl_mem.h"
#include "../include/xml/print.h"
#include <assert.h>

typedef enum xml_position {
  unknown  = 0,
  begintag = 1,
  endtag,
  beginel,
  endel,
  beginattr
} xml_position;

XML_EXPORT
xml_doc_t*
xml_parse(const char * __restrict contents, bool reverse) {
  xml_doc_t   *doc;
  xml_t       *obj, *parent, *val;
  xml_attr_t  *attr;
  const char  *tag, *p, *end, *s;
  xml_t        tmproot;
  xml_position pos;
  char         c, quote;
  bool         foundQuote;
  
  if (!contents || (c = *contents) == '\0')
    return NULL;
  
  doc            = calloc(1, sizeof(*doc));
  doc->memroot   = calloc(1, sizeof(xml_mem_t) + XML_MEM_PAGE);
  p              = contents;

  memset(&tmproot, 0, sizeof(tmproot));
  tmproot.type   = XML_ELEMENT;

  attr           = NULL;
  tag            = NULL;
  parent         = &tmproot;
  obj            = parent;
  pos            = unknown;
  quote          = '"';

  ((xml_mem_t *)doc->memroot)->capacity = XML_MEM_PAGE;

  /* TODO: */
  /* skip XML header e.g. version line */
  
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
      case '<': {
        if (pos == begintag)
          goto err;

        pos = begintag;
        break;
      }
      case '/':
        switch (pos) {
          case begintag:
            /* end xml tag */
            tag = obj->tag;
            c   = *++p;
            
            do {
              if (c == '\0')
                goto err;
              
              if (c != *tag++)
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
            obj->val = obj->next;
            obj->next  = NULL;
          }
          
          obj              = parent;
          parent           = parent->parent;
          pos              = endel;
        } else {
          pos = beginel;
        }
     
        break;
      default: {
        switch (pos) {
          case begintag:
            /* switch parent */
            parent    = obj;
            obj       = xml__impl_calloc(doc, sizeof(xml_t));
            obj->type = XML_ELEMENT;
            
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
            obj->tag = p;
            
            do {
              if (c == '\0')
                goto err;
              
              c = *++p;
            } while (c != ' ' && c != '>');
            
            obj->tagsize = (int)(p - obj->tag);
            
            if (c == '>')
              pos = beginel;
            else
              pos = beginattr;
            
            /*
             TODO: obly opens if error is handled
             if (c  == '\0')
             goto err;
             */
            
            s = p + 1;
            p--;
            break;
          case beginattr:
            attr = xml__impl_calloc(doc, sizeof(xml_t));
            
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
                  c = *++p;
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
            c             = *p;
            
            attr->next = obj->attr;
            obj->attr  = attr;
            attr       = NULL;
            s          = p + 1;
            
            goto again;
          case beginel:
          case endel:
            val       = xml__impl_calloc(doc, sizeof(xml_t));
            val->type = XML_STRING;
            
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
            val->val    = s;

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
    free(attr);

  if (tmproot.val) {
    ((xml_t *)tmproot.val)->parent = NULL;
    assert(((xml_t *)tmproot.val)->next == NULL);
  }
  
  doc->root = tmproot.val;
  return doc;
}
