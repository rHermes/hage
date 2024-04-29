#include <chrono>
#include <filesystem>
#include <functional>
#include <latch>
#include <thread>

#include <hage/core/misc.hpp>

#include <hage/logging.hpp>
#include <hage/logging/file_sink.hpp>
#include <hage/logging/ring_buffer.hpp>
#include <hage/logging/vector_buffer.hpp>

#include <doctest/doctest.h>

#include "test_sink.hpp"
#include "test_utils.hpp"
#include <bitset>

using namespace hage::literals;

template<std::size_t N>
std::ostream&
operator<<(std::ostream& os, const std::array<std::byte, N>& value)
{
  for (std::size_t i = 0; i < N - 1; i++) {
    os << value[i] << " ";
  }
  os << value[N - 1];
  return os;
}

namespace doctest {
template<std::size_t N>
struct StringMaker<std::array<std::byte, N>>
{
  static String convert(const std::array<std::byte, N>& value)
  {
    std::stringstream buf;
    for (std::size_t i = 0; i < N - 1; i++) {
      // buf << std::bitset<8>(std::to_integer<int>(value[i])) << " ";
      buf << std::to_integer<int>(value[i]) << " ";
    }
    // buf << std::bitset<8>(std::to_integer<int>(value[N-1]));
    buf << std::to_integer<int>(value[N - 1]);
    return buf.str().c_str();
  }
};
}

TEST_SUITE_BEGIN("logging");

TEST_CASE_TEMPLATE("ByteBuffer tests", BufferType, hage::RingBuffer<50>, hage::RingBuffer<4096>, hage::VectorBuffer)
{
  static_assert(std::derived_from<BufferType, hage::ByteBuffer>);
  BufferType buff{};
  hage::ByteBuffer& buffer = buff;

  SUBCASE("A buffer should start empty")
  {
    const auto reader = buffer.get_reader();
    std::array<std::byte, 1> testBuffer{};
    REQUIRE_UNARY_FALSE(reader->read(testBuffer));
    REQUIRE_UNARY(reader->commit());
  }

  SUBCASE("We should be able to read what is written, in the correct order")
  {
    constexpr auto in = hage::byte_array(1, 2, 3);
    const auto writer = buffer.get_writer();

    REQUIRE_UNARY(writer->write(in));

    SUBCASE("But not before commiting")
    {
      const auto reader = buffer.get_reader();
      std::array<std::byte, 3> out{};
      REQUIRE_UNARY_FALSE(reader->read(out));
    }

    REQUIRE_UNARY(writer->commit());

    SUBCASE("We can read after writer commit")
    {
      // Without commit, we should be able to read again.
      for (int i = 0; i < 2; i++) {
        const auto reader = buffer.get_reader();
        std::array<std::byte, 3> out{};
        REQUIRE_UNARY(reader->read(out));
        REQUIRE_EQ(out, in);
      }

      {
        const auto reader = buffer.get_reader();
        std::array<std::byte, 3> out{};
        REQUIRE_UNARY(reader->read(out));
        REQUIRE_EQ(out, in);
        REQUIRE_UNARY(reader->commit());
      }

      SUBCASE("We cannot read once a read has been commited")
      {
        const auto reader = buffer.get_reader();
        std::array<std::byte, 3> out{};
        REQUIRE_UNARY_FALSE(reader->read(out));
      }
    }

    // We should be able to write even after commit.
    REQUIRE_UNARY(writer->write(in));
    REQUIRE_UNARY(writer->commit());

    SUBCASE("We should be able to read multiple times, without commit")
    {
      const auto reader = buffer.get_reader();
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
      const auto reader = buffer.get_reader();
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
      constexpr auto in = hage::byte_array(1, 2, 3);
      const auto writer = buffer.get_writer();

      REQUIRE_UNARY(writer->write(in));
    }

    const auto reader = buffer.get_reader();
    std::array<std::byte, 1> out{};
    REQUIRE_UNARY_FALSE(reader->read(out));
  }

  // Ok, we will now create one reader thread and one writer thread. We will
  SUBCASE("Reader and writer threads should work")
  {
    BufferType ringBuffer;
    std::latch ready(2);

    constexpr std::int64_t TIMES = 10000;

    static constexpr auto goodBytes = hage::byte_array(1, 2, 3, 4, 5, 6, 7, 8);

    std::thread writer([&ringBuffer, &ready]() {
      ready.arrive_and_wait();
      // std::string str("I hope this works!");
      std::int64_t i = 0;
      while (i < TIMES) {
        const auto writer = ringBuffer.get_writer();
        if (!writer->write(goodBytes))
          continue;

        REQUIRE_UNARY(writer->commit());
        i++;
      }
    });

    std::thread reader([&ringBuffer, &ready]() {
      ready.arrive_and_wait();
      std::int64_t i = 0;
      while (i < TIMES) {
        std::remove_cv_t<decltype(goodBytes)> srcBuf{};
        const auto reader = ringBuffer.get_reader();
        if (!reader->read(srcBuf))
          continue;

        REQUIRE_UNARY(reader->commit());
        REQUIRE_EQ(srcBuf, goodBytes);
        i++;
      }
    });

    writer.join();
    reader.join();
  }

  SUBCASE("Shadow readers should also work.")
  {
    BufferType ringBuffer;
    std::latch ready(2);

    constexpr std::int64_t TIMES = 10000;

    static constexpr auto goodBytes = hage::byte_array(1, 2, 3, 4, 5, 6, 7, 8);

    std::thread writer([&ringBuffer, &ready]() {
      ready.arrive_and_wait();
      // std::string str("I hope this works!");
      std::int64_t i = 0;
      const auto writer = ringBuffer.get_writer();
      while (i < TIMES) {
        if (!writer->write(goodBytes))
          continue;

        REQUIRE_UNARY(writer->commit());
        i++;
      }
    });

    std::thread reader([&ringBuffer, &ready]() {
      ready.arrive_and_wait();
      std::int64_t i = 0;
      const auto reader = ringBuffer.get_reader();
      while (i < TIMES) {
        std::remove_cv_t<decltype(goodBytes)> srcBuf{};

        if (!reader->read(srcBuf))
          continue;

        REQUIRE_UNARY(reader->commit());
        REQUIRE_EQ(srcBuf, goodBytes);
        i++;
      }
    });

    writer.join();
    reader.join();
  }
}

TEST_CASE_TEMPLATE("Single producer, single consumer buffers", BufferType, hage::RingBuffer<4096>, hage::VectorBuffer)
{
  static_assert(std::derived_from<BufferType, hage::ByteBuffer>);
  BufferType buff;
  hage::ByteBuffer& buffer = buff;

#ifdef HAGE_DEBUG
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
#endif
}

TEST_CASE("RingBuffer")
{
  constexpr std::size_t N = 10;
  hage::RingBuffer<N> buffer;

  // We have some unique tests here, like being able to write 100 zeros.
  SUBCASE("Writing larger than buffer size, should fail")
  {
    const auto writer = buffer.get_writer();
    std::array<std::byte, N + 1> in{};
    CHECK_UNARY_FALSE(writer->write(in));
  }

  // We have some unique tests here, like being able to write 100 zeros.
  SUBCASE("Reading larger than buffer size, should fail")
  {
    const auto reader = buffer.get_reader();
    std::array<std::byte, N + 1> out{};
    CHECK_UNARY_FALSE(reader->read(out));
  }

  // We are going to check that the writes fail across the style
  SUBCASE("Writing and reading should not work for all positions in the buffer")
  {
    const auto writer = buffer.get_writer();
    const auto reader = buffer.get_reader();
    for (std::size_t i = 0; i < N + 3; i++) {
      constexpr std::array<std::byte, N + 1> in = hage::byte_array(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
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
    const auto writer = buffer.get_writer();
    const auto reader = buffer.get_reader();
    for (std::size_t i = 0; i < N + 3; i++) {
      constexpr std::array<std::byte, N> in = hage::byte_array(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
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

TEST_CASE("logger should fail on too small a buffer")
{
  hage::NullSink sink;
  hage::RingBuffer<100> ringBuffer;
  REQUIRE_THROWS(hage::Logger(&ringBuffer, &sink));
}

TEST_CASE("logger should fail on too big a message")
{
  hage::NullSink sink;
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &sink, 500);

  std::string lel(600, 'l');
  REQUIRE_UNARY_FALSE(logger.try_error("{}", lel));
  REQUIRE_UNARY_FALSE(logger.try_error("{}"_fmt, lel));

  std::string power(200, 'w');
  REQUIRE_UNARY_FALSE(logger.try_error("{} {} {}", power, power, power));
  REQUIRE_UNARY_FALSE(logger.try_error("{} {} {}"_fmt, power, power, power));
}

TEST_CASE("allow logger to set max message size")
{
  hage::NullSink sink;
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &sink, 2000);

  std::string lel(600, 'l');
  REQUIRE_UNARY(logger.try_error("{}", lel));
  REQUIRE_UNARY(logger.try_read_log());
  REQUIRE_UNARY(logger.try_error("{}"_fmt, lel));
  REQUIRE_UNARY(logger.try_read_log());

  std::string power(200, 'w');
  REQUIRE_UNARY(logger.try_error("{} {} {}", power, power, power));
  REQUIRE_UNARY(logger.try_read_log());
  REQUIRE_UNARY(logger.try_error("{} {} {}"_fmt, power, power, power));
  REQUIRE_UNARY(logger.try_read_log());
}

TEST_CASE("test async interface")
{
  hage::test::TestSink sink;
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &sink);

  SUBCASE("Logger should return negative when trying to read from an empty log")
  {
    REQUIRE_UNARY_FALSE(logger.try_read_log());
    REQUIRE_UNARY(sink.empty());
  }

  SUBCASE("Logger should default to info log level")
  {
    REQUIRE_UNARY(logger.try_debug("This is a debug message: {} {}", 10, "Fun"));
    REQUIRE_UNARY(logger.try_debug("This is a debug message: {} {}"_fmt, 10, "Fun"));
    REQUIRE_UNARY_FALSE(logger.try_read_log());

    REQUIRE_UNARY(sink.empty());

    REQUIRE_UNARY(logger.try_info("This is a debug message: {} {}", 10, "Fun"));
    REQUIRE_UNARY(logger.try_info("This is a compile time debug message: {} {}"_fmt, 10, "Fan"));
    REQUIRE_UNARY(logger.try_read_log());
    REQUIRE_UNARY(logger.try_read_log());

    sink.require_info("This is a debug message: 10 Fun");
    sink.require_info("This is a compile time debug message: 10 Fan");
    REQUIRE_UNARY(sink.empty());
  }

  SUBCASE("Logger should filter on minimal log level")
  {
    logger.set_min_log_level(hage::LogLevel::Warn);

    REQUIRE_UNARY(logger.try_info("This is a debug message: {} {}", 10, "Fun"));
    REQUIRE_UNARY(logger.try_info("This is a debug message: {} {}"_fmt, 10, "Fun"));
    REQUIRE_UNARY_FALSE(logger.try_read_log());

    REQUIRE_UNARY(sink.empty());
  }

  SUBCASE("Logger normal format should send all message types")
  {
    logger.set_min_log_level(hage::LogLevel::Trace);

    REQUIRE_UNARY(logger.try_trace("trace {}", 1));
    REQUIRE_UNARY(logger.try_debug("debug {}", 2));
    REQUIRE_UNARY(logger.try_info("info {}", 3));
    REQUIRE_UNARY(logger.try_warn("warn {}", 4));
    REQUIRE_UNARY(logger.try_error("error {}", 5));
    REQUIRE_UNARY(logger.try_critical("critical {}", 6));

    for (std::size_t i = 0; i < 6; i++)
      REQUIRE_UNARY(logger.try_read_log());

    REQUIRE_UNARY_FALSE(logger.try_read_log());

    sink.require_trace("trace 1");
    sink.require_debug("debug 2");
    sink.require_info("info 3");
    sink.require_warn("warn 4");
    sink.require_error("error 5");
    sink.require_critical("critical 6");
    REQUIRE_UNARY(sink.empty());
  }

  SUBCASE("Logger compile time format should send all message types")
  {
    logger.set_min_log_level(hage::LogLevel::Trace);

    REQUIRE_UNARY(logger.try_trace("trace {}"_fmt, 1));
    REQUIRE_UNARY(logger.try_debug("debug {}"_fmt, 2));
    REQUIRE_UNARY(logger.try_info("info {}"_fmt, 3));
    REQUIRE_UNARY(logger.try_warn("warn {}"_fmt, 4));
    REQUIRE_UNARY(logger.try_error("error {}"_fmt, 5));
    REQUIRE_UNARY(logger.try_critical("critical {}"_fmt, 6));

    for (std::size_t i = 0; i < 6; i++)
      REQUIRE_UNARY(logger.try_read_log());

    REQUIRE_UNARY_FALSE(logger.try_read_log());

    sink.require_trace("trace 1");
    sink.require_debug("debug 2");
    sink.require_info("info 3");
    sink.require_warn("warn 4");
    sink.require_error("error 5");
    sink.require_critical("critical 6");
    REQUIRE_UNARY(sink.empty());
  }
}

TEST_CASE("Test syncronized interface")
{
  hage::test::TestSink sink;
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &sink, 100);

  // TODO(rHermes): Figure out how to test that
  SUBCASE("Logger should default to info log level")
  {
    logger.debug("This is a debug message: {} {}", 10, "Fun");
    logger.debug("This is a debug message: {} {}"_fmt, 10, "Fun");
    REQUIRE_UNARY(sink.empty());

    logger.info("This is a debug message: {} {}", 10, "Fun");
    logger.info("This is a compile time debug message: {} {}"_fmt, 10, "Fan");

    logger.read_log();
    logger.read_log();

    sink.require_info("This is a debug message: 10 Fun");
    sink.require_info("This is a compile time debug message: 10 Fan");
    REQUIRE_UNARY(sink.empty());
  }

  SUBCASE("Logger should filter on minimal log level")
  {
    logger.set_min_log_level(hage::LogLevel::Warn);

    logger.info("This is a debug message: {} {}", 10, "Fun");
    logger.info("This is a debug message: {} {}"_fmt, 10, "Fun");

    REQUIRE_UNARY(sink.empty());
  }

  SUBCASE("Logger normal format should send all message types")
  {
    logger.set_min_log_level(hage::LogLevel::Trace);

    logger.trace("trace {}", 1);
    logger.debug("debug {}", 2);
    logger.info("info {}", 3);
    logger.warn("warn {}", 4);
    logger.error("error {}", 5);
    logger.critical("critical {}", 6);

    for (std::size_t i = 0; i < 6; i++)
      logger.read_log();

    sink.require_trace("trace 1");
    sink.require_debug("debug 2");
    sink.require_info("info 3");
    sink.require_warn("warn 4");
    sink.require_error("error 5");
    sink.require_critical("critical 6");
    REQUIRE_UNARY(sink.empty());
  }

  SUBCASE("Logger compile time format should send all message types")
  {
    logger.set_min_log_level(hage::LogLevel::Trace);

    logger.trace("trace {}"_fmt, 1);
    logger.debug("debug {}"_fmt, 2);
    logger.info("info {}"_fmt, 3);
    logger.warn("warn {}"_fmt, 4);
    logger.error("error {}"_fmt, 5);
    logger.critical("critical {}"_fmt, 6);

    for (std::size_t i = 0; i < 6; i++)
      logger.read_log();

    sink.require_trace("trace 1");
    sink.require_debug("debug 2");
    sink.require_info("info 3");
    sink.require_warn("warn 4");
    sink.require_error("error 5");
    sink.require_critical("critical 6");
    REQUIRE_UNARY(sink.empty());
  }
}

TEST_CASE("MultiSink test")
{
  hage::test::TestSink sink1;
  hage::test::TestSink sink2;
  hage::MultiSink mSink{ &sink1, &sink2 };

  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &mSink);

  logger.error("test 123");
  logger.read_log();

  sink1.require_error("test 123");
  sink2.require_error("test 123");

  REQUIRE_UNARY(sink1.empty());
  REQUIRE_UNARY(sink2.empty());
}

TEST_CASE("FilterSink")
{
  hage::test::TestSink testSink;
  hage::FilterSink filterSink(&testSink, hage::LogLevel::Error);

  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &filterSink);

  logger.set_min_log_level(hage::LogLevel::Trace);

  logger.error("test 123");
  logger.trace("not through");
  logger.info("Not enough");
  logger.critical("this will make it");
  logger.read_log();
  logger.read_log();
  logger.read_log();
  logger.read_log();

  REQUIRE_UNARY_FALSE(logger.try_read_log());

  testSink.require_error("test 123");
  testSink.require_critical("this will make it");

  REQUIRE_UNARY(testSink.empty());
}

TEST_CASE("File sink")
{
  const hage::test::ScopedTempFile tempFile("file_sink_test.{}.txt");

  hage::FileSink fs(tempFile.path);
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &fs);

  logger.info("Hello there!: {}", 10);
  logger.read_log();

  // Windows has two digits fewer in it's clock than linux.
  REQUIRE_GE(fs.bytes_written(), 62);
  REQUIRE_LE(fs.bytes_written(), 64);
}

TEST_CASE("testing syncronized logger")
{
  using namespace hage::literals;

  hage::test::TestSink testSink;
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &testSink);
  std::latch ready(2);

  logger.set_min_log_level(hage::LogLevel::Debug);

  constexpr std::int64_t TIMES = 1000;

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

  for (std::int64_t i = 0; i < TIMES; i++) {
    testSink.require_debug(fmt::format("Here we are: {} and my name is: hermes", i));
  }

  REQUIRE_UNARY(testSink.empty());
}

TEST_CASE("testing syncronized logger, compile_time passing")
{
  using namespace hage::literals;

  hage::test::TestSink testSink;
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &testSink);
  std::latch ready(2);

  constexpr std::int64_t TIMES = 1000;

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

  for (std::int64_t i = 0; i < TIMES; i++) {
    testSink.require_error(fmt::format("Here we are: {} and my name is: hermes", i));
  }

  REQUIRE_UNARY(testSink.empty());
}

TEST_CASE("Testing timeout reading")
{
  using namespace hage::literals;
  using namespace std::chrono_literals;

  hage::test::TestSink testSink;
  hage::RingBuffer<4096> ringBuffer;
  hage::Logger logger(&ringBuffer, &testSink);

  REQUIRE_UNARY_FALSE(logger.read_log(10ms));
  logger.info("Warning!");
}

TEST_SUITE_END();