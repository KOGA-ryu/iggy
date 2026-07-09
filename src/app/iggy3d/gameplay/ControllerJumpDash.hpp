#pragma once

#include <string_view>

namespace iggy3d {

class Session;
class SpatialSurfaceSet;
struct ProductAppWindowState;

void advanceProductJump(Session& session,
                        ProductAppWindowState& window,
                        const SpatialSurfaceSet* collisionSurfaces);

void submitProductJump(Session& session,
                       ProductAppWindowState& window,
                       std::string_view source);

void recordProductJumpPosition(ProductAppWindowState& window,
                               float groundY,
                               float startY,
                               float finalY);

void clearProductJumpTiming(ProductAppWindowState& window);

void rejectProductJump(ProductAppWindowState& window,
                       std::string_view status,
                       std::string_view reason);

bool productJumpBufferLive(const ProductAppWindowState& window);

void bufferProductJump(ProductAppWindowState& window);

void applyProductJumpReleaseCut(ProductAppWindowState& window);

void advanceProductDashCooldown(ProductAppWindowState& window);

void rejectProductDash(ProductAppWindowState& window,
                       std::string_view status,
                       std::string_view reason);

void submitProductDash(Session& session,
                       ProductAppWindowState& window,
                       float moveX,
                       float moveY,
                       std::string_view source,
                       const SpatialSurfaceSet* collisionSurfaces);

}  // namespace iggy3d
