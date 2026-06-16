#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayAsciiScenarioPacket.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlan.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanValidator.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayAsciiSourcePlanAdapterStatus {
	Converted,
	SourcePlanInvalid,
	Unmappable,
};

enum class RuntimeGameplayAsciiSourcePlanAdapterIssueCode {
	SourcePlanInvalid,
	DuplicateScenarioMarkerGlyph,
	MissingActorMarkerId,
	MissingProfileId,
	UnsupportedAnnotatedCellMapping,
};

struct RuntimeGameplayAsciiSourcePlanAdapterIssue {
	RuntimeGameplayAsciiSourcePlanAdapterIssueCode code =
		RuntimeGameplayAsciiSourcePlanAdapterIssueCode::SourcePlanInvalid;
	std::size_t index = 0;
	std::size_t firstIndex = 0;
	std::size_t row = 0;
	std::size_t column = 0;
	char glyph = '\0';
};

struct RuntimeGameplayAsciiSourcePlanAdapterResult {
	RuntimeGameplayAsciiSourcePlan sourcePlan;
	RuntimeGameplayAsciiSourcePlanValidationResult validation;
	RuntimeGameplayAsciiScenarioPacket packet;
	std::vector<RuntimeGameplayAsciiSourcePlanAdapterIssue> issues;
	RuntimeGameplayAsciiSourcePlanAdapterStatus status =
		RuntimeGameplayAsciiSourcePlanAdapterStatus::Converted;
	std::size_t issueCount = 0;
	std::size_t markerCount = 0;
	std::size_t rowCount = 0;

	[[nodiscard]] bool converted() const;
};

class RuntimeGameplayAsciiSourcePlanToPacketAdapter {
public:
	[[nodiscard]] RuntimeGameplayAsciiSourcePlanAdapterResult convert(
		const RuntimeGameplayAsciiSourcePlan &sourcePlan) const;
};

} // namespace iggy::runtime
