#include <x3d/sai/experimental/reference_hosted.hpp>
#include <x3d/sai/experimental/testing/hosted_fixture.hpp>
#include <iostream>
int main() {
  try {
    const auto report = x3d::sai::experimental::testing::run_hosted_fixture(
        x3d::sai::experimental::make_reference_hosted);
    std::cout << "reference hosted runtime: " << report.checks << " common checks passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
