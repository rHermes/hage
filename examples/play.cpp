#include <iostream>

class C
{
public:
  void foo() const& { std::cout << "foo() const&\n"; }
  void foo() && { std::cout << "foo() &&\n"; }
  void foo() & { std::cout << "foo() &\n"; }
  void foo() const&& { std::cout << "foo() const&&\n"; }
};

int
main()
{
  C x;
  x.foo();            // calls foo() &
  C{}.foo();          // calls foo() &&
  std::move(x).foo(); // calls foo() &&

  const C cx;
  cx.foo();            // calls foo() const&
  std::move(cx).foo(); // calls foo() const&&
}