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

#include <stdlib.h>
#include <string.h>

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
 * @param[in] reverse  store members, array items as reverse order
 * @return xml document which contains xml object as root object
 */
XML_EXPORT
xml_doc_t*
xml_parse(const char * __restrict contents, bool reverse);

/*!
 * @brief frees xml document and its allocated memory
 */
XML_INLINE
void
xml_free(xml_doc_t * __restrict jsondoc);

#endif /* xml_h */
