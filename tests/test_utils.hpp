#pragma once

#include <filesystem>
#include <fmt/core.h>

namespace hage::test {

[[nodiscard]] std::filesystem::path
temp_file_name(fmt::format_string<std::uint32_t> prefix);

namespace {

template<bool IsDir>
struct ScopedFsEntry final
{
  std::filesystem::path path;

  ~ScopedFsEntry()
  {
    if constexpr (IsDir) {
      std::filesystem::remove_all(path);
    } else {
      std::filesystem::remove(path);
    }
  }
};

template<bool IsDir>
struct ScopedTempFsEntry final
{
  std::filesystem::path path;

  explicit ScopedTempFsEntry(const fmt::format_string<std::uint32_t> prefix) : path{ temp_file_name(prefix) } {}

  ~ScopedTempFsEntry()
  {
    if constexpr (IsDir) {
      std::filesystem::remove_all(path);
    } else {
      std::filesystem::remove(path);
    }
  }
};

} // namespace

/**
 * A simple utility class to delete a file on destruction.
 */
using ScopedFile = ScopedFsEntry<false>;
using ScopedDir = ScopedFsEntry<true>;

using ScopedTempFile = ScopedTempFsEntry<false>;
using ScopedTempDir = ScopedTempFsEntry<true>;
} // namespace hage::test