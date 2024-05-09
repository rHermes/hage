#pragma once

#include <atomic>
#include <thread>

namespace hage {
class atomic_flag
{
  std::atomic_flag m_flag;

public:
  atomic_flag() noexcept = default;
  ~atomic_flag() noexcept = default;
  atomic_flag(const atomic_flag&) = delete;
  atomic_flag& operator=(const atomic_flag&) = delete;
  atomic_flag& operator=(const atomic_flag&) volatile = delete;

  bool test_and_set(std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    return m_flag.test_and_set(order);
  }

  bool test_and_set(std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    return m_flag.test_and_set(order);
  }

  bool test(std::memory_order order = std::memory_order::seq_cst) const noexcept { return m_flag.test(order); }
  bool test(std::memory_order order = std::memory_order::seq_cst) const volatile noexcept { return m_flag.test(order); }

  void clear(std::memory_order order = std::memory_order::seq_cst) noexcept { return m_flag.clear(order); }
  void clear(std::memory_order order = std::memory_order::seq_cst) volatile noexcept { return m_flag.clear(order); }

  void wait(bool old, std::memory_order order = std::memory_order::seq_cst) const noexcept { m_flag.wait(old, order); };

  // TODO(rHermes): Implement these better, once I have an actual futex, waitaddress implementation.
  // We also need a spinlock implementation, in the case where futex two is not implemented.
  void notify_one() noexcept { m_flag.notify_one(); };
  void notify_all() noexcept { m_flag.notify_all(); }
};

} // namespace hage