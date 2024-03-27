#include <doctest/doctest.h>
#include <functional>
#include <hage/logging.hpp>

#include <latch>
#include <thread>

TEST_CASE("testing syncronized logger")
{
  using namespace hage::literals;
  hage::Logger<hage::RingBuffer<4096>> logger;
  std::latch ready(2);

  constexpr std::int64_t TIMES = 10;

  std::thread writer([&logger, &ready]() {
    ready.arrive_and_wait();
    // std::string str("I hope this works!");
    std::int64_t i = 0;
    while (i < TIMES) {
      logger.log("Here we are: {} and my name is: {}", i, "hermes");
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
}

TEST_CASE("testing syncronized logger, compile_time passing")
{
  using namespace hage::literals;
  hage::Logger<hage::RingBuffer<4096>> logger;
  std::latch ready(2);

  constexpr std::int64_t TIMES = 10;

  std::thread writer([&logger, &ready]() {
    ready.arrive_and_wait();
    // std::string str("I hope this works!");
    std::int64_t i = 0;
    while (i < TIMES) {
      logger.log("Here we are: {} and my name is: {}", i, "hermes");
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
}
