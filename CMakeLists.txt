cmake_minimum_required(VERSION 3.8.0)

project(arpc)

option(BUILD_SHARED_LIBS "Build as shared library" ON)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build test programs, using Google Test" ON)

add_custom_command(OUTPUT arpc_protocol.ad.h
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/aprotoc.py <${CMAKE_SOURCE_DIR}/src/arpc_protocol.proto >${CMAKE_BINARY_DIR}/arpc_protocol.ad.h
  DEPENDS ${CMAKE_SOURCE_DIR}/src/arpc_protocol.proto
)

include_directories(${CMAKE_BINARY_DIR})

add_library(arpc
  arpc_protocol.ad.h
  include/arpc++/arpc++.h
  src/argdata_builder.cc
  src/argdata_parser.cc
  src/channel.cc
  src/client_reader_impl.cc
  src/client_writer_impl.cc
  src/server.cc
  src/server_reader_impl.cc
  src/server_writer_impl.cc
  src/status.cc
  src/status_code.cc
)
target_link_libraries(arpc PUBLIC argdata)
target_include_directories(arpc PUBLIC include)
set_property(TARGET arpc PROPERTY VERSION "1")

install(FILES scripts/aprotoc.py
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  DESTINATION bin
  RENAME aprotoc)

install(DIRECTORY include/arpc++
  DESTINATION include
)

install(TARGETS arpc EXPORT arpc
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

if(BUILD_TESTS)
  add_subdirectory(contrib/googletest-release-1.8.0/googletest EXCLUDE_FROM_ALL)

  add_custom_command(OUTPUT server_test_proto.ad.h
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/aprotoc.py <${CMAKE_SOURCE_DIR}/src/server_test_proto.proto >${CMAKE_BINARY_DIR}/server_test_proto.ad.h
    DEPENDS ${CMAKE_SOURCE_DIR}/src/server_test_proto.proto
  )

  include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

  add_executable(arpc_tests
    server_test_proto.ad.h
    src/server_test.cc
  )
  target_link_libraries(arpc_tests arpc gtest_main)
endif(BUILD_TESTS)
