#pragma once

#include <hage/core/misc.hpp>
#include "byte_buffer.hpp"

#include <algorithm>
#include <mutex>
#include <span>
#include <vector>

namespace hage {
/**
This is a reference implementation of the buffer. It's here to show an alternative
implementation to the hage::ring_buffer. It should not be used for any real usage. It's just to provide known good
implementation that we can use.
*/
class VectorBuffer final : public ByteBuffer
{
  std::mutex m_mtx;
  std::vector<std::byte> m_q;

  using index_type = std::ptrdiff_t;


#ifdef HAGE_DEBUG
  bool m_hasReader{ false };
  bool m_hasWriter{ false };
#endif

  index_type m_writeLevel{ 0 };

  class Reader final : public ByteBuffer::Reader
  {
  public:
    explicit Reader(VectorBuffer& parent)
      : m_parent(parent)
    {
#ifdef HAGE_DEBUG
      std::scoped_lock lk(m_parent.m_mtx);

      if (m_parent.m_hasReader)
        throw std::runtime_error("Two readers created, only 1 is supported at a time");

      m_parent.m_hasReader = true;
#endif
    }

#ifdef HAGE_DEBUG
    ~Reader() override
    {
      std::scoped_lock lk(m_parent.m_mtx);
      m_parent.m_hasReader = false;
    }
#endif

    bool read(std::span<std::byte> dst) override
    {
      std::scoped_lock lk(m_parent.m_mtx);

      if ((m_parent.m_q.size() - m_newReadLevel + m_parent.m_writeLevel) < dst.size_bytes())
        return false;

      const auto first = std::next(m_parent.m_q.begin(), m_newReadLevel);
      const auto end = std::next(first, static_cast<std::ptrdiff_t>(dst.size_bytes()));

      std::copy(first, end, dst.begin());

      m_bytesRead += dst.size_bytes();
      m_newReadLevel += dst.size_bytes();
      return true;
    }

    bool commit() override
    {
      std::scoped_lock lk(m_parent.m_mtx);

      m_parent.m_q.erase(m_parent.m_q.begin(), std::next(m_parent.m_q.begin(), m_newReadLevel));
      m_newReadLevel = 0;

      return true;
    }

    [[nodiscard]] std::size_t bytes_read() const override { return m_bytesRead; }

  private:
    VectorBuffer& m_parent;
    index_type m_newReadLevel{ 0 };
    std::size_t m_bytesRead{ 0 };
  };

  class Writer final : public ByteBuffer::Writer
  {
  public:
    explicit Writer(VectorBuffer& parent)
      : m_parent(parent)
    {
#ifdef HAGE_DEBUG
      std::scoped_lock lk(m_parent.m_mtx);
      if (m_parent.m_hasWriter)
        throw std::runtime_error("Two writers created, only 1 is supported at a time");

      m_parent.m_hasWriter = true;
#endif
    }

    ~Writer() override
    {
      std::scoped_lock lk(m_parent.m_mtx);
#ifdef HAGE_DEBUG
      m_parent.m_hasWriter = false;
#endif

      m_parent.m_q.erase(std::next(m_parent.m_q.end(), m_parent.m_writeLevel), m_parent.m_q.end());
      m_parent.m_writeLevel = 0;
    }

    bool write(std::span<const std::byte> src) override
    {
      std::scoped_lock lk(m_parent.m_mtx);

      std::ranges::copy(src, std::back_inserter(m_parent.m_q));
      m_bytesWritten += src.size_bytes();
      m_parent.m_writeLevel -= src.size_bytes();

      return true;
    }

    bool commit() override
    {
      std::scoped_lock lk(m_parent.m_mtx);
      m_parent.m_writeLevel = 0;

      return true;
    }
    [[nodiscard]] std::size_t bytes_written() const override { return m_bytesWritten; }

  private:
    VectorBuffer& m_parent;
    std::size_t m_bytesWritten{ 0 };
  };

public:
  [[nodiscard]] std::unique_ptr<ByteBuffer::Reader> get_reader() override { return std::make_unique<Reader>(*this); }
  [[nodiscard]] std::unique_ptr<ByteBuffer::Writer> get_writer() override { return std::make_unique<Writer>(*this); }

  [[nodiscard]] constexpr std::size_t capacity() override { return std::numeric_limits<std::size_t>::max(); }
};

}