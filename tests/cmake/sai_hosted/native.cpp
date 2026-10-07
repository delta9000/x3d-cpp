#include <x3d/sai/experimental/testing/hosted_fixture.hpp>
#include <x3d/sai_hosted.hpp>
#include <iostream>
#ifdef X3D_SAI_EXPERIMENTAL_KERNEL_HPP
#error "The hosted native consumer must not include the reference kernel"
#endif
#ifdef X3D_SAI_EXPERIMENTAL_METADATA_HPP
#error "The hosted native consumer must not include reference metadata"
#endif
namespace sai = x3d::sai::experimental;
int main() {
  try {
    const auto report = sai::testing::run_hosted_fixture(
        []() -> sai::result<std::unique_ptr<sai::hosted::service>> {
          return std::unique_ptr<sai::hosted::service>{
              std::make_unique<x3d::runtime::SaiHostedService>()};
        });
    std::cout << "native hosted runtime: " << report.checks << " common checks passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
