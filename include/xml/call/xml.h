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

typedef struct xml_resolved_name_t {
  const char *local, *namespace_value;
  size_t      local_size, namespace_size;
} xml_resolved_name_t;

typedef enum xml_markup_kind_t {
  XML_MARKUP_COMMENT = 0,
  XML_MARKUP_PI,
  XML_MARKUP_DECLARATION
} xml_markup_kind_t;

typedef struct xml_markup_span_t {
  size_t            offset, size;
  xml_markup_kind_t kind;
} xml_markup_span_t;

typedef xml_parse_status_t (*xml_markup_visitor_t)(void *context, const xml_markup_span_t *span);

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
 * otherwise), PI targets (NCNames with XML_PREFIXES, Names otherwise),
 * UTF-8/XML characters and value references. namespace PI targets cannot use
 * a colon, even when that prefix is declared. PI data may still contain colons.
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

/* visit comments, processing instructions and the XML declaration in original
 * source order, including before/after the root. contents must be the unchanged
 * bytes of a complete document successfully parsed with xmlc_parse_span_checked
 * and XML_READONLY. this is an optional lexical projection, not another XML
 * validator. DTDs remain unsupported. an empty span enumerates nothing.
 *
 * offsets/sizes include complete delimiters and preserve raw spelling, quotes,
 * references and line endings. CDATA lookalikes are skipped. in checked XML,
 * raw '<' cannot occur in attribute values, so tag/attribute reparsing is not
 * needed. no nodes, source copies, allocations or retained state are introduced.
 * span is valid only during its callback; copy it to retain the record. bytes
 * still borrow contents. the caller owns any persistent index/storage.
 *
 * max_scan_bytes admits an input prefix; this function reads no bytes beyond
 * min(size, max_scan_bytes). a shorter prefix returns LIMIT at that boundary,
 * delivering only spans completed within it. this counts unique input bytes,
 * not instructions/time or callback work. callbacks are provisional until OK;
 * publish retained results only after complete success. visitor is required.
 * non-OK callback status is propagated unchanged at that span's starting offset.
 * callbacks must not change the source. error/output must not alias the source.
 *
 * success reports OK at size. invalid arguments report offset zero; defensive
 * malformed-markup checks may report INVALID_DATA on unvalidated input, but do
 * not establish complete XML validity. no parser entry/option/layout changes.
 */
XML_EXPORT
xml_parse_status_t
xmlc_visit_markup(const char           *contents,
                   size_t                size,
                   size_t                max_scan_bytes,
                   xml_markup_visitor_t  visitor,
                   void                 *context,
                   xml_parse_error_t    *error);

/* validate namespace 1.0 declarations and prefix bindings on a complete tree from
 * xmlc_parse_span_checked with XML_PREFIXES. the tree and borrowed input must
 * remain unchanged while this runs. this performs no allocation or recursion.
 *
 * checks local/inherited declarations, shadowing, implicit xml, reserved
 * prefix/URI pairs and the ban on empty prefixed bindings. URI values use the
 * existing attribute decoder. default bindings may be empty; URI reference
 * syntax is not checked and no percent decoding/case folding is performed.
 * expanded-name attribute uniqueness is a separate check. PI target syntax is
 * already checked before discard by checked XML_PREFIXES parsing; binding-only
 * success does not establish complete namespace-well-formed input.
 *
 * max_work counts each tree/ancestor step, each attribute visit plus its name
 * length, each resolved prefix length and raw URI bytes per decoding pass.
 * zero permits no work; SIZE_MAX is the largest cap. ancestor lookup can
 * revisit attributes; a finite cap bounds that work with XML_PARSE_LIMIT.
 * one root binding may reuse its boundness proof within this call; descendant
 * declarations are still checked and no selected URI is cached or returned.
 *
 * returns OK, INVALID_ARGUMENT, INVALID_DATA or LIMIT. optional invalid receives
 * NULL on success/argument failure, otherwise a borrowed source cursor at the
 * failed name/value or work step. callers with the source base can compute an
 * absolute offset. existing tokens, values, options and parser entries do not
 * change; this checks bindings only and retains no resolved-name index.
 */
XML_EXPORT
xml_parse_status_t
xmlc_validate_namespace_bindings(const xml_doc_t *document, size_t max_work, const char **invalid);

/* resolve an element name (attribute == NULL) or one of its attribute names on
 * a checked XML_PREFIXES tree after successful namespace binding validation.
 * attribute must belong to element; the complete tree/input must remain valid
 * and unchanged throughout. this does not repeat lexical/binding validation.
 *
 * local is the literal local-name span. namespace_value is the nearest selected
 * declaration's RAW attribute value, not its decoded URI. use XML_DECODE_ATTRIBUTE
 * before URI comparison. xml and xmlns return their implicit, plain URI values.
 * absent/empty default bindings and unprefixed ordinary attributes have a NULL
 * namespace_value and zero namespace_size. default namespaces apply to elements
 * only. namespace declarations themselves resolve to the xmlns namespace.
 *
 * spans borrow the input, except implicit URI strings with static lifetime.
 * no allocation, recursion, mutation or retained state occurs. resolving again
 * after a valid edit observes the current declaration; no binding proof/cache
 * is reused as a URI. max_work charges one query plus qualified-name bytes,
 * each visited ancestor and each inspected attribute plus its name bytes.
 * zero permits no work; SIZE_MAX is the largest cap. URI decoding has separate
 * costs and is not performed or charged here.
 *
 * name is required and reset to zero on failure. optional invalid is NULL on
 * success/argument failure, otherwise a borrowed source cursor at the failed
 * query/attribute step. an unbound queried prefix returns INVALID_DATA; quota
 * exhaustion returns LIMIT. outputs must not overlap the tree/input.
 */
XML_EXPORT
xml_parse_status_t
xmlc_resolve_name(const xml_t          *element,
                  const xml_attr_t     *attribute,
                  size_t                max_work,
                  xml_resolved_name_t  *name,
                  const char          **invalid);

/* check expanded attribute uniqueness after checked XML_PREFIXES parsing and
 * successful xmlc_validate_namespace_bindings. the tree/input must remain
 * unchanged throughout. raw duplicates were checked by the parser; unprefixed,
 * xml and xmlns attributes cannot alias ordinary bound prefixes in namespace
 * 1.0. only ordinary prefixed attributes need this additional check.
 *
 * local names are grouped before effective URI lookup. nearest declarations
 * select the URI, with normal attribute decoding and byte-exact equality; no
 * percent decoding, case folding or Unicode normalization. unique local names
 * need no URI lookup. copied entries are sorted without reordering tokens.
 *
 * max_work counts tree/ancestor steps, attribute visits plus name lengths,
 * key comparisons plus equal-length key bytes and raw URI bytes per inspection
 * or decoding pass. max_memory caps additional requested heap bytes, excluding
 * allocator metadata, fixed stack, borrowed input and existing document pages.
 * zero is a real cap; SIZE_MAX is the largest cap. up to eight candidates and
 * 128 decoded URI bytes use fixed stack. larger scratch is released before
 * return; nothing is retained. URI lookup can revisit ancestors within quota.
 *
 * optional invalid is NULL on success/argument failure, otherwise a borrowed
 * source cursor for the error or quota step. a duplicate reports the earliest
 * second occurrence within the first offending element visited. limit/range/
 * allocation failures may precede complete semantic validation. complete
 * namespace/document grammar qualification remains a separate gate.
 */
XML_EXPORT
xml_parse_status_t
xmlc_validate_expanded_attributes(const xml_doc_t  *document,
                                  size_t            max_work,
                                  size_t            max_memory,
                                  const char      **invalid);

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
