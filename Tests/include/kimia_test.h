#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace kimia::test {
using Test = std::pair<std::string, std::function<void()>>;
inline std::vector<Test>& registry() { static std::vector<Test> tests; return tests; }
struct Register { Register(const char* name, std::function<void()> fn) { registry().emplace_back(name, std::move(fn)); } };
inline void require(bool condition, const char* expression, const char* file, int line) {
  if (!condition) throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + ": " + expression);
}
}
#define KIMIA_TEST(name) static void name(); static ::kimia::test::Register reg_##name(#name, name); static void name()
#define KIMIA_REQUIRE(expr) ::kimia::test::require((expr), #expr, __FILE__, __LINE__)
