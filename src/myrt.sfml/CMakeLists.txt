add_library(myrt.sfml STATIC
  "include/myrt/sfml/sfml.hpp"
  sfml.cpp 
  "include/myrt/sfml/gl.hpp"
  gl.cpp "utils.cpp" "include/myrt/sfml/utils.hpp")

target_include_directories(myrt.sfml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(myrt.sfml PUBLIC cxx_std_23)

find_package(SFML 2 COMPONENTS graphics audio main network system window REQUIRED)
target_link_libraries(myrt.sfml PUBLIC Freetype OpenAL OpenGL Vorbis sfml-audio sfml-graphics sfml-main sfml-network sfml-system sfml-window)
find_package(imgui CONFIG REQUIRED)
target_link_libraries(myrt.sfml PUBLIC imgui::imgui)
find_package(ImGui-SFML CONFIG REQUIRED)
target_link_libraries(myrt.sfml PRIVATE ImGui-SFML::ImGui-SFML)
target_link_libraries(myrt.sfml PUBLIC mygl::mygl)