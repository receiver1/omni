#include <Windows.h>

#include <array>
#include <boost/ut.hpp>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>

#include "omni/module.hpp"
#include "omni/modules.hpp"

namespace ut = boost::ut;
using ut::expect;
using ut::fatal;
using ut::operator""_test;

namespace {

  [[nodiscard]] std::filesystem::path get_module_path(HMODULE module_handle) {
    std::array<wchar_t, 32768> buffer{};
    auto length = ::GetModuleFileNameW(module_handle, buffer.data(), static_cast<DWORD>(buffer.size()));
    return {std::wstring_view{buffer.data(), length}};
  }

  [[nodiscard]] omni::module get_loaded_module(HMODULE module_handle) {
    omni::modules loaded_modules{};
    auto module_it = loaded_modules.find(omni::address{module_handle});
    if (module_it == loaded_modules.end()) {
      return {};
    }

    return *module_it;
  }

  struct loaded_library {
    explicit loaded_library(const wchar_t* module_name) noexcept
      : handle{::LoadLibraryExW(module_name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32)} {
      if (handle == nullptr) {
        handle = ::LoadLibraryW(module_name);
      }
    }

    loaded_library(const loaded_library&) = delete;
    loaded_library& operator=(const loaded_library&) = delete;

    loaded_library(loaded_library&& other) noexcept: handle{std::exchange(other.handle, nullptr)} {}
    loaded_library& operator=(loaded_library&& other) noexcept {
      if (this != &other) {
        if (handle != nullptr) {
          ::FreeLibrary(handle);
        }
        handle = std::exchange(other.handle, nullptr);
      }
      return *this;
    }

    ~loaded_library() {
      if (handle != nullptr) {
        ::FreeLibrary(handle);
      }
    }

    [[nodiscard]] explicit operator bool() const noexcept {
      return handle != nullptr;
    }

    HMODULE handle{};
  };

} // namespace

ut::suite<"omni::module"> module_suite = [] {
  "default constructed module is empty"_test = [] {
    omni::module module{};
    omni::module other{};

    expect(not module.present());
    expect(not static_cast<bool>(module));
    expect(module.entry() == nullptr);
    expect(module == other);
  };

  "copied module keeps the original loader entry identity"_test = [] {
    HMODULE executable_handle = ::GetModuleHandleW(nullptr);
    omni::module executable_module = get_loaded_module(executable_handle);
    omni::module reconstructed{executable_module.entry()};

    expect(fatal(executable_handle != nullptr));
    expect(fatal(executable_module.present()));

    expect(reconstructed.present());
    expect(reconstructed.entry() == executable_module.entry());
    expect(reconstructed == executable_module);
  };

  "base address image and entry point match the PE image"_test = [] {
    HMODULE executable_handle = ::GetModuleHandleW(nullptr);
    omni::module executable_module = get_loaded_module(executable_handle);
    omni::win::image* image = executable_module.image();
    omni::address expected_entry_point = executable_module.base_address().offset(image->get_optional_header()->entry_point);

    expect(fatal(executable_handle != nullptr));
    expect(fatal(executable_module.present()));

    expect(executable_module.base_address() == executable_handle);
    expect(executable_module.native_handle() == executable_handle);
    expect(image == executable_module.base_address().ptr<omni::win::image>());
    expect(image->get_dos_headers()->e_magic == IMAGE_DOS_SIGNATURE);
    expect(image->get_nt_headers()->signature == IMAGE_NT_SIGNATURE);
    expect(image->get_file_header() != nullptr);
    expect(image->get_optional_header() != nullptr);
    expect(executable_module.entry_point() == expected_entry_point);
  };

  "wname and system_path match the module path from WinAPI"_test = [] {
    HMODULE kernel32_handle = ::GetModuleHandleW(L"kernel32.dll");
    omni::module kernel32_module = get_loaded_module(kernel32_handle);
    auto kernel32_path = get_module_path(kernel32_handle);

    expect(fatal(kernel32_handle != nullptr));
    expect(fatal(kernel32_module.present()));

    expect(std::wcscmp(kernel32_module.system_path().c_str(), kernel32_path.c_str()) == 0);
    expect(kernel32_module.wname() == kernel32_path.filename().wstring());
  };

  "name and ostream expose the ansi module filename"_test = [] {
    loaded_library version_dll{L"version.dll"};
    omni::module version_module = get_loaded_module(version_dll.handle);
    std::string expected_name = version_module.system_path().filename().string();
    std::ostringstream output{};

    expect(fatal(static_cast<bool>(version_dll)));
    expect(fatal(version_module.present()));

    output << version_module;

    expect(version_module.name() == expected_name);
    expect(output.str() == expected_name);
  };

  "matches_name_hash accepts stem full name and case variations"_test = [] {
    loaded_library version_dll{L"version.dll"};
    omni::module version_module = get_loaded_module(version_dll.handle);

    expect(fatal(static_cast<bool>(version_dll)));
    expect(fatal(version_module.present()));

    expect(version_module.matches_name_hash(L"version"));
    expect(version_module.matches_name_hash(L"version.dll"));
    expect(version_module.matches_name_hash(L"VERSION.DLL"));
    expect(not version_module.matches_name_hash(L"version.exe"));
    expect(not version_module.matches_name_hash(L"versions"));
  };

  "exports exposes a real export from version.dll"_test = [] {
    loaded_library version_dll{L"version.dll"};
    omni::module version_module = get_loaded_module(version_dll.handle);
    omni::module_exports exports = version_module.exports();
    omni::default_hash export_name{"GetFileVersionInfoSizeW"};
    auto export_it = exports.find(export_name);
    FARPROC export_address = ::GetProcAddress(version_dll.handle, "GetFileVersionInfoSizeW");
    auto export_directory_rva = version_module.image()->get_optional_header()->data_directories.export_directory.rva;
    auto* export_directory = version_module.base_address().ptr<omni::win::export_directory>(export_directory_rva);

    expect(fatal(static_cast<bool>(version_dll)));
    expect(fatal(version_module.present()));
    expect(fatal(export_address != nullptr));

    expect(exports.directory() != nullptr);
    expect(exports.directory() == export_directory);
    expect(exports.size() > 0U);
    expect(export_it != exports.end());
    expect(not export_it->is_forwarded);
    expect(export_it->address == export_address);
    expect(export_it->module_base == version_module.base_address());
  };
};
