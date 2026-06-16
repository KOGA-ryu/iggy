#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlan.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayAsciiSourcePlanValidationStatus {
	Valid,
	Invalid,
};

enum class RuntimeGameplayAsciiSourcePlanIssueCode {
	EmptyRows,
	GridDimensionMismatch,
	RaggedRow,
	EmptyGlyph,
	DuplicateGlyph,
	UnknownGridGlyph,
	AnnotatedCellOutOfBounds,
	AnnotatedCellGlyphMismatch,
	DuplicateAnnotatedCellId,
	InvalidRegionBounds,
	RegionOutOfBounds,
	AuthoredControlMissingNpcId,
	AuthoredControlUnsupportedBehavior,
	AuthoredControlUnsupportedMoveMode,
	AuthoredControlMissingTarget,
	EmptyActorMarkerId,
	EmptyProfileMarkerId,
	UnsafeNoClaims,
	UnsafePromotionPolicy,
};

struct RuntimeGameplayAsciiSourcePlanIssue {
	RuntimeGameplayAsciiSourcePlanIssueCode code =
		RuntimeGameplayAsciiSourcePlanIssueCode::EmptyRows;
	std::size_t index = 0;
	std::size_t firstIndex = 0;
	std::size_t row = 0;
	std::size_t column = 0;
	char glyph = '\0';
	ResourceId id;
};

struct RuntimeGameplayAsciiSourcePlanValidationResult {
	RuntimeGameplayAsciiSourcePlan plan;
	std::vector<RuntimeGameplayAsciiSourcePlanIssue> issues;
	RuntimeGameplayAsciiSourcePlanValidationStatus status =
		RuntimeGameplayAsciiSourcePlanValidationStatus::Valid;
	std::size_t issueCount = 0;
	std::size_t rowCount = 0;
	std::size_t width = 0;
	std::size_t height = 0;
	std::size_t legendCount = 0;
	std::size_t annotatedCellCount = 0;
	std::size_t regionCount = 0;
	std::size_t authoredControlCount = 0;
	std::size_t dimensionMismatchCount = 0;
	std::size_t raggedRowCount = 0;
	std::size_t duplicateGlyphCount = 0;
	std::size_t unknownGridGlyphCount = 0;
	std::size_t annotationIssueCount = 0;
	std::size_t regionIssueCount = 0;
	std::size_t authoredControlIssueCount = 0;
	std::size_t unsafeBoundaryIssueCount = 0;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayAsciiSourcePlanValidator {
public:
	[[nodiscard]] RuntimeGameplayAsciiSourcePlanValidationResult validate(
		const RuntimeGameplayAsciiSourcePlan &plan) const;
};

} // namespace iggy::runtime
