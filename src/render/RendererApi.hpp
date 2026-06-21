#pragma once

#include <cstdint>
#include <memory>

#include "render/FrameInput.hpp"
#include "render/RenderDiagnostics.hpp"
#include "render/RendererConfig.hpp"

namespace iggy3d {

class RenderBackend;

enum class RendererBackendKind : std::uint8_t {
  Null,
  Vulkan,
};

enum class RendererLifecycleState : std::uint8_t {
  NotInitialized,
  Ready,
  DeviceLost,
  Shutdown,
};

struct RendererCreateInfo {
  RendererBackendKind backend = RendererBackendKind::Null;
  RendererConfig config;
};

struct RenderSubmitResult {
  RenderOutcome outcome = RenderOutcome::RendererNotReady;
  RenderReason reason{"renderer_not_ready", "renderer not ready"};
  RenderReceipt receipt;
};

class RendererApi {
public:
  RendererApi();
  explicit RendererApi(std::unique_ptr<RenderBackend> backend);
  ~RendererApi();

  RendererApi(const RendererApi&) = delete;
  RendererApi& operator=(const RendererApi&) = delete;
  RendererApi(RendererApi&&) noexcept;
  RendererApi& operator=(RendererApi&&) noexcept;

  RenderSubmitResult submitFrame(const FrameInput& frame);
  RenderSubmitResult resize(RenderViewport viewport);
  RenderReceipt diagnostics() const;
  RenderOutcome waitIdle();
  void shutdown();
  RendererLifecycleState lifecycleState() const;
  bool hasBackend() const;

private:
  explicit RendererApi(RenderReceipt diagnostics);
  friend RendererApi createRenderer(const RendererCreateInfo& createInfo);

  std::unique_ptr<RenderBackend> backend_;
  RendererLifecycleState lifecycleState_ = RendererLifecycleState::NotInitialized;
  RenderReceipt diagnostics_;
};

RendererApi createRenderer(const RendererCreateInfo& createInfo);
std::string_view rendererBackendKindName(RendererBackendKind kind);
std::string_view rendererLifecycleStateName(RendererLifecycleState state);

}  // namespace iggy3d
