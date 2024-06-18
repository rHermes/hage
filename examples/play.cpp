#include <concepts>
#include <future>
#include <iostream>
#include <type_traits>
#include <vector>

template<bool b, typename T = void>
struct enable_biff
{};

template<typename T>
struct enable_biff<true, T>
{
  using type = T;
};

template<bool b, typename T>
using enable_biff_t = enable_biff<b, T>::type;

template<typename T>
concept is_vec2_compat = requires(T a) {
  a + a;

  a / a;

  std::cout << a;
};

template<is_vec2_compat T>
struct Vec2
{
  T x{ 0 };
  T y{ 0 };

  Vec2() = default;
  Vec2(T xx, T yy) : x{ xx }, y{ yy } {}

  template<typename U, enable_biff_t<std::is_same_v<T, std::common_type_t<T, U>>, bool> = true>
  Vec2(const Vec2<U>& other)
  {
    x = other.x;
    y = other.y;
  }

  template<typename U, enable_biff_t<std::is_same_v<T, std::common_type_t<T, U>>, bool> = true>
  Vec2& operator=(const Vec2<U>& other)
  {
    x = other.x;
    y = other.y;
    return *this;
  }

  friend Vec2 operator+(const Vec2& a, const Vec2& b) { return { a.x + b.x, a.y + b.y }; }

  friend Vec2 operator/(const Vec2& a, const T& other) { return { a.x / other, a.y / other }; }

  friend std::ostream& operator<<(std::ostream& out, const Vec2& o)
  {
    out << "Vec2{ .x = " << o.x << ", .y = " << o.y << "}";
    return out;
  }
};

int
main()
{

  Vec2<double> hello{ 0, 10 };
  Vec2<float> ohno{ 0.3f, 4.2f };

  std::cout << "Look at our vector: " << hello << "\n";
  std::cout << "rad man: " << (hello / 10.0) << "\n";

  return 0;
}
