cmake_minimum_required(VERSION 3.5)

add_subdirectory(libpafe-emscripten)

# if (EMSCRIPTEN)
#   set(CMAKE_EXECUTABLE_SUFFIX ".html")
# endif()

# Build test binaries

set(LIBRARY_JS "${CMAKE_CURRENT_SOURCE_DIR}/libpafe-emscripten/src/webpafe/library.js")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 --js-library ${LIBRARY_JS}")

set(LIBPAFE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libpafe-emscripten/src/libpafe")
set(HTML_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/libpafe-emscripten/src/webpafe/shell.html")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/tests/pasori_test/" "${LIBPAFE_DIR}/src/")

add_executable(pasori_test "${LIBPAFE_DIR}/tests/pasori_test.c")
set(JS_FILENAME "pasori_test.js")
configure_file(${HTML_TEMPLATE} "${CMAKE_CURRENT_BINARY_DIR}/pasori_test.html")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/tests/pasori_test")
target_link_libraries(pasori_test pafe)

add_executable(felica_dump "${LIBPAFE_DIR}/tests/felica_dump.c")
set(JS_FILENAME "felica_dump.js")
configure_file(${HTML_TEMPLATE} "${CMAKE_CURRENT_BINARY_DIR}/felica_dump.html")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/tests/pasori_test")
target_link_libraries(felica_dump pafe)

configure_file(${LIBPAFE_DIR}/src/libpafe.h ${CMAKE_CURRENT_BINARY_DIR}/libpafe/libpafe.h COPYONLY)
set(JS_FILENAME "suica_dump.js")
configure_file(${HTML_TEMPLATE} "${CMAKE_CURRENT_BINARY_DIR}/suica_dump.html")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
add_executable(suica_dump "${CMAKE_CURRENT_SOURCE_DIR}/tests/suica-dump/main.c")
target_link_libraries(suica_dump pafe)
