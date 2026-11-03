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
