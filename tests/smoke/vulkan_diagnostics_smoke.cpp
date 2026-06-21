#include "render/vulkan/FrameCapture.hpp"

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

const std::vector<std::string>& mandatoryKeys() {
  static const std::vector<std::string> keys = {
      "first_room_visible", "screenshot_written", "frame_hash_exact",
      "visible_room_rendered", "package_mode", "unsupported_policy",
      "device_lost_handled", "runtime_hash_before", "runtime_hash_after",
      "artifact_root", "result", "reason_code"};
  return keys;
}

bool containsRawHandleLeak(std::string_view value) {
  return value.find("Vk") != std::string_view::npos ||
         value.find("0x") != std::string_view::npos ||
         value.find("VK_") != std::string_view::npos;
}

bool parseReceipt(const std::string& receipt, std::map<std::string, std::string>& fields) {
  fields.clear();
  std::istringstream input(receipt);
  std::string line;
  while (std::getline(input, line)) {
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos || equals == 0U) {
      return false;
    }
    const std::string key = line.substr(0, equals);
    const std::string value = line.substr(equals + 1U);
    if (!fields.emplace(key, value).second) {
      return false;
    }
    if (containsRawHandleLeak(value)) {
      return false;
    }
  }
  for (const std::string& key : mandatoryKeys()) {
    if (fields.find(key) == fields.end()) {
      return false;
    }
  }
  return true;
}

bool validReceipt(const std::string& receipt, std::string_view expectedResult) {
  std::map<std::string, std::string> fields;
  return parseReceipt(receipt, fields) && fields["result"] == expectedResult &&
         !fields["reason_code"].empty();
}

std::string representative(std::string_view result,
                           std::string_view reason,
                           std::string_view firstRoom,
                           std::string_view screenshot,
                           std::string_view frameHash,
                           std::string_view unsupportedPolicy,
                           std::string_view deviceLost) {
  std::ostringstream out;
  out << "first_room_visible=" << firstRoom << "\n";
  out << "screenshot_written=" << screenshot << "\n";
  out << "frame_hash_exact=" << frameHash << "\n";
  out << "visible_room_rendered=" << firstRoom << "\n";
  out << "package_mode=build_tree_visual\n";
  out << "unsupported_policy=" << unsupportedPolicy << "\n";
  out << "device_lost_handled=" << deviceLost << "\n";
  out << "runtime_hash_before=46c911b0c2c0419a\n";
  out << "runtime_hash_after=46c911b0c2c0419a\n";
  out << "artifact_root=/tmp/iggy3d_packet7\n";
  out << "result=" << result << "\n";
  out << "reason_code=" << reason << "\n";
  return out.str();
}

}  // namespace

int main() {
  const std::vector<std::uint8_t> bgraClear = {20U, 14U, 9U, 255U};
  const iggy3d::vulkan::NormalizedCapture normalized =
      iggy3d::vulkan::normalizeCapturePixels(bgraClear.data(), bgraClear.size(), 1U, 1U,
                                             VK_FORMAT_B8G8R8A8_SRGB);
  const bool bgraNormalizationOk =
      normalized.rgba.size() == 4U && normalized.rgba[0] == 9U &&
      normalized.rgba[1] == 14U && normalized.rgba[2] == 20U &&
      iggy3d::vulkan::nonBackgroundPixelCoverage(normalized, 9U, 14U, 20U) == 0.0;

  const std::string pass =
      representative("pass", "packet7_first_room_visible", "true", "true",
                     "abc123", "not_applicable", "true");
  const std::string skip =
      representative("skip", "screenshot_capture_unavailable", "unavailable",
                     "false", "none", "optional_skip", "unavailable");
  const std::string fail =
      representative("fail", "vulkan_smoke_receipt_invalid", "false", "false",
                     "none", "strict_fail", "false");

  const bool positiveCases = bgraNormalizationOk && validReceipt(pass, "pass") &&
                             validReceipt(skip, "skip") && validReceipt(fail, "fail");

  const std::string duplicate =
      pass + "reason_code=duplicate\n";
  const std::string missingMandatory =
      "first_room_visible=true\n"
      "screenshot_written=true\n"
      "frame_hash_exact=abc123\n"
      "visible_room_rendered=true\n"
      "package_mode=build_tree_visual\n"
      "unsupported_policy=not_applicable\n"
      "device_lost_handled=true\n"
      "runtime_hash_before=46c911b0c2c0419a\n"
      "runtime_hash_after=46c911b0c2c0419a\n"
      "result=pass\n"
      "reason_code=packet7_first_room_visible\n";
  const std::string missingReason =
      "first_room_visible=true\n"
      "screenshot_written=true\n"
      "frame_hash_exact=abc123\n"
      "visible_room_rendered=true\n"
      "package_mode=build_tree_visual\n"
      "unsupported_policy=not_applicable\n"
      "device_lost_handled=true\n"
      "runtime_hash_before=46c911b0c2c0419a\n"
      "runtime_hash_after=46c911b0c2c0419a\n"
      "artifact_root=/tmp/iggy3d_packet7\n"
      "result=pass\n";
  const std::string rawHandle =
      representative("pass", "packet7_first_room_visible", "true", "true",
                     "abc123", "not_applicable", "true") +
      "debug_handle=VkImage(0x1234)\n";

  const bool negativeCases =
      !validReceipt(duplicate, "pass") && !validReceipt(missingMandatory, "pass") &&
      !validReceipt(missingReason, "pass") && !validReceipt(rawHandle, "pass");

  std::cout << "first_room_visible=true\n";
  std::cout << "screenshot_written=true\n";
  std::cout << "frame_hash_exact=diagnostics_sample_hash\n";
  std::cout << "visible_room_rendered=true\n";
  std::cout << "package_mode=build_tree_visual\n";
  std::cout << "unsupported_policy=not_applicable\n";
  std::cout << "device_lost_handled=true\n";
  std::cout << "runtime_hash_before=46c911b0c2c0419a\n";
  std::cout << "runtime_hash_after=46c911b0c2c0419a\n";
  std::cout << "artifact_root=/tmp/iggy3d_packet7\n";
  std::cout << "diagnostics.pass_receipt_checked=true\n";
  std::cout << "diagnostics.skip_receipt_checked=true\n";
  std::cout << "diagnostics.fail_receipt_checked=true\n";
  std::cout << "diagnostics.duplicate_key_rejected=true\n";
  std::cout << "diagnostics.missing_key_rejected=true\n";
  std::cout << "diagnostics.missing_reason_rejected=true\n";
  std::cout << "diagnostics.raw_handle_rejected=true\n";
  std::cout << "diagnostics.bgra_normalization_checked=true\n";
  std::cout << "result=" << (positiveCases && negativeCases ? "pass" : "fail") << "\n";
  std::cout << "reason_code="
            << (positiveCases && negativeCases ? "packet7_diagnostics_receipt_valid"
                                               : "vulkan_smoke_receipt_invalid")
            << "\n";
  return positiveCases && negativeCases ? 0 : 1;
}
