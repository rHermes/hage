#pragma once

#include <hage/core/forward_declared_storage.hpp>
#include <hage/core/misc.hpp>
#include <string>

namespace hage::test {
namespace details {
struct FastPimplTestImpl;
}

class FastPimplTest
{
private:
#ifdef _WIN32

#if HAGE_DEBUG
  hage::ForwardDeclaredStorage<details::FastPimplTestImpl, 48, 8> m_impl;
#else
  hage::ForwardDeclaredStorage<details::FastPimplTestImpl, 40, 8> m_impl;
#endif

#elif defined(__APPLE__)
  hage::ForwardDeclaredStorage<details::FastPimplTestImpl, 32, 8> m_impl;
#else
  hage::ForwardDeclaredStorage<details::FastPimplTestImpl, 40, 8> m_impl;
#endif
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