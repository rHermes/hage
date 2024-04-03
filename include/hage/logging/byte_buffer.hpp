#pragma once

#include <span>
#include <memory>

namespace hage {

class ByteBuffer
{
public:
  virtual ~ByteBuffer() = default;

  class Reader
  {
  public:
    virtual ~Reader() = default;
    virtual bool read(std::span<std::byte> dst) = 0;
    virtual bool commit() = 0;
    [[nodiscard]] virtual std::size_t bytes_read() const = 0;
  };

  [[nodiscard]] virtual std::unique_ptr<Reader> get_reader() = 0;

  class Writer
  {
  public:
    virtual ~Writer() = default;
    virtual bool write(std::span<const std::byte> src) = 0;
    virtual bool commit() = 0;
    [[nodiscard]] virtual std::size_t bytes_written() const = 0;
  };

  [[nodiscard]] virtual std::unique_ptr<Writer> get_writer() = 0;

  [[nodiscard]] virtual std::size_t capacity() = 0;
};

}