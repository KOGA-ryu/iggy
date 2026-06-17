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
	return convert(packet, {});
}

RuntimeGameplayScenarioAuthoringAdapterResult RuntimeGameplayScenarioAuthoringAdapter::convert(
	const RuntimeGameplayScenarioAuthoringPacket &packet,
	const RuntimeGameplayScenarioAuthoringAdapterConfig &config) const
{
	RuntimeGameplayScenarioAuthoringAdapterResult result;
	result.packet = packet;
	result.config = config;

	if (packet.source == RuntimeGameplayScenarioAuthoringSource::ProfileScenarioDefinition) {
		if (!packet.hasProfileScenario) {
			result.status = RuntimeGameplayScenarioAuthoringAdapterStatus::InvalidPacket;
			result.issues.push_back({
				RuntimeGameplayScenarioAuthoringAdapterIssueCode::MissingProfileScenarioPayload,
				packet.source,
				packet.hasProfileScenario,
				packet.hasAsciiSourcePlan,
				config.hasAsciiSourcePlanProfileScenarioConfig,
			});
			result.issueCount = result.issues.size();
			return result;
		}

		result.profileScenario = packet.profileScenario;
		result.status = RuntimeGameplayScenarioAuthoringAdapterStatus::Converted;
		result.converted = true;
		return result;
	}

	if (packet.source == RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan) {
		if (!packet.hasAsciiSourcePlan) {
			result.status = RuntimeGameplayScenarioAuthoringAdapterStatus::InvalidPacket;
			result.issues.push_back({
				RuntimeGameplayScenarioAuthoringAdapterIssueCode::MissingAsciiSourcePlanPayload,
				packet.source,
				packet.hasProfileScenario,
				packet.hasAsciiSourcePlan,
				config.hasAsciiSourcePlanProfileScenarioConfig,
			});
			result.issueCount = result.issues.size();
			return result;
		}

		const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig
			conversionConfig = config.hasAsciiSourcePlanProfileScenarioConfig
			? config.asciiSourcePlanProfileScenario
			: RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig {};
		result.asciiSourcePlanConversion =
			RuntimeGameplayAsciiSourcePlanProfileScenarioConverter {}.convert(
				packet.asciiSourcePlan,
				conversionConfig);
		if (result.asciiSourcePlanConversion.ok()) {
			result.profileScenario = result.asciiSourcePlanConversion.definition;
			result.status = RuntimeGameplayScenarioAuthoringAdapterStatus::Converted;
			result.converted = true;
			return result;
		}

		result.status = RuntimeGameplayScenarioAuthoringAdapterStatus::ConversionFailed;
		result.issues.push_back({
			RuntimeGameplayScenarioAuthoringAdapterIssueCode::AsciiSourcePlanConversionFailed,
			packet.source,
			packet.hasProfileScenario,
			packet.hasAsciiSourcePlan,
			config.hasAsciiSourcePlanProfileScenarioConfig,
			result.asciiSourcePlanConversion.status,
		});
		result.issueCount = result.issues.size();
		return result;
	}

	if (packet.source != RuntimeGameplayScenarioAuthoringSource::ProfileScenarioDefinition) {
		result.status = RuntimeGameplayScenarioAuthoringAdapterStatus::UnsupportedSource;
		result.issues.push_back({
			RuntimeGameplayScenarioAuthoringAdapterIssueCode::UnsupportedSource,
			packet.source,
			packet.hasProfileScenario,
			packet.hasAsciiSourcePlan,
			config.hasAsciiSourcePlanProfileScenarioConfig,
		});
		result.issueCount = result.issues.size();
		return result;
	}
	return result;
}

} // namespace iggy::runtime
