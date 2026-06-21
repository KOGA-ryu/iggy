#include "render/RenderDiagnostics.hpp"

#include <charconv>

namespace iggy3d {

bool isRenderReceiptKeyValid(std::string_view key) {
  if (key.empty()) {
    return false;
  }
  for (const char c : key) {
    const bool lower = c >= 'a' && c <= 'z';
    const bool digit = c >= '0' && c <= '9';
    if (!lower && !digit && c != '_') {
      return false;
    }
  }
  return true;
}

std::string normalizeReceiptValue(std::string_view value) {
  std::string normalized;
  normalized.reserve(value.size());
  for (const char c : value) {
    if (c == '\n' || c == '\r' || c == '\t') {
      normalized.push_back(' ');
    } else {
      normalized.push_back(c);
    }
  }
  return normalized;
}

void appendReceiptField(RenderReceipt& receipt, std::string_view key, std::string_view value) {
  if (!isRenderReceiptKeyValid(key)) {
    receipt.fields.push_back({"reason_code", "receipt_key_invalid"});
    return;
  }
  receipt.fields.push_back({std::string(key), normalizeReceiptValue(value)});
}

void appendReceiptField(RenderReceipt& receipt, std::string_view key, const char* value) {
  appendReceiptField(receipt, key,
                     value == nullptr ? std::string_view{"unavailable"} : std::string_view{value});
}

void appendReceiptField(RenderReceipt& receipt, std::string_view key, bool value) {
  appendReceiptField(receipt, key, value ? std::string_view{"true"} : std::string_view{"false"});
}

void appendReceiptField(RenderReceipt& receipt, std::string_view key, std::uint64_t value) {
  char buffer[32]{};
  const auto [ptr, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
  if (error != std::errc{}) {
    appendReceiptField(receipt, key, "unavailable");
    return;
  }
  appendReceiptField(receipt, key, std::string_view{buffer, static_cast<std::size_t>(ptr - buffer)});
}

std::string formatRenderReceipt(const RenderReceipt& receipt) {
  std::string out;
  for (const RenderReceiptField& field : receipt.fields) {
    out += field.key;
    out.push_back('=');
    out += field.value;
    out.push_back('\n');
  }
  return out;
}

std::string_view renderOutcomeName(RenderOutcome outcome) {
  switch (outcome) {
    case RenderOutcome::Ok:
      return "ok";
    case RenderOutcome::InvalidFrameInput:
      return "invalid_frame_input";
    case RenderOutcome::RendererNotReady:
      return "renderer_not_ready";
    case RenderOutcome::Unsupported:
      return "unsupported";
    case RenderOutcome::SkipFrame:
      return "skip_frame";
    case RenderOutcome::RecreateSwapchain:
      return "recreate_swapchain";
    case RenderOutcome::SurfaceLost:
      return "surface_lost";
    case RenderOutcome::DeviceLost:
      return "device_lost";
    case RenderOutcome::OutOfMemory:
      return "out_of_memory";
    case RenderOutcome::ValidationFailure:
      return "validation_failure";
    case RenderOutcome::PipelineOrShaderFailure:
      return "pipeline_or_shader_failure";
    case RenderOutcome::FatalRendererError:
      return "fatal_renderer_error";
  }
  return "fatal_renderer_error";
}

bool hasReceiptField(const RenderReceipt& receipt, std::string_view key) {
  for (const RenderReceiptField& field : receipt.fields) {
    if (field.key == key) {
      return true;
    }
  }
  return false;
}

bool hasReceiptField(const RenderReceipt& receipt, std::string_view key, std::string_view value) {
  for (const RenderReceiptField& field : receipt.fields) {
    if (field.key == key && field.value == value) {
      return true;
    }
  }
  return false;
}

}  // namespace iggy3d
