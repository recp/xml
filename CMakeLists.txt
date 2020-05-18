cmake_minimum_required(VERSION 3.11...3.16)
project(xml VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(DEFAULT_BUILD_TYPE "Release")

set(XML_BUILD)
option(XML_SHARED "Shared build" ON)
option(XML_STATIC "Static build" OFF)
option(XML_USE_C99 "" OFF)

if(NOT XML_STATIC AND XML_SHARED)
  set(XML_BUILD SHARED)
else(XML_STATIC)
  set(XML_BUILD STATIC)
endif()

if(XML_USE_C99)
  set(CMAKE_C_STANDARD 99)
endif()

if(MSVC)
	add_definitions(-D_WINDOWS -D_USRDLL -DXML_EXPORTS -DXML_DLL)
	add_compile_options(/W3 /Ox /Gy /Oi /TC)
else()
  add_compile_options(-Wall -Werror -O3 -fstrict-aliasing)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

include(GNUInstallDirs)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Target Start
add_library(${PROJECT_NAME} ${XML_BUILD} src/xml.c)

set_target_properties(${PROJECT_NAME} PROPERTIES 
                             VERSION ${PROJECT_VERSION}
                           SOVERSION ${PROJECT_VERSION_MAJOR})

target_include_directories(${PROJECT_NAME} PRIVATE include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Install 
install(TARGETS ${PROJECT_NAME}
        EXPORT  ${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        PATTERN ".*" EXCLUDE)

# Test Configuration
if(XML_USE_TEST)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()
