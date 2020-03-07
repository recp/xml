/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_h
#define xml_h

#include "common.h"
#include "util.h"
#include "attrib.h"

typedef enum xml_options_t {
  XML_NONE     = 0,      /* use the default options (XML_DEFAULTS)           */
  XML_REVERSE  = 1 << 0, /* store members, array items as reverse order      */
  XML_PREFIXES = 1 << 1, /* separate prefixes and store them in xml->prefix  */

  /*
   * Don't touch to the content even temporarily, just parse it by creating 
   * tokens. You cannot use utilities in xml library to convert string to a 
   * number (strtol, strtof) because null terminator cannot be added. 
   * But you can duplicate string to another place or bring your own 
   * string-to-number converter. Since this library wants to be lightweight, 
   * it won't add this functionality. In the uture things may be changed.
   */ 
  XML_READONLY = 1 << 2,

  /* --------------------- DEFAULT OPTIONS ------------------------------------
   *
   * Option 1: NULL Terminator for Strings
   * --------------------------------------------------------------------------
   * Add null terminator for string values and attributes. You can also use
   * valsize to get string byte length. All string functions shall work for
   * xml->val member.
   *
   * NOTE: prefixes are not null terminated for now.
   *
   * This is disabled in READONLY mode.
   *
   * Option 2: separate prefixes from tag name
   * --------------------------------------------------------------------------
   * separate prefixes and store them in xml->prefix
   *
   */
  XML_DEFAULTS = XML_PREFIXES
} xml_options_t;

/*!
 * @brief parse xml string
 *
 * this function parses XML string and retuns a document which contains:
 *   - XML object
 *   - allocated memory
 * after XML is processed, the object must be freed with xml_free()
 *
 * this library doesn't alloc any memory for XM itsef and doesn't copy XML
 * contents into a data structure. It only allocs memory for tokens.
 * So don't free 'contents' parameter until you finished to process XML.
 *
 * Desired order:
 *  1. Read XML file
 *  2. Pass the contents to xml_parse()
 *  3. Process/Handle XML
 *  4. free XML document with xml_free()
 *  5. free `contents`
 *
 * @param[in] contents XML string
 * @param[in] options  options use XML_DEFAULTS or XML_NONE for default
 *
 * @return xml document which contains xml object as root object
 */
XML_INLINE
xml_doc_t*
xml_parse(const char * __restrict contents, xml_options_t options);

/*!
 * @brief frees xml document and its allocated memory
 */
XML_INLINE
void
xml_free(xml_doc_t * __restrict jsondoc);

/*!
* @brief get an attribute by name for given XML element
*
* @param[in] object   xml element
* @param[in] name     attribute name to find
* @return attribute (xml_attr_t)
*/
XML_INLINE
xml_attr_t*
xmla(const xml_t * __restrict object, const char * __restrict name);

/*!
* @brief get an child element by name for given XML element
*
* @param[in] object   xml element
* @param[in] name     element name to find
* @return element (xml_t)
*/
XML_INLINE
xml_t*
xml_elem(const xml_t * __restrict object, const char * __restrict name);

/*!
* @brief get an child element by name for given XML element
*
* @param[in] current  current xml element
* @param[in] name     element name to find
* @return element (xml_t)
*/
XML_INLINE
xml_t*
xml_elem_next(const xml_t * __restrict current, const char * __restrict name);

#include "impl/impl_parse.h"
#include "impl/impl_common.h"
#include "objmap.h"

#endif /* xml_h */
