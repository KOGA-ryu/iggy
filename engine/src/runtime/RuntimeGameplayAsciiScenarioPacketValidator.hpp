#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayAsciiScenarioPacket.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayAsciiScenarioPacketValidationStatus {
	Valid,
	Invalid,
};

enum class RuntimeGameplayAsciiScenarioPacketIssueCode {
	EmptyRows,
	RaggedRow,
	DuplicateMarker,
	UnknownMarkerKind,
	EmptyActorId,
	EmptyProfileId,
	UnknownGridMarker,
};

struct RuntimeGameplayAsciiScenarioPacketIssue {
	RuntimeGameplayAsciiScenarioPacketIssueCode code =
		RuntimeGameplayAsciiScenarioPacketIssueCode::EmptyRows;
	std::size_t markerIndex = 0;
	std::size_t firstMarkerIndex = 0;
	std::size_t rowIndex = 0;
	std::size_t columnIndex = 0;
	char marker = '\0';
	RuntimeGameplayAsciiScenarioMarkerDeclaration declaration;
};

struct RuntimeGameplayAsciiScenarioPacketValidationResult {
	RuntimeGameplayAsciiScenarioPacket packet;
	std::vector<RuntimeGameplayAsciiScenarioPacketIssue> issues;
	RuntimeGameplayAsciiScenarioPacketValidationStatus status =
		RuntimeGameplayAsciiScenarioPacketValidationStatus::Valid;
	std::size_t issueCount = 0;
	std::size_t rowCount = 0;
	std::size_t width = 0;
	std::size_t markerCount = 0;
	std::size_t emptyRowsCount = 0;
	std::size_t raggedRowCount = 0;
	std::size_t duplicateMarkerCount = 0;
	std::size_t unknownMarkerKindCount = 0;
	std::size_t emptyActorIdCount = 0;
	std::size_t emptyProfileIdCount = 0;
	std::size_t unknownGridMarkerCount = 0;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayAsciiScenarioPacketValidator {
public:
	[[nodiscard]] RuntimeGameplayAsciiScenarioPacketValidationResult validate(
		const RuntimeGameplayAsciiScenarioPacket &packet) const;
};

} // namespace iggy::runtime
