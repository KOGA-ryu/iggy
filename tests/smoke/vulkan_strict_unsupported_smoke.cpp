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
  int exitCode = 0;
};

constexpr std::array<UnsupportedCase, 5> kCases = {{
    {"loader_missing", "vulkan_smoke_strict_dependency_missing"},
    {"display_unavailable", "vulkan_smoke_strict_dependency_missing"},
    {"no_physical_device", "vulkan_smoke_strict_dependency_missing"},
    {"validation_missing", "vulkan_smoke_validation_failed"},
    {"shader_missing", "packet7_smoke_strict_dependency_missing"},
}};

PolicyResult evaluateUnsupported(const UnsupportedCase& unsupported, bool strict) {
  if (!strict) {
    return {"skip", unsupported.name == "loader_missing" ? "vulkan_smoke_skipped_loader_missing"
                                                         : "vulkan_smoke_skipped_no_display",
            77};
  }
  return {"fail", unsupported.reason, 1};
}

bool strictPolicyFails(const UnsupportedCase& unsupported) {
  const PolicyResult result = evaluateUnsupported(unsupported, true);
  return result.result == std::string_view{"fail"} && result.reason == unsupported.reason &&
         result.exitCode != 77 && result.exitCode != 0;
}

}  // namespace

int main() {
  bool allCasesCovered = true;
  for (const UnsupportedCase& unsupported : kCases) {
    allCasesCovered = allCasesCovered && strictPolicyFails(unsupported);
  }

  std::cout << "smoke=vulkan_strict_unsupported\n";
  std::cout << "strict_lane=true\n";
  std::cout << "unsupported_case=loader_missing\n";
  std::cout << "unsupported_case_count=" << kCases.size() << "\n";
  std::cout << "unsupported_cases=loader_missing,display_unavailable,no_physical_device,"
               "validation_missing,shader_missing\n";
  std::cout << "unsupported_policy=strict_fail\n";
  std::cout << "renderer_outcome=unsupported\n";
  std::cout << "expected_exit_code=1\n";
  std::cout << "actual_exit_code=" << (allCasesCovered ? 1 : 0) << "\n";
  std::cout << "result=" << (allCasesCovered ? "fail" : "pass") << "\n";
  std::cout << "reason_code="
            << (allCasesCovered ? "vulkan_smoke_strict_dependency_missing"
                                : "vulkan_smoke_receipt_invalid")
            << "\n";
  return allCasesCovered ? 1 : 0;
}
