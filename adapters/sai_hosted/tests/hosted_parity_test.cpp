#include <iostream>
#include <x3d/sai/experimental/reference_hosted.hpp>
#include <x3d/sai/experimental/testing/hosted_fixture.hpp>
#include <x3d/sai_hosted.hpp>

int main() {
  namespace sai = x3d::sai::experimental;
  try {
    const auto reference =
        sai::testing::run_hosted_fixture(sai::make_reference_hosted);
    const auto native = sai::testing::run_hosted_fixture(
        []() -> sai::result<std::unique_ptr<sai::hosted::service>> {
          return std::unique_ptr<sai::hosted::service>{
              new x3d::runtime::SaiHostedService};
        });
    if (!(reference == native)) {
      std::cerr << "reference/native hosted observations differ\n";
      return 1;
    }
    std::cout << "reference/native hosted parity: " << native.checks
              << " identical checks and observation reports\n";
  } catch (const std::exception &e) {
    std::cerr << "reference/native hosted parity failed: " << e.what() << '\n';
    return 1;
  }
}
