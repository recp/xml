add_executable(xml-parse-test parse.c)
target_link_libraries(xml-parse-test PRIVATE xml)
add_test(NAME parser-corrections COMMAND xml-parse-test)

add_executable(xml-alloc-test alloc.c)
target_include_directories(xml-alloc-test PRIVATE "${PROJECT_SOURCE_DIR}/include")
add_test(NAME allocation-failures COMMAND xml-alloc-test)

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
  add_test(NAME decode-reference
           COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/decode_reference.py"
                   $<TARGET_FILE:xml-decode-test>)
  add_test(NAME name-reference
           COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/name_reference.py"
                   $<TARGET_FILE:xml-name-test>)
  set_tests_properties(name-reference PROPERTIES SKIP_RETURN_CODE 77)
endif()
