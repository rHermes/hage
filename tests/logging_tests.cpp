#include <doctest/doctest.h>
#include <functional>
#include <hage/logging.hpp>
#include <hage/logging/console_sink.hpp>
#include <hage/logging/queue_buffer.hpp>
#include <hage/logging/ring_buffer.hpp>

#include <latch>
#include <thread>

using namespace hage::literals;

TEST_CASE_TEMPLATE("ByteBuffer tests", BufferType, hage::RingBuffer<4096>, hage::QueueBuffer)
{
  static_assert(std::derived_from<BufferType, hage::ByteBuffer>);
  BufferType buff;
  hage::ByteBuffer& buffer = buff;

  SUBCASE("A buffer should start empty")
  {
    auto reader = buffer.get_reader();
    std::array<std::byte, 1> testBuffer{};
    REQUIRE_FALSE(reader->read(testBuffer));
    REQUIRE(reader->commit());
  }

  SUBCASE("We should be able to read what is written, in the correct order")
  {
    auto in = hage::byte_array(1, 2, 3);
    auto writer = buffer.get_writer();

    REQUIRE(writer->write(in));

    SUBCASE("But not before commiting")
    {
      auto reader = buffer.get_reader();
      std::array<std::byte, 3> out{};
      REQUIRE_FALSE(reader->read(out));
    }

    REQUIRE(writer->commit());

    SUBCASE("We can read after writer commit")
    {
      // Without commit, we should be able to read again.
      for (int i = 0; i < 2; i++) {
        auto reader = buffer.get_reader();
        std::array<std::byte, 3> out{};
        REQUIRE(reader->read(out));
        REQUIRE(out == in);
      }

      {
        auto reader = buffer.get_reader();
        std::array<std::byte, 3> out{};
        REQUIRE(reader->read(out));
        REQUIRE(out == in);
        REQUIRE(reader->commit());
      }

      SUBCASE("We cannot read once a read has been commited")
      {
        auto reader = buffer.get_reader();
        std::array<std::byte, 3> out{};
        REQUIRE_FALSE(reader->read(out));
      }
    }

    // We should be able to write even after commit.
    REQUIRE(writer->write(in));
    REQUIRE(writer->commit());

    SUBCASE("We should be able to read multiple times, without commit")
    {
      auto reader = buffer.get_reader();
      std::array<std::byte, 3> out{};
      REQUIRE(reader->read(out));
      REQUIRE(out == in);
      REQUIRE(reader->read(out));
      REQUIRE(out == in);
      REQUIRE(reader->commit());
      REQUIRE_FALSE(reader->read(out));
    }

    SUBCASE("We should be able to read multiple times, commit")
    {
      auto reader = buffer.get_reader();
      std::array<std::byte, 3> out{};
      REQUIRE(reader->commit());

      REQUIRE(reader->read(out));
      REQUIRE(out == in);

      REQUIRE(reader->commit());

      REQUIRE(reader->read(out));
      REQUIRE(out == in);

      REQUIRE(reader->commit());
      REQUIRE_FALSE(reader->read(out));
    }
  }

  SUBCASE("A writer without commit should not happen")
  {
    {
      auto in = hage::byte_array(1, 2, 3);
      auto writer = buffer.get_writer();

      REQUIRE(writer->write(in));
    }

    auto reader = buffer.get_reader();
    std::array<std::byte, 1> out{};
    REQUIRE_FALSE(reader->read(out));
  }
}

TEST_CASE_TEMPLATE("Single produser, single consumer buffers", BufferType, hage::RingBuffer<4096>, hage::QueueBuffer)
{
  static_assert(std::derived_from<BufferType, hage::ByteBuffer>);
  BufferType buff;
  hage::ByteBuffer& buffer = buff;

  SUBCASE("Creating two readers should throw")
  {
    auto reader1 = buffer.get_reader();
    REQUIRE_THROWS(buffer.get_reader());
  }

  SUBCASE("Creating two writers should throw")
  {
    auto writer = buffer.get_writer();
    REQUIRE_THROWS(buffer.get_writer());
  }
}

TEST_CASE("RingBuffer")
{
  hage::RingBuffer<10> buffer;

  // We have some unique tests here, like being able to write a 100 zeros.
  SUBCASE("Writing larger than buffer size, should fail")
  {
    auto writer = buffer.get_writer();
    std::array<std::byte, 11> in{};
    CHECK_FALSE(writer->write(in));
  }

}


TEST_CASE("test async interface")
{
  hage::NullSink nullSink;
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &nullSink);

  REQUIRE(logger.try_log(hage::LogLevel::Warn, "This is a test: {}", 10));
  REQUIRE(logger.try_read_log());

  SUBCASE("Logger should return negative when trying to read from an empty log")
  {
    REQUIRE(!logger.try_read_log());
  }

  SUBCASE("Logger should default to info log level")
  {
    REQUIRE(logger.try_debug("This is a debug message: {} {}", 10, "Fun"));
    REQUIRE(logger.try_debug("This is a debug message: {} {}"_fmt, 10, "Fun"));
    REQUIRE(!logger.try_read_log());

    REQUIRE(logger.try_info("This is a debug message: {} {}", 10, "Fun"));
    REQUIRE(logger.try_info("This is a debug message: {} {}"_fmt, 10, "Fun"));
    REQUIRE(logger.try_read_log());
    REQUIRE(logger.try_read_log());
  }

  SUBCASE("Logger should filter on minimal log level")
  {
    logger.set_log(hage::LogLevel::Warn);

    REQUIRE(logger.try_info("This is a debug message: {} {}", 10, "Fun"));
    REQUIRE(logger.try_info("This is a debug message: {} {}"_fmt, 10, "Fun"));
    REQUIRE(!logger.try_read_log());
  }
}

TEST_CASE("testing syncronized logger")
{
  using namespace hage::literals;

  hage::ConsoleSink consoleSink;
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &consoleSink);
  std::latch ready(2);

  logger.set_log(hage::LogLevel::Debug);

  constexpr std::int64_t TIMES = 2;

  std::thread writer([&logger, &ready]() {
    ready.arrive_and_wait();
    // std::string str("I hope this works!");
    std::int64_t i = 0;
    while (i < TIMES) {
      logger.log(hage::LogLevel::Debug, "Here we are: {} and my name is: {}", i, "hermes");
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

  hage::ConsoleSink consoleSink;
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &consoleSink);
  std::latch ready(2);

  constexpr std::int64_t TIMES = 2;

  std::thread writer([&logger, &ready]() {
    ready.arrive_and_wait();
    // std::string str("I hope this works!");
    std::int64_t i = 0;
    while (i < TIMES) {
      logger.log(hage::LogLevel::Error, "Here we are: {} and my name is: {}", i, "hermes");
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
