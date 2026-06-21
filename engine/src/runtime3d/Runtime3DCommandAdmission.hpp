#pragma once

#include "runtime3d/Runtime3DCommand.hpp"
#include "runtime3d/Runtime3DWorldState.hpp"

namespace iggy::runtime3d {

struct Runtime3DCommandAdmissionResult {
	bool accepted = false;
	Runtime3DCommandRecord record;
};

class Runtime3DCommandAdmission {
public:
	[[nodiscard]] Runtime3DCommandAdmissionResult admit(
		const Runtime3DWorldState &world,
		Runtime3DCommandRecord record) const;
};

} // namespace iggy::runtime3d
