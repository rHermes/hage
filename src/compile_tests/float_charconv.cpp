//
// Created by rhermes on 2024-05-06.
//
#include <charconv>
#include <string>

int
main()
{
  std::string s = "123.3";
  float x;
  std::from_chars(s.data(), s.data() + s.size(), x);
  return 0;
}
