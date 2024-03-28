#pragma once

#include <span>
#include <deque>
#include <mutex>

namespace hage {
/**
This is a reference implementation of the buffer concepts we implement better above. It's here to show an alternative
implementation to the one given above.
*/
class QueueBuffer
{
  std::mutex m_mtx;
  std::deque<std::byte> m_q;

  decltype(m_q)::iterator m_readLevel;
  decltype(m_q)::iterator m_writeLevel;

public:
  bool read(std::span<std::byte> dst)
  {
    if (std::distance(m_readLevel, m_q.end()) < static_cast<std::ptrdiff_t>(dst.size_bytes()))
      return false;

    const auto first = m_readLevel;
    const auto end = std::next(m_readLevel, static_cast<std::ptrdiff_t>(dst.size_bytes()));

    std::copy(first, end, dst.begin());

    m_readLevel = end;
    return true;
  }

  bool write(std::span<const std::byte> src)
  {
    m_q.insert(m_q.end(), src.begin(), src.end());
    return true;
  }

  void start_read()
  {
    m_mtx.lock();
    m_readLevel = m_q.begin();
  }
  void finish_read()
  {
    m_q.erase(m_q.begin(), m_readLevel);
    m_mtx.unlock();
  }
  void cancel_read() { m_mtx.unlock(); }

  void start_write()
  {
    m_mtx.lock();
    m_writeLevel = m_q.end();
  }
  void finish_write() { m_mtx.unlock(); }

  void cancel_write()
  {
    m_q.erase(m_writeLevel, m_q.end());
    m_mtx.unlock();
  }
};

}