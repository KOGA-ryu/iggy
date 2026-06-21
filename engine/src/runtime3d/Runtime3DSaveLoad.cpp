#include "runtime3d/Runtime3DSaveLoad.hpp"

namespace iggy::runtime3d {

Runtime3DSaveLoadResult SaveRuntime3DSessionPlaceholder(const Runtime3DSessionState &state)
{
	Runtime3DSaveLoadResult result;
	result.state = state;
	return result;
}

Runtime3DSaveLoadResult LoadRuntime3DSessionPlaceholder(const Runtime3DSaveEnvelope &envelope)
{
	Runtime3DSaveLoadResult result;
	if (envelope.status == Runtime3DSaveEnvelopeStatus::Incompatible) {
		result.state.lifecycle = Runtime3DSessionLifecycle::IncompatibleSave;
		result.reason = "runtime3d save envelope is incompatible";
	}
	return result;
}

} // namespace iggy::runtime3d
