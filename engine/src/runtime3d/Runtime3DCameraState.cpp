#include "runtime3d/Runtime3DCameraState.hpp"

namespace iggy::runtime3d {

bool IsRuntime3DRealtimeCameraMode(Runtime3DCameraMode mode)
{
	return mode == Runtime3DCameraMode::RealtimeFirstPerson ||
		mode == Runtime3DCameraMode::RealtimeThirdPersonClose;
}

bool IsRuntime3DTacticalCameraMode(Runtime3DCameraMode mode)
{
	return mode == Runtime3DCameraMode::TacticalOrbit ||
		mode == Runtime3DCameraMode::TacticalOverhead;
}

} // namespace iggy::runtime3d
