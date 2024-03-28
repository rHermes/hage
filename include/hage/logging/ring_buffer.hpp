#pragma once

#include "../misc.hpp"

#include <span>

namespace hage {
// This is a rather dumb container, but it will work for most
// of our tests.
template<std::ptrdiff_t N>
class RingBuffer
{

  alignas(details::destructive_interference_size) std::atomic<std::ptrdiff_t> m_head{ 0 };
  alignas(details::destructive_interference_size) std::ptrdiff_t m_cachedHead{ 0 };

  alignas(details::destructive_interference_size) std::atomic<std::ptrdiff_t> m_tail{ 0 };
  alignas(details::destructive_interference_size) std::ptrdiff_t m_cachedTail{ 0 };

  alignas(details::destructive_interference_size) std::ptrdiff_t m_shadowHead{ 0 };
  alignas(details::destructive_interference_size) std::ptrdiff_t m_shadowTail{ 0 };

  alignas(details::destructive_interference_size) std::array<std::byte, N + 1> m_buff;

public:
  RingBuffer() = default;

  // We don't want copying
  RingBuffer(const RingBuffer&) = delete;
  RingBuffer& operator=(const RingBuffer&) = delete;

  // We don't want moving either.
  RingBuffer(RingBuffer&&) = delete;
  RingBuffer& operator=(RingBuffer&&) = delete;

  bool read(std::span<std::byte> dst)
  {
    while (!dst.empty()) {
      const auto sz = static_cast<std::ptrdiff_t>(dst.size_bytes());
      if (m_shadowHead == N + 1)
        m_shadowHead = 0;

      if (m_shadowHead == m_cachedTail) {
        m_cachedTail = m_tail.load(std::memory_order::acquire);
        if (m_shadowHead == m_cachedTail)
          return false;
      }

      if (m_shadowHead < m_cachedTail) {
        const auto spaceLeft = m_cachedTail - m_shadowHead;
        const auto readSize = std::min(sz, spaceLeft);

        std::memcpy(dst.data(), m_buff.data() + m_shadowHead, static_cast<std::size_t>(readSize));
        m_shadowHead += readSize;

        dst = dst.subspan(static_cast<std::size_t>(readSize));
      } else {
        const auto spaceLeft = N + 1 - m_shadowHead;
        const auto readSize = std::min(sz, spaceLeft);

        std::memcpy(dst.data(), m_buff.data() + m_shadowHead, static_cast<std::size_t>(readSize));
        m_shadowHead += readSize;

        dst = dst.subspan(static_cast<std::size_t>(readSize));
      }
    }

    return true;
  }

  bool write(std::span<const std::byte> src)
  {
    while (!src.empty()) {
      const auto sz = static_cast<std::ptrdiff_t>(src.size_bytes());

      if (m_shadowTail == N + 1) {
        if (m_cachedHead == 0) {
          m_cachedHead = m_head.load(std::memory_order::acquire);
          if (m_cachedHead == 0)
            return false;
        }
        m_shadowTail = 0;
      } else if (m_shadowTail + 1 == m_cachedHead) {
        m_cachedHead = m_head.load(std::memory_order::acquire);
        if (m_shadowTail + 1 == m_cachedHead)
          return false;
      }

      if (m_cachedHead <= m_shadowTail) {
        const auto spaceLeft = N + 1 - m_shadowTail;
        const auto writeSize = std::min(sz, spaceLeft);

        std::memcpy(m_buff.data() + m_shadowTail, src.data(), static_cast<std::size_t>(writeSize));
        m_shadowTail += writeSize;

        src = src.subspan(static_cast<std::size_t>(writeSize));
      } else {
        // In this regard, the tail is behind the head
        const auto spaceLeft = m_cachedHead - m_shadowTail - 1;
        const auto writeSize = std::min(sz, spaceLeft);

        std::memcpy(m_buff.data() + m_shadowTail, src.data(), static_cast<std::size_t>(writeSize));
        m_shadowTail += writeSize;

        src = src.subspan(static_cast<std::size_t>(writeSize));
      }
    }

    return true;
  }

  void start_read() { m_shadowHead = m_head.load(std::memory_order::relaxed); }
  void finish_read() { m_head.store(m_shadowHead, std::memory_order::release); }
  void cancel_read() {}

  void start_write() { m_shadowTail = m_tail.load(std::memory_order::relaxed); }
  void finish_write() { m_tail.store(m_shadowTail, std::memory_order::release); }
  void cancel_write() {}
};

}