#pragma once

#include <filesystem>
#include <functional>

#include <hage/core/concepts.hpp>

#include "sink.hpp"

namespace hage {

struct LogFileStats final
{
  std::size_t bytes;
};

// We should create a concept
template<typename T>
concept Splitter = std::predicate<T, const LogFileStats&>;

class SizeSplitter final
{
public:
  explicit SizeSplitter(std::size_t sz)
    : m_maxSize{ sz }
  {
  }

  [[nodiscard]] constexpr bool operator()(const LogFileStats& stats) const { return m_maxSize < stats.bytes; }

private:
  std::size_t m_maxSize;
};
static_assert(Splitter<SizeSplitter>);

template<Splitter... Splitters>
class AndSplitter final
{
public:
  explicit AndSplitter(Splitters&&... splitters)
    : m_splitters{ std::forward<Splitters>(splitters)... }
  {
  }

  [[nodiscard]] bool operator()(const LogFileStats& stats)
  {
    return [this, &stats]<std::size_t... Is>(std::index_sequence<Is...>) {
      return (... and std::get<Is>(m_splitters)(stats));
    }(std::index_sequence_for<Splitters...>{});
  }

private:
  std::tuple<Splitters...> m_splitters;
};

static_assert(Splitter<AndSplitter<SizeSplitter, SizeSplitter>>);

template<Splitter... Splitters>
class OrSplitter final
{
public:
  explicit OrSplitter(Splitters&&... splitters)
    : m_splitters{ std::forward<Splitters>(splitters)... }
  {
  }

  [[nodiscard]] bool operator()(const LogFileStats& stats)
  {
    return [this, &stats]<std::size_t... Is>(std::index_sequence<Is...>) {
      return (... or std::get<Is>(m_splitters)(stats));
    }(std::index_sequence_for<Splitters...>{});
  }

private:
  std::tuple<Splitters...> m_splitters;
};
static_assert(Splitter<OrSplitter<SizeSplitter, SizeSplitter>>);

// We create a namer, with a removal policy.
class Namer
{
public:
  virtual ~Namer() = default;

  [[nodiscard]] virtual std::vector<std::filesystem::path> generateNames(const LogFileStats& stats, int backlog) = 0;

  // disable assignment operator (due to the problem of slicing):
  Namer& operator=(Namer&&) = delete;
  Namer& operator=(const Namer&) = delete;

protected:
  Namer() = default;
  // enable copy and move semantics (callable only for derived classes):
  Namer(const Namer&) = default;
  Namer(Namer&&) = default;
};

class RotatingFileSink final : public Sink
{
public:
  struct Config final
  {
    std::filesystem::path saveDirectory;
    int backlog{ 10 };

    [[nodiscard]] bool valid() const
    {
      bool good = (0 <= backlog);
      good = good && !saveDirectory.empty();

      return good;
    }
  };

  void receive(LogLevel level, const timestamp_type& ts, std::string_view line) override;

  template<Splitter T>
  RotatingFileSink(Config conf, T&& splitter)
    : m_conf{ std::move(conf) }
    , m_splitter(std::forward<T>(splitter))
  {

    if (m_conf.valid()) {
      throw std::runtime_error("Invalid configuration");
    }
  }

  ~RotatingFileSink() override;

private:
  Config m_conf;
  std::function<bool(const LogFileStats&)> m_splitter;
};

}
