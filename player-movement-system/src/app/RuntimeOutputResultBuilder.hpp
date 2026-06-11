#pragma once

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeOutputResultBuilder {
public:
	explicit RuntimeOutputResultBuilder(RuntimeOutputResult output = RuntimeOutputResult {});

	void beginRunTraceSave();
	void completeRunTraceSave(bool saved);
	void beginDebugBundleSave();
	void completeDebugBundleSave(bool saved);

	[[nodiscard]] RuntimeOutputResult result() const;
	[[nodiscard]] GameLoopResult applyTo(GameLoopResult result) const;

private:
	RuntimeOutputResult output_;
};

} // namespace dev
