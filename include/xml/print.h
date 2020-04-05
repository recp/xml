/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_print_h
#define xml_print_h

#include "common.h"
#include "xml.h"
#include "util.h"
#include <stdio.h>
#include <ctype.h>

/*!
 * @brief print xml with option
 *
 * @param[in] ostream stream to print
 * @param[in] xml     xml object
 * @param[in] opt     0: uglify, 1: pretty, 2: human
 */
XML_INLINE
void
xml_print_ex(FILE  * __restrict ostream,
             xml_t * __restrict xml,
             int                opt) {
  const xml_t      *parent;
  const xml_attr_t *a;
  int               pad, i, snode;

  pad    = snode = 0;
  parent = NULL;

  while (xml) {
    if (!snode && opt > 0 && xml->type == XML_ELEMENT) {
      for (i = 0; i < pad; i++)
        fprintf(ostream, "\t");
    }

    snode = 0;
    switch (xml->type) {
      case XML_ELEMENT: {
        if (!xml->prefix)
          fprintf(ostream, "<%.*s", xml->tagsize, xml->tag);
        else
          fprintf(ostream, "<%.*s:%.*s",
                  xml->prefixsize,
                  xml->prefix,
                  xml->tagsize,
                  xml->tag);

        if ((a = xml->attr)) {
          do {
              fprintf(ostream, " %c%.*s%c=%c%.*s%c",
                      a->namequote,
                      a->namesize,
                      a->name,
                      a->namequote,
                      a->valquote,
                      a->valsize,
                      a->val,
                      a->valquote);
          } while ((a = a->next));
        }

        if (xml->val) {
          fprintf(ostream, ">");

          if (xml_xml(xml)->type == XML_ELEMENT)
            pad++;

          /* parent = xml; */
          xml = xml->val;
          
          if (opt > 0 && xml->type == XML_ELEMENT)
            fprintf(ostream, "\n");
          continue;
        } else {
          if (!xml->next)
            pad--;

          if (xml->parent)
            fprintf(ostream, " />");
          else {
            if (!xml->prefix)
              fprintf(ostream, "></%.*s>", xml->tagsize, xml->tag);
            else
              fprintf(ostream, "></%.*s:%.*s>",
                      xml->prefixsize,
                      xml->prefix,
                      xml->tagsize,
                      xml->tag);
          }
          
          if (opt > 0)
            fprintf(ostream, "\n");
        }
        break;
      }
      case XML_STRING:
        fprintf(ostream, "%.*s", xml->valsize, xml->val);
        snode = 1;
        break;
      default:
        break;
    }

    if (xml->next) {
      xml = xml->next;
    } else if ((parent = xml->parent)) {
      snode = 0;
      do {
        if (opt > 0 && (!xml || (xml && xml->type == XML_ELEMENT))) {
          for (i = 0; i < pad; i++)
            fprintf(ostream, "\t");
        }

        if (parent->type == XML_ELEMENT) {
          if (!parent->prefix)
            fprintf(ostream, "</%.*s>", parent->tagsize, parent->tag);
          else
            fprintf(ostream, "</%.*s:%.*s>",
                    parent->prefixsize,
                    parent->prefix,
                    parent->tagsize,
                    parent->tag);
        }

        if (opt > 0)
          fprintf(ostream, "\n");

        xml    = parent->next;
        parent = parent->parent;
        
        if (!xml)
          --pad;
      } while (!xml && parent);
    } else {
      break;
    }
  } /* while (xml)  */
}

/*!
 * @brief print xml
 *
 * @param[in] ostream stream to print
 * @param[in] xml     xml object
 */
XML_INLINE
void
xml_print_pretty(FILE  * __restrict ostream,
                 xml_t * __restrict xml) {
  xml_print_ex(ostream, xml, 1);
}

/*!
 * @brief print xml for humans
 *
 * @param[in] ostream stream to print
 * @param[in] xml     xml object
 */
XML_INLINE
void
xml_print_human(FILE  * __restrict ostream,
                xml_t * __restrict xml) {
  fprintf(ostream, "xml ( %p ):\n\n", (void *)xml);
  xml_print_ex(ostream, xml, 2);
  fprintf(ostream, "\n");
}

/*!
 * @brief print xml
 *
 * @param[in] ostream stream to print
 * @param[in] xml     xml object
 */
XML_INLINE
void
xml_print_uglify(FILE  * __restrict ostream,
                 xml_t * __restrict xml) {
  xml_print_ex(ostream, xml, 0);
}

#endif /* xml_print_h */
