#pragma once

#include "render/RendererApi.hpp"

namespace iggy3d {

class RenderBackend {
public:
  virtual ~RenderBackend() = default;

  virtual RendererBackendKind backendKind() const = 0;
  virtual RendererLifecycleState lifecycleState() const = 0;
  virtual RenderSubmitResult submitFrame(const FrameInput& frame) = 0;
  virtual RenderSubmitResult resize(RenderViewport viewport) = 0;
  virtual RenderReceipt diagnostics() const = 0;
  virtual RenderOutcome waitIdle() = 0;
  virtual void shutdown() = 0;
};

}  // namespace iggy3d
