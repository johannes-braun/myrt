add_library(myrt.xgl STATIC)
target_compile_features(myrt.xgl PUBLIC cxx_std_23)
target_include_directories(myrt.xgl PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_include_directories(myrt.xgl PRIVATE include/myrt/xgl)
target_sources(myrt.xgl PRIVATE
    "include/myrt/xgl/xgl_tokenizer.hpp"
    "include/myrt/xgl/xgl_grammar.hpp" 
    "include/myrt/xgl/xgl_parser.hpp"
    "include/myrt/xgl/xgl_evaluator.hpp" 
    "include/myrt/xgl/xgl_scope_system.hpp" 
    "include/myrt/xgl/xgl_entities.hpp" 
    "xgl_tokenizer.cpp" 
    "xgl_grammar.cpp" 
    "xgl_parser.cpp" 
    "xgl_evaluator.cpp" 
    "xgl_scope_system.cpp" 
    "xgl_entities.cpp")