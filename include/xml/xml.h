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

  /*
   * retain all character data inside elements, including whitespace-only spans.
   * CDATA is an XML_STRING with cdata set. Values still borrow the input and
   * retain raw entity references and line endings; no decoding/normalization.
   * Comments and processing instructions are skipped without entering text.
   * Unsupported declarations (including DTDs) are rejected in this mode.
   * This is not full XML validation or a lossless markup serializer.
   */
  XML_PRESERVE_TEXT = 1 << 3,

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

typedef enum xml_parse_status_t {
  XML_PARSE_OK = 0,
  XML_PARSE_INVALID_ARGUMENT,
  XML_PARSE_INVALID_DATA,
  XML_PARSE_OUT_OF_MEMORY,
  XML_PARSE_RANGE,
  XML_PARSE_LIMIT
} xml_parse_status_t;

/* zero is a real cap; SIZE_MAX leaves an individual resource unrestricted. */
typedef struct xml_parse_limits_t {
  size_t max_input_bytes;
  size_t max_memory_bytes;
  size_t max_tokens;
  size_t max_depth;
} xml_parse_limits_t;

typedef struct xml_parse_error_t {
  xml_parse_status_t status;
  size_t             offset;
} xml_parse_error_t;

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
 *         NULL on detected parse/size/allocation failure; no partial root.
 */
XML_INLINE
xml_doc_t*
xml_parse(const char * __restrict contents, xml_options_t options);

/*!
 * @brief parse a borrowed XML byte span without a trailing null terminator
 *
 * size excludes any terminator; embedded null bytes are rejected. The caller
 * keeps contents alive until xml_free(). Unless XML_READONLY is set, contents
 * must be writable and may also be changed on failure, as with xml_parse().
 * Token lengths, grammar and options otherwise follow xml_parse(); this is
 * not a full XML conformance validator and does not impose resource budgets.
 *
 * error is optional. On success it reports XML_PARSE_OK and offset == size.
 * On failure it reports a status and a byte cursor in [0, size]; argument and
 * initial allocation errors use offset zero. No partial document is returned.
 * size must fit PTRDIFF_MAX, and options must contain only known option bits.
 */
XML_INLINE
xml_doc_t*
xml_parse_span(const char        * __restrict contents,
               size_t                        size,
               xml_options_t                 options,
               xml_parse_error_t            *error);

/*!
 * @brief parse a borrowed span with explicit resource caps
 *
 * Same options, ownership and grammar as xml_parse_span(). A NULL limits pointer
 * uses its unrestricted path. Otherwise all four inclusive caps apply; zero
 * permits none and SIZE_MAX leaves that resource unrestricted. Input size is
 * checked before scanning any bytes. Memory counts the document and complete
 * arena allocations, including page headers and unused page capacity, but not
 * borrowed input, allocator metadata or fixed parser stack. Pages retain their
 * normal size; a smaller cap fails rather than changing the allocation policy.
 * Tokens count elements, attributes and retained text/CDATA nodes. Element depth
 * starts at one for the root; siblings reuse the same depth allowance.
 *
 * A limit failure reports XML_PARSE_LIMIT, releases all owned memory and returns
 * NULL. Argument/initial input or memory cap failures use offset zero; later
 * failures report the current byte cursor. Input may be modified on failure
 * without XML_READONLY. Limits can precede syntax diagnostics. These resource
 * caps do not impose a wall-clock deadline or validate additional XML grammar.
 */
XML_INLINE
xml_doc_t*
xml_parse_span_limited(const char               * __restrict contents,
                       size_t                               size,
                       xml_options_t                        options,
                       const xml_parse_limits_t            *limits,
                       xml_parse_error_t                   *error);

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

XML_INLINE
xml_attr_t*
xmla_sz(const xml_t * __restrict object,
        const char  * __restrict name,
        size_t                   namesize);

XML_INLINE
xml_attr_t*
xmla_packed4(const xml_t * __restrict object,
             uint32_t                 packed,
             size_t                   namesize);

XML_INLINE
xml_attr_t*
xmla_packed8(const xml_t * __restrict object,
             uint64_t                 packed,
             size_t                   namesize);

XML_INLINE
xml_attr_t*
xmla_packed(const xml_t * __restrict object,
            uint64_t                 packed,
            size_t                   namesize);

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

XML_INLINE
xml_t*
xml_elem_sz(const xml_t * __restrict object,
            const char  * __restrict name,
            size_t                   namesize);

XML_INLINE
xml_t*
xml_elem_packed4(const xml_t * __restrict object,
                 uint32_t                 packed,
                 size_t                   namesize);

XML_INLINE
xml_t*
xml_elem_packed8(const xml_t * __restrict object,
                 uint64_t                 packed,
                 size_t                   namesize);

XML_INLINE
xml_t*
xml_elem_packed(const xml_t * __restrict object,
                uint64_t                 packed,
                size_t                   namesize);

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

XML_INLINE
xml_t*
xml_elem_next_sz(const xml_t * __restrict current,
                 const char  * __restrict name,
                 size_t                   namesize);

XML_INLINE
xml_t*
xml_elem_next_packed4(const xml_t * __restrict current,
                      uint32_t                 packed,
                      size_t                   namesize);

XML_INLINE
xml_t*
xml_elem_next_packed8(const xml_t * __restrict current,
                      uint64_t                 packed,
                      size_t                   namesize);

XML_INLINE
xml_t*
xml_elem_next_packed(const xml_t * __restrict current,
                     uint64_t                 packed,
                     size_t                   namesize);

#include "impl/impl_parse.h"
#include "impl/impl_common.h"
#include "objmap.h"

#endif /* xml_h */
