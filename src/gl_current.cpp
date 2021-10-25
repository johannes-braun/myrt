#include "gl_current.hpp"

#if _WIN32
#include <Windows.h>
#elif __linux__
#  error Linux not implemented yet.
#endif

namespace myrt {
  std::uint64_t current_context() {
#if _WIN32
    return reinterpret_cast<std::uint64_t>(wglGetCurrentContext());
#elif __linux__
#  error Linux not implemented yet.
#endif
}
  std::uint64_t current_window_handle() {
#if _WIN32
    return reinterpret_cast<std::uint64_t>(wglGetCurrentDC());
#elif __linux__
#  error Linux not implemented yet.
#endif

}
  } // namespace myrt