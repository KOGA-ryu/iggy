#pragma once

namespace iggy::runtime3d {

enum class Runtime3DCameraMode {
	RealtimeFirstPerson,
	RealtimeThirdPersonClose,
	TacticalOrbit,
	TacticalOverhead,
};

struct Runtime3DCameraModeState {
	Runtime3DCameraMode activeMode = Runtime3DCameraMode::RealtimeThirdPersonClose;
	Runtime3DCameraMode previousRealtimeMode = Runtime3DCameraMode::RealtimeThirdPersonClose;
	float yaw = 0.0F;
	float pitch = 0.0F;
	float orbitDistance = 8.0F;
};

[[nodiscard]] bool IsRuntime3DRealtimeCameraMode(Runtime3DCameraMode mode);
[[nodiscard]] bool IsRuntime3DTacticalCameraMode(Runtime3DCameraMode mode);

} // namespace iggy::runtime3d
