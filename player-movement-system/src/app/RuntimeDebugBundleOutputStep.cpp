#include "RuntimeDebugBundleOutputStep.hpp"

#include <utility>

namespace dev {

RuntimeDebugBundleOutputStep::RuntimeDebugBundleOutputStep(RuntimeDebugArtifactBundle debugBundle)
    : debugBundle_(std::move(debugBundle))
{
}

void RuntimeDebugBundleOutputStep::save(
    const std::filesystem::path &path,
    const GameLoopResult &result,
    RuntimeOutputResultBuilder &output) const
{
	output.beginDebugBundleSave();
	output.completeDebugBundleSave(debugBundle_.save(path, output.applyTo(result)).saved());
}

} // namespace dev
