#pragma once

#include <cstdint>
#include <format>

#include "omni/concepts.hpp"
#include "omni/detail/hash_impl.hpp"

namespace omni {

  using fnv1a32 = detail::fnv1a_hash<std::uint32_t>;
  using fnv1a64 = detail::fnv1a_hash<std::uint64_t>;
  using default_hash = fnv1a64;

  template <concepts::hash Hasher = default_hash>
  struct hash_pair {
    consteval hash_pair(Hasher first, Hasher second): first(first), second(second) {}

    Hasher first;
    Hasher second;
  };

  static_assert(concepts::hash<fnv1a32>);
  static_assert(concepts::hash<fnv1a64>);
  static_assert(concepts::hash<default_hash>);

  template <typename T>
  [[nodiscard]] constexpr auto hash(concepts::hashable auto object) {
    return T{}(object);
  }

} // namespace omni

template <>
struct std::formatter<omni::fnv1a32> : std::formatter<omni::fnv1a32::value_type> {
  auto format(const omni::fnv1a32& hash, std::format_context& ctx) const {
    return std::formatter<omni::fnv1a32::value_type>::format(hash.value(), ctx);
  }
};

template <>
struct std::formatter<omni::fnv1a64> : std::formatter<omni::fnv1a64::value_type> {
  auto format(const omni::fnv1a64& hash, std::format_context& ctx) const {
    return std::formatter<omni::fnv1a64::value_type>::format(hash.value(), ctx);
  }
};
