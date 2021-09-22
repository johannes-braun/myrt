#pragma once
/*
 __  __  _  _  ___  __   
(  \/  )( \/ )/ __)(  )  
 )    (  \  /( (_-. )(__ 
(_/\/\_) (__) \___/(____)

MYGL - OpenGL Function Loader.
-----------------------------------

If you want to use MyGL as a header-only library, make sure to include it in exactly one cpp with a previous definition of MYGL_IMPLEMENTATION as follows:

    #define MYGL_IMPLEMENTATION
    #include <mygl.hpp>

There is also a pre-generated cpp file so you can also just put the generated files into a source-subdirectory of yours.

You can then run the loader after context-creation and after making a context current with the functions
    
    mygl::load();
        - or -
    mygl::load(reinterpret_cast<mygl::loader_function>(myGetProcAddress));

-----------------------------------

If there is a need to have multiple loaders, you can use a mygl::dispatch

-----------------------------------
*/

#include "mygl_enums.hpp"
#include "mygl_extensions.hpp"
#include "mygl_functions.hpp"
#include "mygl_loader.hpp"
#include "mygl_types.hpp"
