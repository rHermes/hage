#pragma once

#include <hage/core/forward_declared_storage.hpp>
#include <string>

namespace hage::test {
namespace details {
struct FastPimplTestImpl;
}

class FastPimplTest
{
private:
  hage::ForwardDeclaredStorage<details::FastPimplTestImpl, 40, 8> m_impl;

public:
  FastPimplTest();
  ~FastPimplTest();

  FastPimplTest(const FastPimplTest&);
  FastPimplTest& operator=(const FastPimplTest&);
  FastPimplTest(FastPimplTest&&);
  FastPimplTest& operator=(FastPimplTest&&);

  void setA(int a);
  int getA();

  void setB(std::string b);
  std::string getB();
};

} // namespace hage::test