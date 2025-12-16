#include <utility>
#include <iostream>

void bar() {}


void foo() {}


int main() {
  auto [a, b] = std::pair<int, int>(1, 2);
  return 0;
}
