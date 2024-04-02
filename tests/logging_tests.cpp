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
    REQUIRE_UNARY_FALSE(reader->read(testBuffer));
    REQUIRE_UNARY(reader->commit());
  }

  SUBCASE("We should be able to read what is written, in the correct order")
  {
    auto in = hage::byte_array(1, 2, 3);
    auto writer = buffer.get_writer();

    REQUIRE_UNARY(writer->write(in));

    SUBCASE("But not before commiting")
    {
      auto reader = buffer.get_reader();
      std::array<std::byte, 3> out{};
      REQUIRE_UNARY_FALSE(reader->read(out));
    }

    REQUIRE_UNARY(writer->commit());

    SUBCASE("We can read after writer commit")
    {
      // Without commit, we should be able to read again.
      for (int i = 0; i < 2; i++) {
        auto reader = buffer.get_reader();
        std::array<std::byte, 3> out{};
        REQUIRE_UNARY(reader->read(out));
        REQUIRE_EQ(out, in);
      }

      {
        auto reader = buffer.get_reader();
        std::array<std::byte, 3> out{};
        REQUIRE_UNARY(reader->read(out));
        REQUIRE_EQ(out, in);
        REQUIRE_UNARY(reader->commit());
      }

      SUBCASE("We cannot read once a read has been commited")
      {
        auto reader = buffer.get_reader();
        std::array<std::byte, 3> out{};
        REQUIRE_UNARY_FALSE(reader->read(out));
      }
    }

    // We should be able to write even after commit.
    REQUIRE_UNARY(writer->write(in));
    REQUIRE_UNARY(writer->commit());

    SUBCASE("We should be able to read multiple times, without commit")
    {
      auto reader = buffer.get_reader();
      std::array<std::byte, 3> out{};
      REQUIRE_UNARY(reader->read(out));
      REQUIRE_EQ(out, in);
      REQUIRE_UNARY(reader->read(out));
      REQUIRE_EQ(out, in);
      REQUIRE_UNARY(reader->commit());
      REQUIRE_UNARY_FALSE(reader->read(out));
    }

    SUBCASE("We should be able to read multiple times, commit")
    {
      auto reader = buffer.get_reader();
      std::array<std::byte, 3> out{};
      REQUIRE_UNARY(reader->commit());

      REQUIRE_UNARY(reader->read(out));
      REQUIRE_EQ(out, in);

      REQUIRE_UNARY(reader->commit());

      REQUIRE_UNARY(reader->read(out));
      REQUIRE_EQ(out, in);

      REQUIRE_UNARY(reader->commit());
      REQUIRE_UNARY_FALSE(reader->read(out));
    }
  }

  SUBCASE("A writer without commit should not happen")
  {
    {
      auto in = hage::byte_array(1, 2, 3);
      auto writer = buffer.get_writer();

      REQUIRE_UNARY(writer->write(in));
    }

    auto reader = buffer.get_reader();
    std::array<std::byte, 1> out{};
    REQUIRE_UNARY_FALSE(reader->read(out));
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
    REQUIRE_THROWS(std::ignore = buffer.get_reader());
  }

  SUBCASE("Creating two writers should throw")
  {
    auto writer = buffer.get_writer();
    REQUIRE_THROWS(std::ignore = buffer.get_writer());
  }
}

TEST_CASE("RingBuffer")
{
  constexpr std::size_t N = 10;
  hage::RingBuffer<N> buffer;

  // We have some unique tests here, like being able to write a 100 zeros.
  SUBCASE("Writing larger than buffer size, should fail")
  {
    auto writer = buffer.get_writer();
    std::array<std::byte, N + 1> in{};
    CHECK_UNARY_FALSE(writer->write(in));
  }

  // We have some unique tests here, like being able to write a 100 zeros.
  SUBCASE("Reading larger than buffer size, should fail")
  {
    auto reader = buffer.get_reader();
    std::array<std::byte, N + 1> out{};
    CHECK_UNARY_FALSE(reader->read(out));
  }

  // We are going to check that the writes fail across the style
  SUBCASE("Writign and reading should not work for all positions in the buffer")
  {
    auto writer = buffer.get_writer();
    auto reader = buffer.get_reader();
    for (std::size_t i = 0; i < N + 3; i++) {
      std::array<std::byte, N + 1> in = hage::byte_array(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
      CHECK_UNARY_FALSE(writer->write(in));

      std::array<std::byte, N + 1> out{};
      CHECK_UNARY_FALSE(reader->read(out));

      // Now we need to advance the position 1
      std::array<std::byte, 1> off{};
      CHECK_UNARY(writer->write(off));
      CHECK_UNARY(writer->commit());
      CHECK_UNARY(reader->read(off));
      CHECK_UNARY(reader->commit());
    }
  }

  // We are going to check that the writes fail across the style
  SUBCASE("Writing and reading should work for all positions in the buffer")
  {
    auto writer = buffer.get_writer();
    auto reader = buffer.get_reader();
    for (std::size_t i = 0; i < N + 3; i++) {
      std::array<std::byte, N> in = hage::byte_array(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
      CHECK_UNARY(writer->write(in));
      CHECK_UNARY(writer->commit());

      std::array<std::byte, N> out{};
      CHECK_UNARY(reader->read(out));
      CHECK_UNARY(reader->commit());
      CHECK_EQ(in, out);

      // Now we need to advance the position 1
      std::array<std::byte, 1> off{};
      CHECK_UNARY(writer->write(off));
      CHECK_UNARY(writer->commit());
      CHECK_UNARY(reader->read(off));
      CHECK_UNARY(reader->commit());
    }
  }
}

TEST_CASE("test async interface")
{
  hage::NullSink nullSink;
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &nullSink);

  REQUIRE_UNARY(logger.try_log(hage::LogLevel::Warn, "This is a test: {}", 10));
  REQUIRE_UNARY(logger.try_read_log());

  SUBCASE("Logger should return negative when trying to read from an empty log")
  {
    REQUIRE_UNARY_FALSE(logger.try_read_log());
  }

  SUBCASE("Logger should default to info log level")
  {
    REQUIRE_UNARY(logger.try_debug("This is a debug message: {} {}", 10, "Fun"));
    REQUIRE_UNARY(logger.try_debug("This is a debug message: {} {}"_fmt, 10, "Fun"));
    REQUIRE_UNARY(!logger.try_read_log());

    REQUIRE_UNARY(logger.try_info("This is a debug message: {} {}", 10, "Fun"));
    REQUIRE_UNARY(logger.try_info("This is a debug message: {} {}"_fmt, 10, "Fun"));
    REQUIRE_UNARY(logger.try_read_log());
    REQUIRE_UNARY(logger.try_read_log());
  }

  SUBCASE("Logger should filter on minimal log level")
  {
    logger.set_log(hage::LogLevel::Warn);

    REQUIRE_UNARY(logger.try_info("This is a debug message: {} {}", 10, "Fun"));
    REQUIRE_UNARY(logger.try_info("This is a debug message: {} {}"_fmt, 10, "Fun"));
    REQUIRE_UNARY(!logger.try_read_log());
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

  constexpr std::int64_t TIMES = 3;

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
