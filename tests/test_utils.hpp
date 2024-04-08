#pragma once

#include <filesystem>
#include <fmt/core.h>

namespace hage::test {

[[nodiscard]] std::filesystem::path
temp_file_name(fmt::format_string<std::uint32_t> prefix);

/**
 * A simple utility class to delete a file on destruction.
 */
struct ScopedFile
{
  std::filesystem::path path;

  explicit ScopedFile(std::filesystem::path path)
    : path{ std::move(path) }
  {
  }

  ~ScopedFile() { std::filesystem::remove(path); }
};

struct ScopedTempFile
{
  std::filesystem::path path;
  explicit ScopedTempFile(const fmt::format_string<std::uint32_t> prefix)
    : path{ temp_file_name(prefix) }
  {
  }

  ~ScopedTempFile() { std::filesystem::remove(path); }
};

}