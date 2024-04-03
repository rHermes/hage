#pragma once

#include "byte_buffer.hpp"

#include <span>
#include <deque>
#include <mutex>

namespace hage {
/**
This is a reference implementation of the buffer concepts we implement better above. It's here to show an alternative
implementation to the ring buffer. It should not be used for any real usage.
*/
class QueueBuffer final : public ByteBuffer
{
  std::mutex m_mtx;
  std::deque<std::byte> m_q;

  using iterator_type = decltype(m_q)::iterator;

  iterator_type m_readLevel{m_q.begin()};
  iterator_type m_writeLevel{m_q.end()};


  bool m_hasReader{false};
  bool m_hasWriter{false};

  class Reader final : public ByteBuffer::Reader
  {
  public:
    explicit Reader(QueueBuffer& parent) : m_parent(parent)
    {
      std::scoped_lock lk(m_parent.m_mtx);
      if (m_parent.m_hasReader)
        throw std::runtime_error("Two readers created, only 1 is supported at a time");

      m_parent.m_hasReader = true;
      m_newReadLevel = m_parent.m_q.begin();
    }

    ~Reader() override {
      std::scoped_lock lk(m_parent.m_mtx);
      m_parent.m_hasReader = false;
    }

    bool read(std::span<std::byte> dst) override
    {
      std::scoped_lock lk(m_parent.m_mtx);

      if (std::distance(m_newReadLevel, m_parent.m_writeLevel) < static_cast<std::ptrdiff_t>(dst.size_bytes()))
        return false;

      const auto first = m_newReadLevel;
      const auto end = std::next(first, static_cast<std::ptrdiff_t>(dst.size_bytes()));

      std::copy(first, end, dst.begin());

      m_bytesRead += dst.size_bytes();
      m_newReadLevel = end;
      return true;
    }

    bool commit() override
    {
      std::scoped_lock lk(m_parent.m_mtx);

      m_parent.m_q.erase(m_parent.m_q.begin(), m_newReadLevel);
      m_parent.m_readLevel = m_parent.m_q.begin();
      m_newReadLevel = m_parent.m_readLevel;

      if (m_parent.m_q.empty())
        m_parent.m_writeLevel = m_parent.m_q.end();

      return true;
    }
    [[nodiscard]] std::size_t bytes_read() const override
    {
      return m_bytesRead;
    }

  private:
    QueueBuffer& m_parent;
    iterator_type m_newReadLevel;
    std::size_t m_bytesRead{0};
  };

  class Writer final : public ByteBuffer::Writer
  {
  public:
    explicit Writer(QueueBuffer& parent) : m_parent(parent)
    {
      std::scoped_lock lk(m_parent.m_mtx);
      if (m_parent.m_hasWriter)
        throw std::runtime_error("Two writers created, only 1 is supported at a time");

      m_parent.m_hasWriter = true;
    }

    ~Writer() override {
      std::scoped_lock lk(m_parent.m_mtx);
      m_parent.m_hasWriter = false;

      // we are going to erase the end here. This will remove all elements not commited.
      m_parent.m_q.erase(m_parent.m_writeLevel, m_parent.m_q.end());
    }

    bool write(std::span<const std::byte> src) override
    {
      std::scoped_lock lk(m_parent.m_mtx);

      if (m_parent.m_writeLevel == m_parent.m_q.end())
        m_parent.m_writeLevel = m_parent.m_q.insert(m_parent.m_q.end(), src.begin(), src.end());
      else
        m_parent.m_q.insert(m_parent.m_q.end(), src.begin(), src.end());

      m_bytesWritten += src.size_bytes();

      return true;
    }

    bool commit() override
    {
      std::scoped_lock lk(m_parent.m_mtx);
      m_parent.m_writeLevel = m_parent.m_q.end();

      return true;
    }
    [[nodiscard]] std::size_t bytes_written() const override
    {
      return m_bytesWritten;
    }

  private:
    QueueBuffer& m_parent;
    std::size_t m_bytesWritten{0};
  };

public:

  [[nodiscard]] std::unique_ptr<ByteBuffer::Reader> get_reader() override
  {
    return std::make_unique<Reader>(*this);
  }

  [[nodiscard]] std::unique_ptr<ByteBuffer::Writer> get_writer() override
  {
    return std::make_unique<Writer>(*this);
  }

  [[nodiscard]] constexpr std::size_t capacity() override
  {
    return std::numeric_limits<std::size_t>::max();
  }

};

}