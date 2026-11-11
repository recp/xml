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

/* same ownership, bounds and error contract as xml_parse_span(). */
XML_EXPORT
xml_doc_t*
xmlc_parse_span(const char        * __restrict contents,
                size_t                        size,
                xml_options_t                 options,
                xml_parse_error_t            *error);

/* same resource caps and failure contract as xml_parse_span_limited(). */
XML_EXPORT
xml_doc_t*
xmlc_parse_span_limited(const char               * __restrict contents,
                        size_t                               size,
                        xml_options_t                        options,
                        const xml_parse_limits_t            *limits,
                        xml_parse_error_t                   *error);

/* bounded parsing with lexical checks and the normal resource caps.
 * retains raw text/CDATA as with XML_PRESERVE_TEXT. input stays borrowed.
 * validates element and attribute names (QNames with XML_PREFIXES, Names
 * otherwise), PI target names, UTF-8/XML characters and value references.
 * attribute values require single or double quotes and separators.
 * leading tag whitespace and spaced self-closing delimiters are rejected.
 * markup cannot interrupt an unfinished start tag, and closing tag names
 * permit only optional XML whitespace before their final delimiter.
 * a leading UTF-8 BOM is accepted and offsets include it.
 * an XML declaration must appear first (after an optional BOM), with ordered
 * version, optional encoding and optional standalone fields. version 1.x is
 * processed with XML 1.0 rules. only UTF-8 encoding declarations are supported;
 * other encodings return XML_PARSE_INVALID_DATA at the value. standalone is
 * yes/no. other case variants of the reserved PI target xml are rejected.
 *
 * comments/PI remain skipped, values retain raw spelling and no entities are
 * decoded into the tree. repeated attribute names are rejected at the second
 * source occurrence. lists through 32 attributes use fixed stack storage.
 * larger lists temporarily allocate one pointer per attribute, counted
 * against the remaining memory cap. the buffer is released before parsing
 * content or reporting an error. token order and retained memory are preserved.
 * namespace bindings, expanded-name duplicates, complete document grammar
 * and meaningful markup retention remain separate. DTDs are unsupported.
 * failures return no document and report the offending source byte. limit
 * and allocation behavior follows xmlc_parse_span_limited. this compiled
 * entry reuses the parser state machine and existing lexical validators.
 */
XML_EXPORT
xml_doc_t*
xmlc_parse_span_checked(const char               * __restrict contents,
                        size_t                               size,
                        xml_options_t                        options,
                        const xml_parse_limits_t            *limits,
                        xml_parse_error_t                   *error);

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
