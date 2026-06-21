#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "render/RenderBackend.hpp"

namespace iggy3d {

class NullRenderer final : public RenderBackend {
public:
  NullRenderer();
  NullRenderer(const NullRenderer&) = delete;
  NullRenderer& operator=(const NullRenderer&) = delete;
  NullRenderer(NullRenderer&&) noexcept = default;
  NullRenderer& operator=(NullRenderer&&) noexcept = default;
  ~NullRenderer() override;

  RendererBackendKind backendKind() const override;
  RendererLifecycleState lifecycleState() const override;
  RenderSubmitResult submitFrame(const FrameInput& frame) override;
  RenderSubmitResult resize(RenderViewport viewport) override;
  RenderReceipt diagnostics() const override;
  RenderOutcome waitIdle() override;
  void shutdown() override;

private:
  RenderReceipt makeReceipt(std::string_view result,
                            std::string_view reasonCode,
                            std::string_view frameInputValid) const;

  RendererLifecycleState lifecycleState_ = RendererLifecycleState::Ready;
  std::uint64_t submittedFrameCount_ = 0;
  std::uint64_t acceptedFrameCount_ = 0;
  std::uint64_t rejectedFrameCount_ = 0;
  std::uint64_t lastSourceTick_ = 0;
  std::uint64_t lastFrameIndex_ = 0;
  std::uint64_t lastSceneItemCount_ = 0;
  std::uint64_t lastDebugItemCount_ = 0;
  std::uint32_t lastViewportWidth_ = 0;
  std::uint32_t lastViewportHeight_ = 0;
  std::string_view lastReasonCode_ = "null_renderer_ok";
};

}  // namespace iggy3d
