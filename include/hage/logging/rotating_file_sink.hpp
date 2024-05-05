#pragma once

#include "file_sink.hpp"

#include <filesystem>
#include <functional>

#include <deque>
#include <hage/core/concepts.hpp>

#include "sink.hpp"

namespace hage {

struct LogFileStats final
{
  std::size_t bytes;
};

class Rotater
{
public:
  enum class Type
  {
    MoveBackwards,
    RemoveLast
  };

  virtual ~Rotater() = default;

  [[nodiscard]] virtual bool shouldRotate(const LogFileStats& stats) = 0;
  [[nodiscard]] virtual std::vector<std::filesystem::path> generateNames(const LogFileStats& stats, int times) = 0;
  [[nodiscard]] virtual Type getRotateType() const = 0;

  // disable assignment operator (due to the problem of slicing):
  Rotater& operator=(Rotater&&) = delete;
  Rotater& operator=(const Rotater&) = delete;

protected:
  Rotater() = default;
  // enable copy and move semantics (callable only for derived classes):
  Rotater(const Rotater&) = default;
  Rotater(Rotater&&) = default;
};

class RotatingFileSink final : public Sink
{
public:
  struct Config final
  {
    std::filesystem::path saveDirectory;
    int maxNumber{ -1 };

    [[nodiscard]] bool valid() const
    {
      bool good = !saveDirectory.empty();
      good      = good && 0 < maxNumber;

      return good;
    }
  };

  void receive(LogLevel level, const timestamp_type& ts, std::string_view line) override;

  RotatingFileSink(Config conf, std::unique_ptr<Rotater> rotater);

  void flush();

  ~RotatingFileSink() override;

private:
  void rotate(const LogFileStats& stats);

  Config m_conf;
  std::unique_ptr<Rotater> m_rotater;

  // TODO(rHermes): Rework this, so that there exists a default constructable FileSink.
  // this enables move semantics and so on.
  std::optional<FileSink> m_currentFile;
};

class SizeRotater final : public Rotater
{
public:
  SizeRotater(std::filesystem::path base, std::size_t maxSize);

  [[nodiscard]] bool shouldRotate(const LogFileStats& stats) override;
  [[nodiscard]] std::vector<std::filesystem::path> generateNames(const LogFileStats& stats, int times) override;
  [[nodiscard]] Type getRotateType() const override { return Type::MoveBackwards; };

private:
  std::size_t m_maxSize;
  std::filesystem::path m_base;
};

/**
 * @short A rotater based on a timing string.
 */
class TimeRotater final : public Rotater
{
public:
  using fmt_string = fmt::format_string<std::chrono::system_clock::time_point>;
  explicit TimeRotater(fmt_string base);

  [[nodiscard]] bool shouldRotate(const LogFileStats& stats) override;
  [[nodiscard]] std::vector<std::filesystem::path> generateNames(const LogFileStats& stats, int times) override;
  [[nodiscard]] Type getRotateType() const override { return Type::RemoveLast; };

private:
  fmt_string m_format;
  std::vector<std::filesystem::path> m_previous;
};
}
