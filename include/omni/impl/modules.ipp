#pragma once

#include "omni/error.hpp"
#include "omni/hash.hpp"
#include "omni/module_export.hpp"
#include "omni/modules.hpp"

namespace omni {

  namespace detail {

    // Learn more here: https://devblogs.microsoft.com/oldnewthing/20060719-24/?p=30473
    template <concepts::hash Hasher>
    inline std::expected<module_export, std::error_code> resolve_forwarded_export(omni::address export_address) {
      // In a forwarded export, the address is a string containing
      // information about the actual export and its location
      // They are always presented as "module_name.export_name"
      const auto* forwarder_str = export_address.ptr<const char>();

      // Split forwarded export to module name and real export name
      forwarder_string forwarder{forwarder_string::parse(forwarder_str)};
      if (forwarder.function.empty()) {
        return {};
      }

      module_export real_export;

      // Try to search for the real export with a pre-known module name
      if (forwarder.is_ordinal()) {
        real_export = omni::get_export<Hasher>(forwarder.to_ordinal(), omni::hash<Hasher>(forwarder.module), omni::use_ordinal);
      } else {
        real_export = omni::get_export<Hasher>(omni::hash<Hasher>(forwarder.function), omni::hash<Hasher>(forwarder.module));
      }

      // The module that the forwarder is pointing to is not loaded into the
      // process, so we just return the result with forwarder_string
      if (!real_export.address) {
        return module_export{
          .is_forwarded = true,
          .forwarder_string = forwarder,
        };
      }

      return real_export;
    }

  } // namespace detail

  inline omni::module get_module(concepts::hash auto module_name) {
    omni::modules modules{};
    auto it = modules.find(module_name);
    if (it == modules.end()) {
      return {};
    }

    return *it;
  }

  inline omni::module get_module(default_hash module_name) {
    return get_module<default_hash>(module_name);
  }

  inline omni::module get_module(omni::address base_address) {
    omni::modules modules{};
    auto it = modules.find(base_address);
    if (it == modules.end()) {
      return {};
    }

    return *it;
  }

  inline module_export get_export(concepts::hash auto export_name, omni::module module) {
    if (!module.present()) {
      return {};
    }

    auto exports = module.exports();
    auto export_it = exports.find(export_name);
    if (export_it == exports.end()) {
      // The named export that the caller is looking for was
      // not found in the expected module
      return {};
    }

    if (export_it->is_forwarded) {
      return detail::resolve_forwarded_export<decltype(export_name)>(export_it->address).value_or(module_export{});
    }

    return *export_it;
  }

  inline module_export get_export(default_hash export_name, omni::module module) {
    return omni::get_export<default_hash>(export_name, module);
  }

  template <concepts::hash Hasher>
  inline module_export get_export(Hasher export_name, Hasher module_name) {
    return omni::get_export<Hasher>(export_name, omni::get_module(module_name));
  }

  inline module_export get_export(default_hash export_name, default_hash module_name) {
    return omni::get_export<default_hash>(export_name, omni::get_module(module_name));
  }

  inline module_export get_export(concepts::hash auto export_name) {
    for (const omni::module& module : omni::modules{}) {
      if (auto module_export = omni::get_export(export_name, module); module_export) {
        return module_export;
      }
    }
    return {};
  }

  inline module_export get_export(default_hash export_name) {
    return omni::get_export<default_hash>(export_name);
  }

  template <concepts::hash Hasher>
  inline module_export get_export(std::uint32_t ordinal, omni::module module, omni::use_ordinal_t) {
    if (!module.present()) {
      return {};
    }

    auto exports = module.exports();
    auto export_it = exports.find(ordinal, omni::use_ordinal);
    if (export_it == exports.end()) {
      // The ordinal export that the caller is looking for was
      // not found in the expected module
      return {};
    }

    if (export_it->is_forwarded) {
      return detail::resolve_forwarded_export<Hasher>(export_it->address).value_or(module_export{});
    }

    return *export_it;
  }

  inline module_export get_export(std::uint32_t ordinal, omni::module module, omni::use_ordinal_t) {
    return omni::get_export<default_hash>(ordinal, module, omni::use_ordinal);
  }

  inline module_export get_export(std::uint32_t ordinal, concepts::hash auto module_name, omni::use_ordinal_t) {
    return omni::get_export<decltype(module_name)>(ordinal, omni::get_module(module_name), omni::use_ordinal);
  }

  inline module_export get_export(std::uint32_t ordinal, default_hash module_name, omni::use_ordinal_t) {
    return omni::get_export<default_hash>(ordinal, module_name, omni::use_ordinal);
  }

  inline std::expected<module_export, std::error_code> try_get_export(concepts::hash auto export_name, omni::module module) {
    if (!module.present()) {
      return std::unexpected(omni::error::module_not_loaded);
    }

    auto exports = module.exports();
    auto export_it = exports.find(export_name);
    if (export_it == exports.end()) {
      return std::unexpected(omni::error::export_not_found);
    }

    if (export_it->is_forwarded) {
      return detail::resolve_forwarded_export<decltype(export_name)>(export_it->address);
    }

    return *export_it;
  }

  inline std::expected<module_export, std::error_code> try_get_export(default_hash export_name, omni::module module) {
    return try_get_export<default_hash>(export_name, module);
  }

  template <concepts::hash Hasher>
  inline std::expected<module_export, std::error_code> try_get_export(Hasher export_name, Hasher module_name) {
    auto module = omni::get_module(module_name);
    if (!module.present()) {
      return std::unexpected(omni::error::module_not_loaded);
    }

    return omni::try_get_export<Hasher>(export_name, module);
  }

  inline std::expected<module_export, std::error_code> try_get_export(default_hash export_name, default_hash module_name) {
    return try_get_export<default_hash>(export_name, module_name);
  }

  inline std::expected<module_export, std::error_code> try_get_export(concepts::hash auto export_name) {
    for (const omni::module& module : omni::modules{}) {
      auto exports = module.exports();
      auto export_it = exports.find(export_name);
      if (export_it == exports.end()) {
        continue;
      }

      if (export_it->is_forwarded) {
        return detail::resolve_forwarded_export<decltype(export_name)>(export_it->address);
      }

      return *export_it;
    }

    return std::unexpected(omni::error::export_not_found);
  }

  inline std::expected<module_export, std::error_code> try_get_export(default_hash export_name) {
    return try_get_export<default_hash>(export_name);
  }

  template <concepts::hash Hasher>
  inline std::expected<module_export, std::error_code> try_get_export(std::uint32_t ordinal, omni::module module,
    omni::use_ordinal_t) {
    if (!module.present()) {
      return std::unexpected(omni::error::module_not_loaded);
    }

    auto exports = module.exports();
    auto export_it = exports.find(ordinal, omni::use_ordinal);
    if (export_it == exports.end()) {
      return std::unexpected(omni::error::export_not_found);
    }

    if (export_it->is_forwarded) {
      return detail::resolve_forwarded_export<Hasher>(export_it->address);
    }

    return *export_it;
  }

  inline std::expected<module_export, std::error_code> try_get_export(std::uint32_t ordinal, omni::module module,
    omni::use_ordinal_t) {
    return try_get_export<default_hash>(ordinal, module, omni::use_ordinal);
  }

  inline std::expected<module_export, std::error_code> try_get_export(std::uint32_t ordinal, concepts::hash auto module_name,
    omni::use_ordinal_t) {
    return omni::try_get_export<decltype(module_name)>(ordinal, omni::get_module(module_name), omni::use_ordinal);
  }

  inline std::expected<module_export, std::error_code> try_get_export(std::uint32_t ordinal, default_hash module_name,
    omni::use_ordinal_t) {
    return try_get_export<default_hash>(ordinal, module_name, omni::use_ordinal);
  }

} // namespace omni
