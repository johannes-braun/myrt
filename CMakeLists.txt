cmake_minimum_required(VERSION 3.13)

project(myrt
    VERSION 1.0
    LANGUAGES CXX)
  
IF(MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
ENDIF(MSVC)

add_subdirectory(src)

add_executable(myrt 
    #"src/myrt.cpp" 
    "src/pathtracer/bvh.hpp" 
    "src/pathtracer/bvh.cpp" 
    "src/pathtracer/texture_provider.hpp" 
    "src/pathtracer/texture_provider.cpp" 
    "src/pathtracer/pathtracer.hpp" 
    "src/pathtracer/pathtracer.cpp" 
    "src/pathtracer/scene.hpp" 
    "src/pathtracer/scene.cpp" 
    "src/obj.hpp" 
    "src/obj.cpp" 
    "src/glsl/color.h" 
    "src/glsl/tonemapping.h"  
    "src/vectoring/vectoring.hpp" 
    "src/vectoring/distance.hpp"  
    "src/stb.cpp" 
    "src/main.cpp" 
    "src/myrt.sfml/include/myrt/sfml/sfml.hpp" 
    "src/resources.hpp"  
    "src/pathtracer/sdf.hpp"
    "src/thread_pool.hpp" 
    "src/thread_pool.cpp"
    "src/pathtracer/sequential_pathtracer.hpp" 
    "src/pathtracer/sequential_pathtracer.cpp" 
     
     
    "src/pathtracer/parameter.hpp" 
    "src/pathtracer/parameter.cpp" 
    "src/pathtracer/parameterized.hpp" 
    "src/pathtracer/parameterized.cpp"  
    "src/dynamic/object.hpp" 
    "src/dynamic/property.hpp" 
    "src/dynamic/object.cpp" 
    "src/dynamic/property.cpp" 
   #"src/dynamic/material.hpp" 
   #"src/dynamic/material.cpp" 
    "src/pathtracer/material.hpp" 
    "src/pathtracer/material.cpp" 
    "src/mygl/include/mygl/mygl.hpp" 
    "src/mygl/include/mygl/mygl_enums.hpp" 
    "src/mygl/include/mygl/mygl_extensions.hpp" 
    "src/mygl/include/mygl/mygl_functions.hpp" 
    "src/mygl/include/mygl/mygl_loader.hpp" 
    "src/mygl/include/mygl/mygl_types.hpp" 
    "src/mygl/mygl.cpp" 
    "src/pathtracer/post_process.hpp" 
    "src/pathtracer/post_process.cpp"
    "src/pathtracer/bindings.hpp" 
    "src/pathtracer/bindings.cpp"
    "src/pathtracer/forward_renderer.hpp"
    "src/pathtracer/forward_renderer.cpp" "src/pathtracer/sdf.cpp"  
"src/ecs/ecs.cpp"
"src/ecs/system.cpp" )
target_compile_features(myrt PUBLIC cxx_std_23)
target_link_libraries(myrt PUBLIC myrt.sfml myrt.xgl)

find_package(SFML 2 COMPONENTS graphics audio main network system window REQUIRED)
target_link_libraries(myrt PUBLIC Freetype OpenAL OpenGL Vorbis sfml-audio sfml-graphics sfml-main sfml-network sfml-system sfml-window)
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