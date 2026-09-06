#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d {

enum class RenderOutcome : std::uint8_t {
  Ok,
  InvalidFrameInput,
  RendererNotReady,
  Unsupported,
  SkipFrame,
  RecreateSwapchain,
  SurfaceLost,
  DeviceLost,
  OutOfMemory,
  ValidationFailure,
  PipelineOrShaderFailure,
  FatalRendererError,
};

struct RenderReason {
  std::string_view code;
  std::string_view message;
};

struct RenderReceiptField {
  std::string key;
  std::string value;
};

struct RenderReceipt {
  std::vector<RenderReceiptField> fields;
};

bool isRenderReceiptKeyValid(std::string_view key);
std::string normalizeReceiptValue(std::string_view value);
void appendReceiptField(RenderReceipt& receipt, std::string_view key, std::string_view value);
void appendReceiptField(RenderReceipt& receipt, std::string_view key, const char* value);
void appendReceiptField(RenderReceipt& receipt, std::string_view key, bool value);
void appendReceiptField(RenderReceipt& receipt, std::string_view key, std::uint64_t value);
std::string formatRenderReceipt(const RenderReceipt& receipt);
std::string_view renderOutcomeName(RenderOutcome outcome);
bool hasReceiptField(const RenderReceipt& receipt, std::string_view key);
bool hasReceiptField(const RenderReceipt& receipt, std::string_view key, std::string_view value);

}  // namespace iggy3d
