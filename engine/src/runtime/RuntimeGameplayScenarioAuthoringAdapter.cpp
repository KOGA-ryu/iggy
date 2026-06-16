#include "runtime/RuntimeGameplayScenarioAuthoringAdapter.hpp"

namespace iggy::runtime {

bool RuntimeGameplayScenarioAuthoringAdapterResult::ok() const
{
	return status == RuntimeGameplayScenarioAuthoringAdapterStatus::Converted;
}

RuntimeGameplayScenarioAuthoringAdapterResult RuntimeGameplayScenarioAuthoringAdapter::convert(
	const RuntimeGameplayScenarioAuthoringPacket &packet) const
{
	RuntimeGameplayScenarioAuthoringAdapterResult result;
	result.packet = packet;

	if (packet.source != RuntimeGameplayScenarioAuthoringSource::ProfileScenarioDefinition) {
		result.status = RuntimeGameplayScenarioAuthoringAdapterStatus::UnsupportedSource;
		return result;
	}

	if (!packet.hasProfileScenario) {
		result.status = RuntimeGameplayScenarioAuthoringAdapterStatus::InvalidPacket;
		return result;
	}

	result.profileScenario = packet.profileScenario;
	result.status = RuntimeGameplayScenarioAuthoringAdapterStatus::Converted;
	result.converted = true;
	return result;
}

} // namespace iggy::runtime
