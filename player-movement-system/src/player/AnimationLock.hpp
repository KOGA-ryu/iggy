#pragma once

namespace dev {

struct AnimationLock {
	bool active = false;
	float elapsedSeconds = 0.0F;
	float cancelAfterSeconds = 0.0F;

	[[nodiscard]] bool canCancel() const
	{
		return !active || elapsedSeconds >= cancelAfterSeconds;
	}
};

} // namespace dev

