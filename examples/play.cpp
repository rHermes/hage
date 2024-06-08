#include <future>
#include <iostream>

int
small_comp(const int i)
{
  std::this_thread::sleep_for(std::chrono::seconds(2));
  return i * 2;
}

int
main()
{
  std::future<int> theAnswer = std::async(small_comp, 10);
  std::cout << "Here we are after starting that task.\n";
  std::cout << "And we finished now: " << theAnswer.get() << '\n';
  return 0;
}