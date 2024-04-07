#include <cstdint>
#include <memory>

struct RingBuffer
{
  alignas(64) std::uint64_t m_head{ 0xFAFAFAFAFAFAFAFA };

  int good()
  {
    return m_head == 2;
  }
};

int main() {
  RingBuffer buffer;
  return buffer.good();
}