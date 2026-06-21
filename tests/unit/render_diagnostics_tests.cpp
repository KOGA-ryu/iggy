#include "render/RenderDiagnostics.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool isLowerSnake(std::string_view value) {
  if (value.empty()) {
    return false;
  }
  for (const char c : value) {
    const bool lower = c >= 'a' && c <= 'z';
    const bool digit = c >= '0' && c <= '9';
    if (!lower && !digit && c != '_') {
      return false;
    }
  }
  return true;
}

bool receiptFormatsOneKeyValuePerLine() {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(receipt, "backend", "null");
  iggy3d::appendReceiptField(receipt, "app", "render_diagnostics_tests");
  iggy3d::appendReceiptField(receipt, "test_name", "receipt_formats_one_key_value_per_line");
  iggy3d::appendReceiptField(receipt, "platform", "unknown");
  iggy3d::appendReceiptField(receipt, "platform_lane", "headless");
  iggy3d::appendReceiptField(receipt, "strict_vulkan", false);
  iggy3d::appendReceiptField(receipt, "result", "pass");
  iggy3d::appendReceiptField(receipt, "reason_code", "renderer_ok");

  const std::string expected =
      "receipt_version=1\n"
      "repo=iggy3d\n"
      "backend=null\n"
      "app=render_diagnostics_tests\n"
      "test_name=receipt_formats_one_key_value_per_line\n"
      "platform=unknown\n"
      "platform_lane=headless\n"
      "strict_vulkan=false\n"
      "result=pass\n"
      "reason_code=renderer_ok\n";
  return expect(iggy3d::formatRenderReceipt(receipt) == expected, "receipt exact");
}

bool receiptNormalizesAndKeepsEquals() {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "message", "line one\nline two\tvalue=a=b");
  const std::string formatted = iggy3d::formatRenderReceipt(receipt);
  return expect(formatted == "message=line one line two value=a=b\n", "normalized value") &&
         expect(iggy3d::normalizeReceiptValue("a\rb\tc\n") == "a b c ",
                "normalize helper");
}

bool requiredFieldsAreDetectable() {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(receipt, "reason_code", "renderer_ok");
  return expect(iggy3d::hasReceiptField(receipt, "receipt_version", "1"), "receipt version") &&
         expect(iggy3d::hasReceiptField(receipt, "repo"), "repo") &&
         expect(iggy3d::hasReceiptField(receipt, "reason_code", "renderer_ok"),
                "reason code") &&
         expect(!iggy3d::hasReceiptField(receipt, "result"), "missing result");
}

bool outcomeAndReasonNamesAreStable() {
  return expect(iggy3d::renderOutcomeName(iggy3d::RenderOutcome::Ok) == "ok", "ok name") &&
         expect(iggy3d::renderOutcomeName(iggy3d::RenderOutcome::InvalidFrameInput) ==
                    "invalid_frame_input",
                "invalid frame name") &&
         expect(iggy3d::renderOutcomeName(iggy3d::RenderOutcome::Unsupported) == "unsupported",
                "unsupported name") &&
         expect(isLowerSnake("renderer_ok"), "renderer ok lower snake") &&
         expect(isLowerSnake("receipt_value_normalized"), "receipt normalized lower snake") &&
         expect(!isLowerSnake("RendererOk"), "mixed case rejected");
}

}  // namespace

int main() {
  bool ok = true;
  ok = receiptFormatsOneKeyValuePerLine() && ok;
  ok = receiptNormalizesAndKeepsEquals() && ok;
  ok = requiredFieldsAreDetectable() && ok;
  ok = outcomeAndReasonNamesAreStable() && ok;
  return ok ? 0 : 1;
}
