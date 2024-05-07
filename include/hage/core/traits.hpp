#pragma once
#include <type_traits>

namespace hage {
template<typename...>
struct all_same : std::true_type
{};

template<typename T, typename... Us>
struct all_same<T, Us...> : std::conjunction<std::is_same<T, Us>...>
{};

template<typename... Ts>
inline constexpr bool all_same_v = all_same<Ts...>::value;

/*
template<typename... T>
inline constexpr bool all_same = (... && std::same_as<std::tuple_element_t<0, std::tuple<T...>>, T>);

This is an interesting way to do it, let's try to figure out why it works.
*/
} // namespace hage
