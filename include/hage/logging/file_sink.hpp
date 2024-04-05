#pragma once
#include "sink.hpp"
#include <filesystem>

#include <fmt/os.h>

namespace hage {

class FileSink final : public Sink
{
public:
  explicit FileSink(const std::filesystem::path& path, const int flags = fmt::file::WRONLY | fmt::file::CREATE | fmt::file::TRUNC)
    : m_out(fmt::output_file(path.string(), flags))
  {
  }

  void receive(LogLevel, const timestamp_type&, std::string_view) override;

  [[nodiscard]] constexpr std::size_t bytes_written() const { return m_bytesWritten; };

private:
  fmt::ostream m_out;
  std::size_t m_bytesWritten{ 0 };
};
}