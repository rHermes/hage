#pragma once

#include "../misc.hpp"
#include "byte_buffer.hpp"

#include <span>

namespace hage {
// This is a rather dumb container, but it will work for most
// of our tests.
template<std::ptrdiff_t N>
class RingBuffer final : public ByteBuffer
{

  alignas(details::destructive_interference_size) std::atomic<std::ptrdiff_t> m_head{ 0 };
  alignas(details::destructive_interference_size) std::ptrdiff_t m_cachedHead{ 0 };

  alignas(details::destructive_interference_size) std::atomic<std::ptrdiff_t> m_tail{ 0 };
  alignas(details::destructive_interference_size) std::ptrdiff_t m_cachedTail{ 0 };

  alignas(details::destructive_interference_size) std::array<std::byte, N + 1> m_buff;

  alignas(details::constructive_interference_size) std::atomic_flag m_hasReader;
  alignas(details::constructive_interference_size) std::atomic_flag m_hasWriter;

  class Reader final : public ByteBuffer::Reader
  {
  public:
    explicit Reader(RingBuffer& parent)
      : m_parent{ parent }
    {
      if (m_parent.m_hasReader.test_and_set(std::memory_order::acq_rel))
        throw std::runtime_error("We can only have one concurrent reader for RingBuffer");

      m_shadowHead = m_parent.m_head.load(std::memory_order::relaxed);
    }

    ~Reader() override { m_parent.m_hasReader.clear(std::memory_order::release); }

    bool read(std::span<std::byte> dst) override
    {
      if (N < dst.size_bytes())
        return false;

      while (!dst.empty()) {
        const auto sz = static_cast<std::ptrdiff_t>(dst.size_bytes());
        if (m_shadowHead == N + 1)
          m_shadowHead = 0;

        if (m_shadowHead == m_parent.m_cachedTail) {
          m_parent.m_cachedTail = m_parent.m_tail.load(std::memory_order::acquire);
          if (m_shadowHead == m_parent.m_cachedTail)
            return false;
        }

        if (m_shadowHead < m_parent.m_cachedTail) {
          const auto spaceLeft = m_parent.m_cachedTail - m_shadowHead;
          const auto readSize = std::min(sz, spaceLeft);

          std::memcpy(dst.data(), m_parent.m_buff.data() + m_shadowHead, static_cast<std::size_t>(readSize));
          m_shadowHead += readSize;
          m_bytesRead += readSize;

          dst = dst.subspan(static_cast<std::size_t>(readSize));
        } else {
          const auto spaceLeft = N + 1 - m_shadowHead;
          const auto readSize = std::min(sz, spaceLeft);

          std::memcpy(dst.data(), m_parent.m_buff.data() + m_shadowHead, static_cast<std::size_t>(readSize));
          m_shadowHead += readSize;
          m_bytesRead += readSize;

          dst = dst.subspan(static_cast<std::size_t>(readSize));
        }
      }

      return true;
    }

    bool commit() override
    {
      m_parent.m_head.store(m_shadowHead, std::memory_order::release);
      return true;
    }
    [[nodiscard]] std::size_t bytes_read() const override { return m_bytesRead; }

  private:
    RingBuffer& m_parent;
    std::ptrdiff_t m_shadowHead{ 0 };
    std::size_t m_bytesRead{ 0 };
  };

  class Writer final : public ByteBuffer::Writer
  {
  public:
    explicit Writer(RingBuffer& parent)
      : m_parent{ parent }
    {
      if (m_parent.m_hasWriter.test_and_set(std::memory_order::acq_rel))
        throw std::runtime_error("We can only have one concurrent writer for RingBuffer");

      m_shadowTail = m_parent.m_tail.load(std::memory_order::relaxed);
    }

    ~Writer() override { m_parent.m_hasWriter.clear(std::memory_order::release); }

    bool commit() override
    {
      m_parent.m_tail.store(m_shadowTail, std::memory_order::release);
      return true;
    }

    bool write(std::span<const std::byte> src) override
    {
      if (N < src.size_bytes())
        return false;

      while (!src.empty()) {
        const auto sz = static_cast<std::ptrdiff_t>(src.size_bytes());

        if (m_shadowTail == N + 1) {
          if (m_parent.m_cachedHead == 0) {
            m_parent.m_cachedHead = m_parent.m_head.load(std::memory_order::acquire);
            if (m_parent.m_cachedHead == 0)
              return false;
          }
          m_shadowTail = 0;
        } else if (m_shadowTail + 1 == m_parent.m_cachedHead) {
          m_parent.m_cachedHead = m_parent.m_head.load(std::memory_order::acquire);
          if (m_shadowTail + 1 == m_parent.m_cachedHead)
            return false;
        }

        if (m_parent.m_cachedHead <= m_shadowTail) {
          const auto spaceLeft = N + 1 - m_shadowTail;
          const auto writeSize = std::min(sz, spaceLeft);

          std::memcpy(m_parent.m_buff.data() + m_shadowTail, src.data(), static_cast<std::size_t>(writeSize));
          m_shadowTail += writeSize;
          m_bytesWritten += writeSize;

          src = src.subspan(static_cast<std::size_t>(writeSize));
        } else {
          // In this regard, the tail is behind the head
          const auto spaceLeft = m_parent.m_cachedHead - m_shadowTail - 1;
          const auto writeSize = std::min(sz, spaceLeft);

          std::memcpy(m_parent.m_buff.data() + m_shadowTail, src.data(), static_cast<std::size_t>(writeSize));
          m_shadowTail += writeSize;
          m_bytesWritten += writeSize;

          src = src.subspan(static_cast<std::size_t>(writeSize));
        }
      }

      return true;
    }
    [[nodiscard]] std::size_t bytes_written() const override { return m_bytesWritten; }

  private:
    RingBuffer& m_parent;
    std::ptrdiff_t m_shadowTail{ 0 };
    std::size_t m_bytesWritten{ 0 };
  };

public:
  RingBuffer() = default;

  // We don't want copying
  RingBuffer(const RingBuffer&) = delete;
  RingBuffer& operator=(const RingBuffer&) = delete;

  // We don't want moving either.
  RingBuffer(RingBuffer&&) = delete;
  RingBuffer& operator=(RingBuffer&&) = delete;

  [[nodiscard]] std::unique_ptr<ByteBuffer::Reader> get_reader() override { return std::make_unique<Reader>(*this); }

  [[nodiscard]] std::unique_ptr<ByteBuffer::Writer> get_writer() override { return std::make_unique<Writer>(*this); }
  [[nodiscard]] std::size_t capacity() override { return N; }
};

}