#include "runtime/RuntimeGameplayScenarioAuthoringAdapter.hpp"

namespace iggy::runtime {

bool RuntimeGameplayScenarioAuthoringAdapterResult::ok() const
{
	return status == RuntimeGameplayScenarioAuthoringAdapterStatus::Converted;
}

bool RuntimeGameplayScenarioAuthoringAdapterResult::hasIssues() const
{
	return !issues.empty();
}

RuntimeGameplayScenarioAuthoringAdapterResult RuntimeGameplayScenarioAuthoringAdapter::convert(
	const RuntimeGameplayScenarioAuthoringPacket &packet) const
{
	RuntimeGameplayScenarioAuthoringAdapterResult result;
	result.packet = packet;

	if (packet.source != RuntimeGameplayScenarioAuthoringSource::ProfileScenarioDefinition) {
		result.status = RuntimeGameplayScenarioAuthoringAdapterStatus::UnsupportedSource;
		result.issues.push_back({
			RuntimeGameplayScenarioAuthoringAdapterIssueCode::UnsupportedSource,
			packet.source,
			packet.hasProfileScenario,
		});
		result.issueCount = result.issues.size();
		return result;
	}

	if (!packet.hasProfileScenario) {
		result.status = RuntimeGameplayScenarioAuthoringAdapterStatus::InvalidPacket;
		result.issues.push_back({
			RuntimeGameplayScenarioAuthoringAdapterIssueCode::MissingProfileScenarioPayload,
			packet.source,
			packet.hasProfileScenario,
		});
		result.issueCount = result.issues.size();
		return result;
	}

	result.profileScenario = packet.profileScenario;
	result.status = RuntimeGameplayScenarioAuthoringAdapterStatus::Converted;
	result.converted = true;
	return result;
}

} // namespace iggy::runtime
