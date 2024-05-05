#pragma once

#include <atomic>
#include <optional>
#include <thread>

#include "concepts.hpp"

namespace hage {

template<detail::AtomicFloat T>
struct atomic<T>
{
private:
  std::atomic<T> m_atomic;

public:
  using value_type = T;
  using difference_type = T;

  // we first remove all operators.
  atomic() = default;
  ~atomic() noexcept = default;
  atomic(const atomic&) = delete;
  atomic& operator=(const atomic&) = delete;
  atomic& operator=(const atomic&) volatile = delete;

  constexpr atomic(T i) noexcept
    : m_atomic(i)
  {
  }

  operator T() const noexcept { return load(); }
  operator T() const volatile noexcept { return load(); }

  T operator=(T i) noexcept
  {
    store(i);
    return i;
  }
  T operator=(T i) volatile noexcept
  {
    store(i);
    return i;
  }

  bool is_lock_free() const noexcept { return m_atomic.is_lock_free(); }
  bool is_lock_free() const volatile noexcept { return m_atomic.is_lock_free(); }

  static constexpr bool is_always_lock_free = std::atomic<T>::is_always_lock_free;

  void store(T desired, std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    m_atomic.store(desired, order);
  }
  void store(T desired, std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    m_atomic.store(desired, order);
  }

  T load(std::memory_order order = std::memory_order::seq_cst) const noexcept { return m_atomic.load(order); }
  T load(std::memory_order order = std::memory_order::seq_cst) const volatile noexcept { return m_atomic.load(order); }

  T exchange(T desired, std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    return m_atomic.exchange(desired, order);
  }

  T exchange(T desired, std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    return m_atomic.exchange(desired, order);
  }

  bool compare_exchange_weak(T& expected, T desired, std::memory_order success, std::memory_order failure) noexcept
  {
    return m_atomic.compare_exchange_weak(expected, desired, success, failure);
  }
  bool compare_exchange_weak(T& expected,
                             T desired,
                             std::memory_order success,
                             std::memory_order failure) volatile noexcept
  {
    return m_atomic.compare_exchange_weak(expected, desired, success, failure);
  }

  bool compare_exchange_weak(T& expected, T desired, std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    return m_atomic.compare_exchange_weak(expected, desired, order);
  }

  bool compare_exchange_weak(T& expected,
                             T desired,
                             std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    return m_atomic.compare_exchange_weak(expected, desired, order);
  }

  bool compare_exchange_strong(T& expected, T desired, std::memory_order success, std::memory_order failure) noexcept
  {
    return m_atomic.compare_exchange_strong(expected, desired, success, failure);
  }
  bool compare_exchange_strong(T& expected,
                               T desired,
                               std::memory_order success,
                               std::memory_order failure) volatile noexcept
  {
    return m_atomic.compare_exchange_strong(expected, desired, success, failure);
  }

  bool compare_exchange_strong(T& expected, T desired, std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    return m_atomic.compare_exchange_strong(expected, desired, order);
  }

  bool compare_exchange_strong(T& expected,
                               T desired,
                               std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    return m_atomic.compare_exchange_strong(expected, desired, order);
  }

  T fetch_add(T arg, std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    return m_atomic.fetch_add(arg, order);
  }
  T fetch_add(T arg, std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    return m_atomic.fetch_add(arg, order);
  }

  T fetch_sub(T arg, std::memory_order order = std::memory_order::seq_cst) noexcept
  {
    return m_atomic.fetch_sub(arg, order);
  }
  T fetch_sub(T arg, std::memory_order order = std::memory_order::seq_cst) volatile noexcept
  {
    return m_atomic.fetch_sub(arg, order);
  }

  T operator+=(T arg) noexcept { return m_atomic.operator+=(arg); }
  T operator+=(T arg) volatile noexcept { return m_atomic.operator+=(arg); }

  T operator-=(T arg) noexcept { return m_atomic.operator-=(arg); }
  T operator-=(T arg) volatile noexcept { return m_atomic.operator-=(arg); }

  // Ok, here starts the functions that I overload.

  void wait(T old, std::memory_order order = std::memory_order::seq_cst) const noexcept { m_atomic.wait(old, order); };
  void wait(T old, std::memory_order order = std::memory_order::seq_cst) const volatile noexcept
  {
    m_atomic.wait(old, order);
  }

  // TODO(rHermes): Implement these better, once I have an actual futex, waitaddress implementation.
  // We also need a spinlock implementation, in the case where futex two is not implemented.
  void notify_one() noexcept { m_atomic.notify_one(); };
  void notify_all() noexcept { m_atomic.notnotify_all(); }

  // These are my current implementations. They are very bad, but I will come back and improve them later. For now
  // I only need the semantics.
  T wait_value(T old, std::memory_order order = std::memory_order::seq_cst) const noexcept
  {
    T val = load(order);
    while (val == old) {
      std::this_thread::yield();
      val = load(order);
    }

    return val;
  }

  std::optional<T> try_wait(T old, std::memory_order order = std::memory_order::seq_cst) const noexcept
  {
    T val = load(order);
    if (val == old) {
      return std::nullopt;
    }

    return val;
  }

  template<typename Rep, typename Period>
  std::optional<T> wait_for(T old,
                            const std::chrono::duration<Rep, Period>& relTime,
                            std::memory_order order = std::memory_order::seq_cst) const
  {
    const auto startTs = std::chrono::steady_clock::now();

    T val = load(order);
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
  std::optional<T> wait_until(T old,
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
    requires std::predicate<P, T>
  T wait_with_predicate(P&& stop_waiting, std::memory_order order = std::memory_order::seq_cst) const
  {
    T val = load(order);
    while (!std::invoke(stop_waiting, val)) {
      val = wait_value(val, order);
    }
    return val;
  }

  // Timed, unspecified duration.
  template<class P>
    requires std::predicate<P, T>
  std::optional<T> try_wait_with_predicate(P&& stop_waiting, std::memory_order order = std::memory_order::seq_cst) const
  {
    T val = load(order);
    if (!std::invoke(stop_waiting, val))
      return std::nullopt;

    return val;
  }

  template<typename P, typename Rep, typename Period>
    requires std::predicate<P, T>
  std::optional<T> wait_for_with_predicate(P&& stop_waiting,
                                           const std::chrono::duration<Rep, Period>& relTime,
                                           std::memory_order order = std::memory_order::seq_cst) const
  {
    const auto startTs = std::chrono::steady_clock::now();

    T val = load(order);
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
  std::optional<T> wait_until_with_predicate(P&& stop_waiting,
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