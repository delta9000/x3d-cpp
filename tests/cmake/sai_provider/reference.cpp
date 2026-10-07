#include "x3d/sai/experimental/reference_provider.hpp"
#include "x3d/sai/experimental/testing/provider_fixture.hpp"
#include <iostream>
int main() {
  try {
    auto report = x3d::sai::experimental::testing::run_provider_fixture(
        x3d::sai::experimental::make_reference_provider);
    std::cout << "reference offline provider: " << report.checks
              << " checks passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
