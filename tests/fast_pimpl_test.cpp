//
// Created by rhermes on 2024-06-08.
//

#include "fast_pimpl_test.hpp"

#include <string>

using namespace hage::test;

namespace hage::test::details {

struct FastPimplTestImpl
{
  int a{ 2 };
  std::string b{ "this is the default string" };

  void setA(int a) { this->a = a; }

  int getA() { return this->a; }

  void setB(std::string b) { this->b = std::move(b); }
  std::string getB() { return this->b; }
};

} // namespace hage::test::details
FastPimplTest::FastPimplTest() = default;
FastPimplTest::~FastPimplTest() = default;
FastPimplTest::FastPimplTest(const FastPimplTest&) = default;
FastPimplTest&
FastPimplTest::operator=(const FastPimplTest&) = default;
FastPimplTest::FastPimplTest(FastPimplTest&&) = default;
FastPimplTest&
FastPimplTest::operator=(FastPimplTest&&) = default;

void
FastPimplTest::setA(int a)
{
  m_impl->setA(a);
}

int
FastPimplTest::getA()
{
  return m_impl->getA();
}

void
FastPimplTest::setB(std::string b)
{
  return m_impl->setB(std::move(b));
}

std::string
FastPimplTest::getB()
{
  return m_impl->getB();
}
