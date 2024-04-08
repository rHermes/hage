#pragma once

#include <atomic>
#include <thread>

namespace hage {

template<typename T>
struct atomic<T*>
{
private:
  std::atomic<T*> m_atomic;

  using TP = T*;

public:
  using value_type = TP;
  using different_type = std::ptrdiff_t;

  // we first remove all operators.
  atomic() = default;
  ~atomic() noexcept = default;
  atomic(const atomic&) = delete;
  atomic& operator=(const atomic&) = delete;
  atomic& operator=(const atomic&) volatile = delete;

  constexpr atomic(TP p) noexcept
    : m_atomic(p)
  {
  }

  operator TP() const noexcept { return load(); }
  operator TP() const volatile noexcept { return load(); }

  TP operator=(TP i) noexcept
  {
    store(i);
    return i;
  }
  TP operator=(TP i) volatile noexcept
  {
    store(i);
    return i;
  }

  bool is_lock_free() const noexcept { return m_atomic.is_lock_free(); }
  bool is_lock_free() const volatile noexcept { return m_atomic.is_lock_free(); }

  static constexpr bool is_always_lock_free = std::atomic<TP>::is_always_lock_free;

  TP operator++(int) noexcept { return fetch_add(1); }
  TP operator++(int) volatile noexcept { return fetch_add(1); }
  TP operator--(int) noexcept { return fetch_sub(1); }
  TP operator--(int) volatile noexcept { return fetch_sub(1); }

  TP operator++() noexcept { return m_atomic.operator++(); }
  TP operator++() volatile noexcept { return m_atomic.operator++(); }

  TP operator--() noexcept { return m_atomic.operator--(); }
  TP operator--() volatile noexcept { return m_atomic.operator--(); }

  TP operator+=(std::ptrdiff_t arg) noexcept { return m_atomic.operator+=(arg); }
  TP operator+=(std::ptrdiff_t arg) volatile noexcept { return m_atomic.operator+=(arg); }

  TP operator-=(std::ptrdiff_t arg) noexcept { return m_atomic.operator-=(arg); }
  TP operator-=(std::ptrdiff_t arg) volatile noexcept { return m_atomic.operator-=(arg); }

  void store(TP desired, std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    m_atomic.store(desired, order);
  }

  void store(TP desired, std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    m_atomic.store(desired, order);
  }

  TP load(std::memory_order order = std::memory_order::seq_cst) noexcept { return m_atomic.load(order); }
  TP load(std::memory_order order = std::memory_order::seq_cst) volatile noexcept { return m_atomic.load(order); }

  TP exchange(TP desired, std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    return m_atomic.exchange(desired, order);
  }

  T exchange(TP desired, std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    return m_atomic.exchange(desired, order);
  }

  bool compare_exchange_weak(TP& expected, TP desired, std::memory_order success, std::memory_order failure) noexcept
  {
    return m_atomic.compare_exchange_weak(expected, desired, success, failure);
  }
  bool compare_exchange_weak(TP& expected,
                             TP desired,
                             std::memory_order success,
                             std::memory_order failure) volatile noexcept
  {
    return m_atomic.compare_exchange_weak(expected, desired, success, failure);
  }

  bool compare_exchange_weak(TP& expected, TP desired, std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    return m_atomic.compare_exchange_weak(expected, desired, order);
  }

  bool compare_exchange_weak(TP& expected,
                             TP desired,
                             std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    return m_atomic.compare_exchange_weak(expected, desired, order);
  }

  bool compare_exchange_strong(TP& expected, TP desired, std::memory_order success, std::memory_order failure) noexcept
  {
    return m_atomic.compare_exchange_strong(expected, desired, success, failure);
  }
  bool compare_exchange_strong(TP& expected,
                               TP desired,
                               std::memory_order success,
                               std::memory_order failure) volatile noexcept
  {
    return m_atomic.compare_exchange_strong(expected, desired, success, failure);
  }

  bool compare_exchange_strong(TP& expected, TP desired, std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    return m_atomic.compare_exchange_strong(expected, desired, order);
  }

  bool compare_exchange_strong(TP& expected,
                               TP desired,
                               std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    return m_atomic.compare_exchange_strong(expected, desired, order);
  }

  TP fetch_add(std::ptrdiff_t arg, std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    return m_atomic.fetch_add(arg, order);
  }
  TP fetch_add(std::ptrdiff_t arg, std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    return m_atomic.fetch_add(arg, order);
  }

  TP fetch_sub(std::ptrdiff_t arg, std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    return m_atomic.fetch_sub(arg, order);
  }
  TP fetch_sub(std::ptrdiff_t arg, std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    return m_atomic.fetch_sub(arg, order);
  }

  // Ok, here starts the functions that I overload.

  void wait(TP old, std::memory_order order = std::memory_order::seq_cst) const noexcept { m_atomic.wait(old, order); };
  void wait(TP old, std::memory_order order = std::memory_order::seq_cst) const volatile noexcept
  {
    m_atomic.wait(old, order);
  }

  // TODO(rHermes): Implement these better, once I have an actual futex, waitaddress implementation.
  // We also need a spinlock implementation, in the case where futex two is not implemented.
  void notify_one() noexcept { m_atomic.notify_one(); };
  void notify_all() noexcept { m_atomic.notnotify_all(); }

  // These are my current implementations. They are very bad, but I will come back and improve them later. For now
  // I only need the semantics.
  TP wait_value(TP old, std::memory_order order = std::memory_order::seq_cst) const noexcept
  {
    TP val = load(order);
    while (val == old) {
      std::this_thread::yield();
      val = load(order);
    }

    return val;
  }

  std::optional<TP> try_wait(TP old, std::memory_order order = std::memory_order::seq_cst) const noexcept
  {
    TP val = load(order);
    if (val == old) {
      return std::nullopt;
    }

    return val;
  }

  template<typename Rep, typename Period>
  std::optional<TP> wait_for(TP old,
                             const std::chrono::duration<Rep, Period>& relTime,
                             std::memory_order order = std::memory_order::seq_cst) const
  {
    const auto startTs = std::chrono::steady_clock::now();

    TP val = load(order);
    while (val == old) {
      std::this_thread::yield();

      const auto dur = std::chrono::steady_clock::now() - startTs;
      if (dur < relTime)
        return std::nullopt;

      val = load(order);
    }

    return val;
  }

  template<typename Clock, typename Duration>
  std::optional<TP> wait_until(TP old,
                               const std::chrono::time_point<Clock, Duration>& absTime,
                               std::memory_order order = std::memory_order::seq_cst) const
  {
    const auto startTs = std::chrono::system_clock::now();
    if (absTime < startTs)
      return std::nullopt;

    return wait_for(old, absTime - startTs, order);
  }

  // predicate versions of the previous.
  template<typename P>
    requires std::predicate<P, TP>
  TP wait_with_predicate(P&& stop_waiting, std::memory_order order = std::memory_order::seq_cst) const
  {
    TP val = load(order);
    while (!std::invoke(stop_waiting, val)) {
      val = wait_value(val, order);
    }
    return val;
  }

  // Timed, unspecified duration.
  template<class P>
    requires std::predicate<P, TP>
  std::optional<TP> try_wait_with_predicate(P&& stop_waiting,
                                            std::memory_order order = std::memory_order::seq_cst) const
  {
    TP val = load(order);
    if (!std::invoke(stop_waiting, val))
      return std::nullopt;

    return val;
  }

  template<typename P, typename Rep, typename Period>
    requires std::predicate<P, TP>
  std::optional<TP> wait_for_with_predicate(P&& stop_waiting,
                                            const std::chrono::duration<Rep, Period>& relTime,
                                            std::memory_order order = std::memory_order::seq_cst) const
  {
    const auto startTs = std::chrono::steady_clock::now();

    TP val = load(order);
    while (!std::invoke(stop_waiting, val)) {
      std::this_thread::yield();

      const auto dur = std::chrono::steady_clock::now() - startTs;
      if (dur < relTime)
        return std::nullopt;

      val = load(order);
    }

    return val;
  }

  template<typename P, typename Clock, typename Duration>
  std::optional<TP> wait_until_with_predicate(P&& stop_waiting,
                                              const std::chrono::time_point<Clock, Duration>& absTime,
                                              std::memory_order order = std::memory_order::seq_cst) const
  {
    const auto startTs = std::chrono::system_clock::now();
    if (absTime < startTs)
      return std::nullopt;

    return wait_for_with_predicate(std::forward<P>(stop_waiting), absTime - startTs, order);
  }
};

}