add_executable(xml-parse-test parse.c)
target_link_libraries(xml-parse-test PRIVATE xml)
add_test(NAME parser-corrections COMMAND xml-parse-test)

add_executable(xml-alloc-test alloc.c)
target_include_directories(xml-alloc-test PRIVATE "${PROJECT_SOURCE_DIR}/include")
add_test(NAME allocation-failures COMMAND xml-alloc-test)

add_executable(xml-checked-test checked.c)
target_link_libraries(xml-checked-test PRIVATE xml)
add_test(NAME checked-input COMMAND xml-checked-test)

add_executable(xml-document-test document.c)
target_link_libraries(xml-document-test PRIVATE xml)
add_test(NAME document-grammar COMMAND xml-document-test)

add_executable(xml-processing-test processing.c)
target_link_libraries(xml-processing-test PRIVATE xml)
add_test(NAME processing-targets COMMAND xml-processing-test)

add_executable(xml-namespace-test namespace.c)
target_link_libraries(xml-namespace-test PRIVATE xml)
add_test(NAME namespace-bindings COMMAND xml-namespace-test)

add_executable(xml-namespace-resolve-test namespace_resolve.c)
target_link_libraries(xml-namespace-resolve-test PRIVATE xml)
add_test(NAME namespace-resolution COMMAND xml-namespace-resolve-test)

add_executable(xml-namespace-attributes-test namespace_attributes.c)
target_link_libraries(xml-namespace-attributes-test PRIVATE xml)
add_test(NAME namespace-attributes COMMAND xml-namespace-attributes-test)

add_executable(xml-namespace-attributes-alloc-test namespace_attributes_alloc.c)
target_link_libraries(xml-namespace-attributes-alloc-test PRIVATE xml)
add_test(NAME namespace-attribute-allocations COMMAND xml-namespace-attributes-alloc-test)

add_executable(xml-checked-alloc-test alloc.c)
target_include_directories(xml-checked-alloc-test PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_compile_definitions(xml-checked-alloc-test PRIVATE XML_TEST_CHECKED)
target_link_libraries(xml-checked-alloc-test PRIVATE xml)
add_test(NAME checked-allocation-failures COMMAND xml-checked-alloc-test)

add_executable(xml-span-test span.c)
target_link_libraries(xml-span-test PRIVATE xml)
add_test(NAME bounded-input COMMAND xml-span-test)

add_executable(xml-text-test text.c)
target_link_libraries(xml-text-test PRIVATE xml)
add_test(NAME text-preservation COMMAND xml-text-test)

add_executable(xml-limits-test limits.c)
target_link_libraries(xml-limits-test PRIVATE xml)
add_test(NAME resource-limits COMMAND xml-limits-test)

add_executable(xml-decode-test decode.c)
target_link_libraries(xml-decode-test PRIVATE xml)
add_test(NAME value-decoding COMMAND xml-decode-test)

add_executable(xml-name-test name.c)
target_link_libraries(xml-name-test PRIVATE xml)
add_test(NAME name-validation COMMAND xml-name-test)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
  add_test(NAME document-reference
           COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/document_reference.py"
                   $<TARGET_FILE:xml-document-test>)
  add_test(NAME decode-reference
           COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/decode_reference.py"
                   $<TARGET_FILE:xml-decode-test>)
  add_test(NAME name-reference
           COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/name_reference.py"
                   $<TARGET_FILE:xml-name-test>)
  set_tests_properties(name-reference PROPERTIES SKIP_RETURN_CODE 77)
endif()
