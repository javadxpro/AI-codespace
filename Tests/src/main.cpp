#include <kimia_test.h>

int main() {
  int passed = 0;
  for (const auto& test : kimia::test::registry()) {
    try { test.second(); ++passed; }
    catch (const std::exception& error) { std::cerr << "FAIL " << test.first << ": " << error.what() << '\n'; return 1; }
  }
  std::cout << passed << "/" << kimia::test::registry().size() << " tests passed\n";
  return 0;
}
