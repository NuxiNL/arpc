cmake_minimum_required(VERSION 3.8.0)

project(arpc)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(contrib/googletest-release-1.8.0/googletest)

add_custom_command(OUTPUT arpc_protocol.h
	COMMAND ${CMAKE_SOURCE_DIR}/scripts/aprotoc.py <${CMAKE_SOURCE_DIR}/src/arpc_protocol.proto >${CMAKE_BINARY_DIR}/arpc_protocol.h
	DEPENDS ${CMAKE_SOURCE_DIR}/src/arpc_protocol.proto
)
add_custom_command(OUTPUT server_test_proto.h
	COMMAND ${CMAKE_SOURCE_DIR}/scripts/aprotoc.py <${CMAKE_SOURCE_DIR}/src/server_test_proto.proto >${CMAKE_BINARY_DIR}/server_test_proto.h
	DEPENDS ${CMAKE_SOURCE_DIR}/src/server_test_proto.proto
)

include_directories(${CMAKE_BINARY_DIR} ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_library(arpc
  arpc_protocol.h
  include/arpc++/arpc++.h
  src/argdata_builder.cc
  src/argdata_parser.cc
  src/channel.cc
  src/server.cc
  src/server_context.cc
  src/status_code.cc
)
target_link_libraries(arpc PUBLIC argdata)
target_include_directories(arpc PUBLIC include)

add_executable(arpc_tests
  server_test_proto.h
  src/server_test.cc
)
target_link_libraries(arpc_tests arpc gtest_main)

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
