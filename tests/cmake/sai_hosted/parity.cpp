#include <x3d/sai/experimental/reference_hosted.hpp>
#include <x3d/sai/experimental/testing/hosted_fixture.hpp>
#include <x3d/sai_hosted.hpp>
#include <iostream>
namespace sai = x3d::sai::experimental;
int main() {
  try {
    const auto reference = sai::testing::run_hosted_fixture(sai::make_reference_hosted);
    const auto native = sai::testing::run_hosted_fixture(
        []() -> sai::result<std::unique_ptr<sai::hosted::service>> {
          return std::unique_ptr<sai::hosted::service>{
              std::make_unique<x3d::runtime::SaiHostedService>()};
        });
    if (native != reference)
      throw std::runtime_error("hosted backends returned different observation reports");
    std::cout << "identical hosted reports: " << native.checks << " common checks per backend\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
