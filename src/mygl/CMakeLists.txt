add_library(mygl STATIC
  mygl.cpp)
target_include_directories(mygl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(mygl::mygl ALIAS mygl)