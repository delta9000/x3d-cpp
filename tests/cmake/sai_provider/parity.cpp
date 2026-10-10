#include "x3d/sai/experimental/reference_provider.hpp"
#include "x3d/sai/experimental/testing/provider_fixture.hpp"
#include "x3d/sai_provider.hpp"
#include <iostream>
namespace sai = x3d::sai::experimental;
int main() {
  try {
    auto reference = sai::testing::run_provider_fixture(
        [] { return sai::make_reference_provider(); });
    auto native = sai::testing::run_provider_fixture(
        [] { return sai::native::make_service(); });
    if (native != reference)
      throw std::runtime_error(
          "independent providers returned different fixture reports");
    std::cout << "identical independent-provider reports: " << native.checks
              << " checks per backend\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
