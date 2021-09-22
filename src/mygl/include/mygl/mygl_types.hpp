#pragma once
/* This header contains the needed basic types for OpenGL.
INFO: the GLenum type is defined externally in mygl_enums.hpp.
*/

#include "mygl_enums.hpp"
#include <cinttypes>
#include <type_traits>
#if __has_include("KHR/khrplatform.h")
#include "KHR/khrplatform.h"
#endif

//All internal function pointer types
using GLVULKANPROCNV = void (*) (void);
using GLDEBUGPROCAMD = void (*) (std::uint32_t id, GLenum category, GLenum severity, std::int32_t length, const char *message, void *userParam);
using GLDEBUGPROC = void (*) (GLenum source, GLenum type, std::uint32_t id, GLenum severity, std::int32_t length, const char *message, const void *userParam);
using GLDEBUGPROCARB = void (*) (GLenum source, GLenum type, std::uint32_t id, GLenum severity, std::int32_t length, const char *message, const void *userParam);
using GLDEBUGPROCKHR = void (*) (GLenum source, GLenum type, std::uint32_t id, GLenum severity, std::int32_t length, const char *message, const void *userParam);

//custom rule types:
namespace mygl{
    template<typename Ident>
    struct basic_handle
    {
        using identifier_type = Ident;
        using handle_type = std::underlying_type_t<identifier_type>;
        using value_type = handle_type;

        constexpr static basic_handle zero() noexcept;
        constexpr static basic_handle from(handle_type h) noexcept;

        constexpr operator handle_type() const noexcept { return handle; }
        constexpr operator bool() const noexcept { return handle != 0; }
        constexpr bool operator ==(basic_handle other) const noexcept { return handle == other.handle; }
        constexpr bool operator !=(basic_handle other) const noexcept { return handle != other.handle; }
        
        handle_type handle;
    };

    template<typename Ident>
    constexpr basic_handle<Ident> basic_handle<Ident>::zero() noexcept
    {
        return from(0);
    }

    template<typename Ident>
    constexpr basic_handle<Ident> basic_handle<Ident>::from(handle_type h) noexcept
    {
        return basic_handle<Ident>{ static_cast<handle_type>(h) };
    }
}
