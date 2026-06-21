#pragma once

#include <cstdint>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

enum class CameraMode : std::uint8_t {
  FirstPerson,
  ThirdPerson,
  TacticalOverhead,
};

struct CameraTarget {
  EntityId entity;
  Vec3 point;
  bool hasPoint = false;
};

struct CameraState {
  CameraMode activeMode = CameraMode::ThirdPerson;
  CameraMode previousRealtimeMode = CameraMode::ThirdPerson;
  CameraTarget target;
  float yawDegrees = 0.0F;
  float pitchDegrees = 0.0F;
  float orbitDistance = 8.0F;
  bool inputClearRequested = false;
};

}  // namespace iggy3d
