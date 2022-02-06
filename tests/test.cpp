#include <utility>
#include "test.hpp"

int main() {
  auto [a, b] = std::pair<int, int>(1, 2);
  return 0;
}
