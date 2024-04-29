#pragma once

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

// We want to support two modes: maxSize
// time stamps.

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

  std::vector<std::filesystem::path> m_names;
};

class RotatingFileSink final : public Sink
{
public:
  struct Config final
  {
    std::filesystem::path saveDirectory;

    [[nodiscard]] bool valid() const
    {
      bool good = !saveDirectory.empty();

      return good;
    }
  };

  void receive(LogLevel level, const timestamp_type& ts, std::string_view line) override;

  RotatingFileSink(Config conf, std::unique_ptr<Rotater> rotater)
    : m_conf{ std::move(conf) }
    , m_rotater(std::move(rotater))
  {
    if (m_conf.valid()) {
      throw std::runtime_error("Invalid configuration");
    }
  }

  ~RotatingFileSink() override;

private:
  Config m_conf;
  std::unique_ptr<Rotater> m_rotater;

  std::deque<std::filesystem::path> m_prevNames;
};

}
