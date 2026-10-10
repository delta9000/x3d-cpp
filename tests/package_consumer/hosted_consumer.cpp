#include "x3d/sai/experimental/reference_provider.hpp"
#include "x3d/sai/experimental/testing/provider_fixture.hpp"
#include <exception>
#include <iostream>

int main() {
  try {
    const auto report = x3d::sai::experimental::testing::run_provider_fixture(
        [] { return x3d::sai::experimental::make_reference_provider(); });
    std::cout << "unified reference checks: " << report.checks << '\n';
    return report.checks ? 0 : 1;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
