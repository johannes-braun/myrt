cmake_minimum_required(VERSION 3.13)

project(myrt
    VERSION 1.0
    LANGUAGES CXX)

add_executable(myrt 
    "src/myrt.cpp" 
    "src/cube.hpp" 
    "src/suzanne.hpp"  
    "src/plate_of_beans.hpp" 
    "src/pathtracer/bvh.hpp" 
    "src/pathtracer/bvh.cpp" 
    "src/pathtracer/utils.hpp" 
    "src/pathtracer/texture_provider.hpp" 
    "src/pathtracer/texture_provider.cpp" 
    "src/pathtracer/pathtracer.hpp" 
    "src/pathtracer/pathtracer.cpp" 
    "src/pathtracer/scene.hpp" 
    "src/pathtracer/scene.cpp" 
    "src/pathtracer/utils.cpp")
target_compile_features(myrt PUBLIC cxx_std_20)

find_package(SFML 2 COMPONENTS graphics audio main network system window REQUIRED)
target_link_libraries(myrt PUBLIC Freetype OpenAL OpenGL Vorbis sfml-audio sfml-graphics sfml-main sfml-network sfml-system sfml-window)
find_package(glad REQUIRED)
target_link_libraries(myrt PUBLIC glad::glad)
find_package(Stb REQUIRED)
target_include_directories(myrt PUBLIC ${Stb_INCLUDE_DIR})
find_package(imgui CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)
target_link_libraries(myrt PRIVATE ImGui-SFML::ImGui-SFML)

# GLShader
find_package(Git REQUIRED)
set(EXT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext)

function(add_external path repo)
    if(NOT EXISTS ${path})
        execute_process(COMMAND ${GIT_EXECUTABLE} clone ${repo} ${path})
    endif()
    add_subdirectory(${path})
endfunction()
add_external(${EXT_DIR}/glsp "https://github.com/johannes-braun/GLshader.git")
add_external(${EXT_DIR}/rnu "https://github.com/johannes-braun/rnu.git")

target_link_libraries(myrt PRIVATE glsp::glsp)
target_link_libraries(myrt PRIVATE rnu::rnu)