#pragma once

#include <cassert>
#include <cstdint>
#include <string_view>

#include "omni/address.hpp"

namespace omni {

  struct use_ordinal_t {};
  [[maybe_unused]] constexpr inline use_ordinal_t use_ordinal{};

  struct forwarder_string {
    std::string_view module;
    std::string_view function;

    [[nodiscard]] static forwarder_string parse(std::string_view forwarder_str) noexcept {
      auto pos = forwarder_str.find('.');
      if (pos != std::string_view::npos) {
        auto first_part = forwarder_str.substr(0, pos);
        auto second_part = forwarder_str.substr(pos + 1);
        return forwarder_string{.module = first_part, .function = second_part};
      }
      assert(false);
      return forwarder_string{.module = forwarder_str, .function = std::string_view{}};
    }

    [[nodiscard]] bool is_ordinal() const noexcept {
      return !function.empty() && function.front() == '#';
    }

    [[nodiscard]] std::uint32_t to_ordinal() const noexcept {
      if (function.empty()) {
        return 0;
      }

      std::uint32_t ordinal{};
      // Ordinal forwarder always starts from '#', skip it
      auto ordinal_str = function.substr(1);
      auto result = std::from_chars(ordinal_str.data(), ordinal_str.data() + ordinal_str.size(), ordinal);
      assert(result.ec == std::errc{});
      return ordinal;
    }

    [[nodiscard]] bool present() const noexcept {
      return !(module.empty() || function.empty());
    }
  };

  struct module_export {
    std::string_view name;
    omni::address address;
    std::uint32_t ordinal{};
    bool is_forwarded{};
    omni::forwarder_string forwarder_string{};
    omni::address module_base;

    [[nodiscard]] bool is_ordinal_only() const noexcept {
      return name.empty();
    }

    [[nodiscard]] bool present() const noexcept {
      return static_cast<bool>(address);
    }

    [[nodiscard]] explicit operator bool() const noexcept {
      return present();
    }
  };

} // namespace omni
