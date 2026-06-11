#include "RuntimeOutputResultBuilder.hpp"

namespace dev {

RuntimeOutputResultBuilder::RuntimeOutputResultBuilder(RuntimeOutputResult output)
    : output_(output)
{
}

void RuntimeOutputResultBuilder::beginRunTraceSave()
{
	output_.runTraceSaveAttempted = true;
}

void RuntimeOutputResultBuilder::completeRunTraceSave(bool saved)
{
	output_.runTraceSaved = saved;
}

void RuntimeOutputResultBuilder::beginDebugBundleSave()
{
	output_.debugBundleSaveAttempted = true;
}

void RuntimeOutputResultBuilder::completeDebugBundleSave(bool saved)
{
	output_.debugBundleSaved = saved;
}

RuntimeOutputResult RuntimeOutputResultBuilder::result() const
{
	return output_;
}

GameLoopResult RuntimeOutputResultBuilder::applyTo(GameLoopResult result) const
{
	result.output = output_;
	return result;
}

} // namespace dev
