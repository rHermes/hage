#pragma once

#include "byte_buffer.hpp"

#include <list>
#include <mutex>
#include <span>

namespace hage {
/**
This is a reference implementation of the buffer. It's here to show an alternative
implementation to the hage::ring_buffer. It should not be used for any real usage. It's just to provide known good
implementation that we can use.
*/
class ListBuffer final : public ByteBuffer
{
  std::mutex m_mtx;
  std::list<std::byte> m_q;

  using iterator_type = decltype(m_q)::iterator;

  bool m_hasReader{ false };
  bool m_hasWriter{ false };

  class Reader final : public ByteBuffer::Reader
  {
  public:
    explicit Reader(ListBuffer& parent)
      : m_parent(parent)
    {
      std::scoped_lock lk(m_parent.m_mtx);
      if (m_parent.m_hasReader)
        throw std::runtime_error("Two readers created, only 1 is supported at a time");

      m_parent.m_hasReader = true;
      m_newReadLevel = m_parent.m_q.begin();
    }

    ~Reader() override
    {
      std::scoped_lock lk(m_parent.m_mtx);
      m_parent.m_hasReader = false;
    }

    bool read(std::span<std::byte> dst) override
    {
      std::scoped_lock lk(m_parent.m_mtx);

      if (std::distance(std::next(m_newReadLevel), m_parent.m_q.end()) < static_cast<std::ptrdiff_t>(dst.size_bytes()))
        return false;

      std::ranges::copy_n(std::next(m_newReadLevel), dst.size_bytes(), dst.begin());

      m_bytesRead += dst.size_bytes();
      m_newReadLevel = std::next(m_newReadLevel, static_cast<std::ptrdiff_t>(dst.size_bytes()));
      return true;
    }

    bool commit() override
    {
      std::scoped_lock lk(m_parent.m_mtx);

      if (m_newReadLevel != m_parent.m_q.begin())
        m_parent.m_q.erase(std::next(m_parent.m_q.begin()), m_newReadLevel);

      return true;
    }

    [[nodiscard]] std::size_t bytes_read() const override { return m_bytesRead; }

  private:
    ListBuffer& m_parent;
    iterator_type m_newReadLevel;
    std::size_t m_bytesRead{ 0 };
  };

  class Writer final : public ByteBuffer::Writer
  {
  public:
    explicit Writer(ListBuffer& parent)
      : m_parent(parent)
    {
      std::scoped_lock lk(m_parent.m_mtx);
      if (m_parent.m_hasWriter)
        throw std::runtime_error("Two writers created, only 1 is supported at a time");

      m_parent.m_hasWriter = true;
    }

    ~Writer() override
    {
      std::scoped_lock lk(m_parent.m_mtx);
      m_parent.m_hasWriter = false;
    }

    bool write(std::span<const std::byte> src) override
    {
      std::scoped_lock lk(m_parent.m_mtx);

      m_buff.insert(m_buff.end(), src.begin(), src.end());
      m_bytesWritten += src.size_bytes();

      return true;
    }

    bool commit() override
    {
      std::scoped_lock lk(m_parent.m_mtx);
      m_parent.m_q.splice(m_parent.m_q.end(), m_buff);

      return true;
    }
    [[nodiscard]] std::size_t bytes_written() const override { return m_bytesWritten; }

  private:
    ListBuffer& m_parent;
    std::list<std::byte> m_buff;
    std::size_t m_bytesWritten{ 0 };
  };

public:
  ListBuffer() { m_q.push_back(static_cast<std::byte>(0)); }

  [[nodiscard]] std::unique_ptr<ByteBuffer::Reader> get_reader() override { return std::make_unique<Reader>(*this); }

  [[nodiscard]] std::unique_ptr<ByteBuffer::Writer> get_writer() override { return std::make_unique<Writer>(*this); }

  [[nodiscard]] constexpr std::size_t capacity() override { return std::numeric_limits<std::size_t>::max() - 1; }
};

}