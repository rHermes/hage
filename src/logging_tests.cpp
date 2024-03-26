#include <doctest/doctest.h>
#include <functional>
#include <hage/logging.hpp>

#include <latch>
#include <thread>

/*
template<typename... T>
struct type_list {};

template<typename F>
struct deduce {
//    TODO: specialize this for function pointers, member function pointers and define:
//    using type = type_list<R, Args...>;
};

template<typename F, typename R, typename... Args>
void actual_do_something_with(F f, type_list<R, Args...>) {
// ...
};

template<typename F>
void do_something_with(F f) {
actual_do_something_with(f, deduce<F>::type{});
}
*/

template<typename Buffer, typename Signature>
struct memberChainLogger;

template<typename Buffer, typename Class, typename... Args>
struct memberChainLogger<Buffer, void (Class::*)(Args...) const>
{
  using FPTR = void (Class::*)(Args...) const;
  bool operator()(Buffer& buffer)
  {
    FPTR funcPtr;
    auto good = hage::read_from_buffer(buffer, funcPtr);
    if (!good)
      return false;

    Class* tis;
    good = hage::read_from_buffer(buffer, tis);
    if (!good)
      return false;

    std::invoke(funcPtr, tis, hage::read_from_buffer<Args>(buffer)...);
    return true;
  };
};

template<typename Signature, typename Buffer>
struct pureChainLogger;

template<typename... Args, typename Buffer>
struct pureChainLogger<void(Args...), Buffer>
{
  using FPTR = std::add_pointer_t<void(Args...)>;
  bool operator()(Buffer& buffer)
  {
    FPTR funcPtr;
    auto good = hage::read_from_buffer(buffer, funcPtr);
    if (!good)
      return false;

    std::invoke(funcPtr, hage::read_from_buffer<Args>(buffer)...);
    return true;
  }
};

template<typename F, typename Buffer>
bool
chainLogger(Buffer& buffer)
{
  if constexpr (std::is_member_function_pointer_v<F>) {
    return std::invoke(memberChainLogger<Buffer, F>(), buffer);
  } else {
    return std::invoke(pureChainLogger<F, Buffer>(), buffer);
  }
}

TEST_CASE("testing syncronized logger")
{
  hage::Logger<hage::RingBuffer<4096>> logger;
  std::latch ready(2);

  constexpr std::int64_t TIMES = 100;

  std::int64_t counter = 0;
  auto checker = [&counter](std::int64_t i) {
    REQUIRE(counter == i);
    counter++;
  };

  std::thread writer([&logger, &ready, &checker]() {
    ready.arrive_and_wait();
    // std::string str("I hope this works!");
    std::int64_t i = 0;
    while (i < TIMES) {
      logger.log(chainLogger<decltype(&decltype(checker)::operator())>, &decltype(checker)::operator(), &checker, i);
      i++;
    }
  });

  std::thread reader([&logger, &ready]() {
    ready.arrive_and_wait();
    std::int64_t i = 0;
    while (i < TIMES) {
      logger.read_log();
      i++;
    }
  });

  writer.join();
  reader.join();

  INFO("Counter is ", counter);
}