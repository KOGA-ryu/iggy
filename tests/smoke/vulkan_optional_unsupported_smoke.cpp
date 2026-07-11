#include <array>
#include <iostream>
#include <string_view>

namespace {

struct UnsupportedCase {
  std::string_view name;
  std::string_view reason;
};

struct PolicyResult {
  std::string_view result;
  std::string_view reason;
  int exitCode = 1;
};

constexpr std::array<UnsupportedCase, 5> kCases = {{
    {"loader_missing", "vulkan_smoke_skipped_loader_missing"},
    {"display_unavailable", "vulkan_smoke_skipped_no_display"},
    {"no_physical_device", "vulkan_smoke_skipped_no_suitable_device"},
    {"validation_missing", "vulkan_smoke_validation_unavailable"},
    {"shader_missing", "shader_artifact_missing"},
}};

PolicyResult evaluateUnsupported(const UnsupportedCase& unsupported, bool strict) {
  if (strict) {
    return {"fail", unsupported.name == "validation_missing" ? "vulkan_smoke_validation_failed"
                                                              : "vulkan_smoke_strict_dependency_missing",
            1};
  }
  return {"skip", unsupported.reason, 77};
}

bool optionalPolicyAccepts(const UnsupportedCase& unsupported) {
  const PolicyResult result = evaluateUnsupported(unsupported, false);
  return result.result == std::string_view{"skip"} && result.reason == unsupported.reason &&
         result.exitCode == 77;
}

}  // namespace

int main() {
  bool allCasesCovered = true;
  for (const UnsupportedCase& unsupported : kCases) {
    allCasesCovered = allCasesCovered && optionalPolicyAccepts(unsupported);
  }

  std::cout << "smoke=vulkan_optional_unsupported\n";
  std::cout << "strict_lane=false\n";
  std::cout << "unsupported_case=loader_missing\n";
  std::cout << "unsupported_case_count=" << kCases.size() << "\n";
  std::cout << "unsupported_cases=loader_missing,display_unavailable,no_physical_device,"
               "validation_missing,shader_missing\n";
  std::cout << "unsupported_policy=optional_skip\n";
  std::cout << "renderer_outcome=unsupported\n";
  std::cout << "expected_exit_code=77\n";
  std::cout << "actual_exit_code=" << (allCasesCovered ? 77 : 1) << "\n";
  std::cout << "result=" << (allCasesCovered ? "skip" : "fail") << "\n";
  std::cout << "reason_code="
            << (allCasesCovered ? "vulkan_smoke_skipped_loader_missing"
                                : "vulkan_smoke_receipt_invalid")
            << "\n";
  return allCasesCovered ? 77 : 1;
}
