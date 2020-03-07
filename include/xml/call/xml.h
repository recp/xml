/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef xml_call_xml_h
#define xml_call_xml_h
#ifdef __cplusplus
extern "C" {
#endif

#include "../common.h"
#include "../xml.h"

/*!
 * @brief parse xml string
 *
 * this function parses XML string and retuns a document which contains:
 *   - XML object
 *   - allocated memory
 * after XML is processed, the object must be freed with xml_free()
 *
 * this library doesn't alloc any memory for XML itsef and doesn't copy XML
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
 * @return xml document which contains xml object as root object
 */
XML_EXPORT
xml_doc_t*
xmlc_parse(const char * __restrict contents, xml_options_t options);

/*!
 * @brief frees xml document and its allocated memory
 */
XML_EXPORT
void
xmlc_free(xml_doc_t * __restrict xmldoc);

#ifdef __cplusplus
}
#endif
#endif /* xml_call_xml_h */
