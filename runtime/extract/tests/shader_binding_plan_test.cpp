// shader_binding_plan_test.cpp — TDD test for ShaderBindingPlan + buildBindingPlan.
// Tests three categories: vocab match, unrecognized with suggestion, author field.
#include "ShaderBindingPlan.hpp"
#include "doctest/doctest.h"
#include <string>
#include <vector>

using namespace x3d::runtime::extract;
using namespace x3d::runtime::extract::vocab;

TEST_CASE("shader binding plan: uniforms bind by vocabulary, author field or suggestion") {
  // Mock declared uniforms: modelViewMatrix (vocab), uModelViewMat (unrecognized,
  // close to vocab match), hue (author field).
  ShaderProgramDesc prog;
  ShaderFieldBinding hueField;
  hueField.name = "hue";
  hueField.type = X3DFieldType::SFFloat;
  prog.fields.push_back(hueField);

  std::vector<std::pair<std::string, int>> declared = {
    {"modelViewMatrix", 0},
    {"uModelViewMat",   1},
    {"hue",             2},
  };

  ShaderBindingPlan plan = buildBindingPlan(declared, prog);

  CHECK((plan.entries.size() == 3));

  SUBCASE("a vocabulary uniform binds") {
    // modelViewMatrix → vocab match
    CHECK((!plan.entries[0].unrecognized));
    CHECK((plan.entries[0].source == UniformSource::ModelViewMatrix));
    CHECK((!plan.entries[0].isAuthorField));
  }
  SUBCASE("an unknown uniform gets a suggestion") {
    // uModelViewMat → unrecognized, with a suggestion
    CHECK((plan.entries[1].unrecognized));
    CHECK((!plan.entries[1].nearestVocabMatch.empty()));
  }
  SUBCASE("an author field binds") {
    // hue → author field
    CHECK((!plan.entries[2].unrecognized));
    CHECK((plan.entries[2].isAuthorField));
  }
  SUBCASE("the diagnostic names the uniform and the suggestion") {
    // Diagnostic for unrecognized should mention both the bad name and the suggestion
    bool hasDiagnostic = false;
    for (const auto &d : plan.diagnostics)
      if (d.find("uModelViewMat") != std::string::npos &&
          d.find("modelViewMatrix") != std::string::npos)
        hasDiagnostic = true;
    CHECK((hasDiagnostic));
  }
}
