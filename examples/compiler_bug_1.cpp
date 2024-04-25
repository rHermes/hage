
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <latch>
#include <memory>
#include <span>
#include <thread>

template<typename... T>
constexpr std::array<std::byte, sizeof...(T)>
byte_array(T... a)
{
  return { static_cast<std::byte>(a)... };
}

class ByteBuffer
{
public:
  virtual ~ByteBuffer() = default;

  class Reader
  {
  public:
    virtual ~Reader() = default;
    virtual bool read(std::span<std::byte> dst) = 0;
    virtual bool commit() = 0;
    [[nodiscard]] virtual std::size_t bytes_read() const = 0;
  };

  [[nodiscard]] virtual std::unique_ptr<Reader> get_reader() = 0;

  class Writer
  {
  public:
    virtual ~Writer() = default;
    virtual bool write(std::span<const std::byte> src) = 0;
    virtual bool commit() = 0;
    [[nodiscard]] virtual std::size_t bytes_written() const = 0;
  };

  [[nodiscard]] virtual std::unique_ptr<Writer> get_writer() = 0;

  [[nodiscard]] virtual std::size_t capacity() = 0;
};

template<std::ptrdiff_t N>
class RingBuffer final : public ByteBuffer
{

  alignas(64) std::atomic<std::ptrdiff_t> m_head{ 0 };
  alignas(64) std::ptrdiff_t m_cachedHead{ 0 };

  alignas(64) std::atomic<std::ptrdiff_t> m_tail{ 0 };
  alignas(64) std::ptrdiff_t m_cachedTail{ 0 };

  alignas(64) std::array<std::byte, N + 1> m_buff{};

  alignas(64) std::atomic_flag m_hasReader;
  alignas(64) std::atomic_flag m_hasWriter;

  class Reader final : public ByteBuffer::Reader
  {
  public:
    explicit Reader(RingBuffer& parent)
      : m_parent{ parent }
    {
      if (m_parent.m_hasReader.test_and_set(std::memory_order::acq_rel))
        throw std::runtime_error("We can only have one concurrent reader for RingBuffer");

      m_shadowHead = m_parent.m_head.load(std::memory_order::relaxed);
    }

    ~Reader() override { m_parent.m_hasReader.clear(std::memory_order::release); }

    bool read(std::span<std::byte> dst) override
    {
      if (N < dst.size_bytes())
        return false;

      while (!dst.empty()) {
        const auto sz = static_cast<std::ptrdiff_t>(dst.size_bytes());

        if (m_shadowHead == m_parent.m_cachedTail) {
          m_parent.m_cachedTail = m_parent.m_tail.load(std::memory_order::acquire);
          if (m_shadowHead == m_parent.m_cachedTail)
            return false;
        }

        if (m_shadowHead == N + 1)
          m_shadowHead = 0;

        if (m_shadowHead == m_parent.m_cachedTail) {
          return false;
        }

        if (m_shadowHead <= m_parent.m_cachedTail) {
          const auto spaceLeft = m_parent.m_cachedTail - m_shadowHead;
          const auto readSize = std::min(sz, spaceLeft);

          std::memcpy(dst.data(), m_parent.m_buff.data() + m_shadowHead, static_cast<std::size_t>(readSize));
          m_shadowHead += readSize;
          m_bytesRead += readSize;

          dst = dst.subspan(static_cast<std::size_t>(readSize));
        } else {
          const auto spaceLeft = N + 1 - m_shadowHead;
          const auto readSize = std::min(sz, spaceLeft);

          std::memcpy(dst.data(), m_parent.m_buff.data() + m_shadowHead, static_cast<std::size_t>(readSize));
          m_shadowHead += readSize;
          m_bytesRead += readSize;

          dst = dst.subspan(static_cast<std::size_t>(readSize));
        }
      }

      return true;
    }

    bool commit() override
    {
      m_parent.m_head.store(m_shadowHead, std::memory_order::release);
      return true;
    }
    [[nodiscard]] std::size_t bytes_read() const override { return m_bytesRead; }

  private:
    RingBuffer& m_parent;
    std::ptrdiff_t m_shadowHead;
    std::size_t m_bytesRead{ 0 };
  };

  class Writer final : public ByteBuffer::Writer
  {
  public:
    explicit Writer(RingBuffer& parent)
      : m_parent{ parent }
    {
      if (m_parent.m_hasWriter.test_and_set(std::memory_order::acq_rel))
        throw std::runtime_error("We can only have one concurrent writer for RingBuffer");

      m_shadowTail = m_parent.m_tail.load(std::memory_order::relaxed);
    }

    ~Writer() override { m_parent.m_hasWriter.clear(std::memory_order::release); }

    bool commit() override
    {
      m_parent.m_tail.store(m_shadowTail, std::memory_order::release);
      return true;
    }

    bool write(std::span<const std::byte> src) override
    {
      if (N < src.size_bytes())
        return false;

      while (!src.empty()) {
        const auto sz = static_cast<std::ptrdiff_t>(src.size_bytes());

        if (m_shadowTail == N + 1) {
          if (m_parent.m_cachedHead == 0) {
            m_parent.m_cachedHead = m_parent.m_head.load(std::memory_order::acquire);
            if (m_parent.m_cachedHead == 0)
              return false;
          }
          m_shadowTail = 0;
        } else if (m_shadowTail + 1 == m_parent.m_cachedHead) {
          m_parent.m_cachedHead = m_parent.m_head.load(std::memory_order::acquire);
          if (m_shadowTail + 1 == m_parent.m_cachedHead)
            return false;
        }

        if (m_parent.m_cachedHead <= m_shadowTail) {
          const auto spaceLeft = N + 1 - m_shadowTail;
          const auto writeSize = std::min(sz, spaceLeft);

          std::memcpy(m_parent.m_buff.data() + m_shadowTail, src.data(), static_cast<std::size_t>(writeSize));
          m_shadowTail += writeSize;
          m_bytesWritten += writeSize;

          src = src.subspan(static_cast<std::size_t>(writeSize));
        } else {
          // In this regard, the tail is behind the head
          const auto spaceLeft = m_parent.m_cachedHead - m_shadowTail - 1;
          const auto writeSize = std::min(sz, spaceLeft);

          std::memcpy(m_parent.m_buff.data() + m_shadowTail, src.data(), static_cast<std::size_t>(writeSize));
          m_shadowTail += writeSize;
          m_bytesWritten += writeSize;

          src = src.subspan(static_cast<std::size_t>(writeSize));
        }
      }

      return true;
    }
    [[nodiscard]] std::size_t bytes_written() const override { return m_bytesWritten; }

  private:
    RingBuffer& m_parent;
    std::ptrdiff_t m_shadowTail;
    std::size_t m_bytesWritten{ 0 };
  };

public:
  RingBuffer() = default;
  ~RingBuffer() override = default;

  // We don't want copying
  RingBuffer(const RingBuffer&) = delete;
  RingBuffer& operator=(const RingBuffer&) = delete;

  // We don't want moving either.
  RingBuffer(RingBuffer&&) = delete;
  RingBuffer& operator=(RingBuffer&&) = delete;

  [[nodiscard]] std::unique_ptr<ByteBuffer::Reader> get_reader() override
  {
    // std::unique_ptr<Reader> out{new (std::align_val_t(alignof(Reader))) Reader(*this)};
    // return out;
    return std::make_unique<Reader>(*this);
  }

  [[nodiscard]] std::unique_ptr<ByteBuffer::Writer> get_writer() override
  {
    // std::unique_ptr<Writer> out{new (std::align_val_t(alignof(Writer))) Writer(*this)};
    //  return out;
    return std::make_unique<Writer>(*this);
  }
  [[nodiscard]] std::size_t capacity() override { return N; }
};

void
test1()
{

  RingBuffer<4096> ringBuffer;
  std::latch ready(2);

  // constexpr std::int64_t TIMES = 1;
  constexpr std::int64_t TIMES = 100;

  static constexpr auto goodBytes = byte_array(1, 2, 3, 4, 5, 6, 7, 8);

  std::thread writer([&ringBuffer, &ready]() {
    ready.arrive_and_wait();

    std::int64_t i = 0;

    while (i < TIMES) {
      const auto writer = ringBuffer.get_writer();
      if (!writer->write(goodBytes))
        continue;

      const auto lel = writer->commit();
      assert(lel);
      i++;
    }
  });

  std::thread reader([&ringBuffer, &ready]() {
    ready.arrive_and_wait();
    std::int64_t i = 0;

    while (i < TIMES) {
      const auto reader = ringBuffer.get_reader();
      std::remove_cv_t<decltype(goodBytes)> srcBuf{};
      if (!reader->read(srcBuf))
        continue;

      // CAPTURE(&srcBuf);

      const auto good = reader->commit();
      assert(good);

      if (srcBuf != goodBytes) {
        assert(false);
      }
      i++;
    }
  });

  writer.join();
  reader.join();
}

int
main()
{
  test1();

  /*
  volatile bool fun = (uintptr_t)(&buffer) % 64 != 0;
  volatile uintptr_t wow = (uintptr_t)(&buffer);

  const auto woww = alignof(std::max_align_t);

  {
    const auto reader = buffer.get_reader();
    std::array<std::byte, 1> testBuffer{};
    assert(!reader->read(testBuffer));
    assert(reader->commit());
  }
  */

  return 0;
}