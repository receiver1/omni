#pragma once

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
#  define OMNI_HAS_EXCEPTIONS
#endif

#if !defined(OMNI_HAS_CACHING)
#  if !defined(OMNI_DISABLE_CACHING)
#    define OMNI_HAS_CACHING
#  endif
#endif

#if !defined(OMNI_DISABLE_ERROR_STRINGS)
#  if defined(OMNI_ENABLE_ERROR_STRINGS) || defined(DEBUG) || defined(_DEBUG)
#    define OMNI_HAS_ERROR_STRINGS
#  endif
#endif

namespace omni::detail {
  [[maybe_unused]] constexpr inline bool is_x64 = sizeof(void*) == 8;
  [[maybe_unused]] constexpr inline bool is_x86 = sizeof(void*) == 4;
} // namespace omni::detail
