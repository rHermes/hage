#pragma once
#include <cstddef>
#include <new>
#include <utility>

namespace hage {

namespace details {

template<std::size_t ExpectedSize, std::size_t ActualSize, std::size_t ExpectedAlignment, std::size_t ActualAlignment>
constexpr inline void
compare_size()
{
  static_assert(ExpectedSize == ActualSize, "The size for the ForwardDeclareStorage is wrong");
  static_assert(ExpectedAlignment == ActualAlignment, "The alignment for the ForwardDeclareStorage is wrong");
}

} // namespace details

// Inspired by:
// https://probablydance.com/2013/10/05/type-safe-pimpl-implementation-without-overhead/

template<typename T, std::size_t Size, std::size_t Alignment>
class ForwardDeclaredStorage
{
private:
  alignas(Alignment) std::byte m_data[Size]{};

  constexpr T& get() { return *std::launder(reinterpret_cast<T*>(&m_data)); }
  constexpr const T& get() const { return *std::launder(reinterpret_cast<const T*>(&m_data)); }

public:
  constexpr ForwardDeclaredStorage() { ::new (m_data) T; }

  constexpr ~ForwardDeclaredStorage()
  {
    details::compare_size<Size, sizeof(T), Alignment, alignof(T)>();
    get().~T();
  }

  template<typename... Args>
  constexpr explicit ForwardDeclaredStorage(std::in_place_t, Args&&... args)
  {
    ::new (m_data) T(std::forward<Args>(args)...);
  }

  constexpr ForwardDeclaredStorage(ForwardDeclaredStorage&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
  {
    ::new (m_data) T(std::move(other.get()));
  }
  constexpr ForwardDeclaredStorage(const ForwardDeclaredStorage& other) { ::new (m_data) T(other.get()); }

  explicit constexpr ForwardDeclaredStorage(const T& other) { ::new (m_data) T(other); }
  constexpr explicit ForwardDeclaredStorage(T&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
  {
    ::new (m_data) T(std::move(other));
  }

  constexpr ForwardDeclaredStorage& operator=(const ForwardDeclaredStorage& other)
  {
    get() = other.get();
    return *this;
  }

  constexpr ForwardDeclaredStorage& operator=(const T& other)
  {
    get() = other;
    return *this;
  }

  constexpr ForwardDeclaredStorage& operator=(T&& other) noexcept(std::is_nothrow_move_assignable_v<T>)
  {
    get() = std::move(other);
    return *this;
  }

  constexpr ForwardDeclaredStorage& operator=(ForwardDeclaredStorage&& other) noexcept(
    std::is_nothrow_move_assignable_v<T>)
  {
    get() = std::move(other.get());
    return *this;
  }

  constexpr T* operator->() { return &get(); }
  constexpr const T* operator->() const { return &get(); };

  constexpr T& operator*() { return get(); }
  constexpr const T& operator*() const { return get(); }
};

} // namespace hage