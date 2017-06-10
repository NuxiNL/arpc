cmake_minimum_required(VERSION 3.8.0)

project(arpc)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(arpc
  include/arpc++/arpc++.h
  src/server.cc
)
target_include_directories(arpc PUBLIC include)

install(FILES scripts/aprotoc.py
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  DESTINATION bin
  RENAME aprotoc)

install(DIRECTORY include/arpc++
  DESTINATION include
)

install(TARGETS arpc EXPORT arpc
  #PUBLIC_HEADERS DESTINATION include
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
