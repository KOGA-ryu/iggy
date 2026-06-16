#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeGameplayAsciiScenarioPacket.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayAsciiSourcePlanGlyphKind {
	Background,
	Terrain,
	Actor,
	PlayerStart,
	RegionMarker,
	Annotation,
	Unknown,
};

struct RuntimeGameplayAsciiSourcePlanGrid {
	std::size_t width = 0;
	std::size_t height = 0;
	std::vector<std::string> rows;
	char backgroundGlyph = ' ';

	[[nodiscard]] bool hasRows() const;
	[[nodiscard]] std::size_t rowCount() const;
};

struct RuntimeGameplayAsciiSourcePlanCellBox {
	bool present = false;
	double minX = 0.0;
	double minY = 0.0;
	double maxX = 0.0;
	double maxY = 0.0;
};

struct RuntimeGameplayAsciiSourcePlanLocalTile {
	bool present = false;
	int x = 0;
	int y = 0;
};

struct RuntimeGameplayAsciiSourcePlanLocalPosition {
	bool present = false;
	double x = 0.0;
	double y = 0.0;
};

enum class RuntimeGameplayAsciiSourcePlanControlBehavior {
	Unknown,
	Waiting,
	Seeking,
};

enum class RuntimeGameplayAsciiSourcePlanControlMoveMode {
	Unknown,
	Still,
	Walk,
	Jog,
	Run,
	Sprint,
};

enum class RuntimeGameplayAsciiSourcePlanPlayerCommandKind {
	Unknown,
	MoveToTile,
};

struct RuntimeGameplayAsciiSourcePlanGlyphLegendEntry {
	char glyph = '\0';
	RuntimeGameplayAsciiSourcePlanGlyphKind kind =
		RuntimeGameplayAsciiSourcePlanGlyphKind::Unknown;
	ResourceId roleId;
	std::vector<ResourceId> roleTags;
	bool mapsToScenarioMarker = false;
	RuntimeGameplayAsciiScenarioMarkerKind scenarioMarkerKind =
		RuntimeGameplayAsciiScenarioMarkerKind::Unknown;
	ResourceId targetMarkerId;
	ResourceId targetProfileId;
};

struct RuntimeGameplayAsciiSourcePlanAnnotatedCell {
	bool hasCellId = false;
	ResourceId cellId;
	std::size_t row = 0;
	std::size_t column = 0;
	char glyph = '\0';
	RuntimeGameplayAsciiSourcePlanLocalTile localTile;
	RuntimeGameplayAsciiSourcePlanLocalPosition localPosition;
	RuntimeGameplayAsciiSourcePlanCellBox cellBounds;
	std::vector<ResourceId> roleTags;
	ResourceId markerId;
	ResourceId profileId;
};

struct RuntimeGameplayAsciiSourcePlanRegion {
	bool hasRegionId = false;
	ResourceId regionId;
	std::size_t minRow = 0;
	std::size_t minColumn = 0;
	std::size_t maxRow = 0;
	std::size_t maxColumn = 0;
	std::vector<ResourceId> roleTags;
};

struct RuntimeGameplayAsciiSourcePlanAuthoredControl {
	bool hasFrameId = false;
	ResourceId frameId;
	ResourceId npcId;
	RuntimeGameplayAsciiSourcePlanControlBehavior behavior =
		RuntimeGameplayAsciiSourcePlanControlBehavior::Unknown;
	RuntimeGameplayAsciiSourcePlanControlMoveMode moveMode =
		RuntimeGameplayAsciiSourcePlanControlMoveMode::Unknown;
	RuntimeGameplayAsciiSourcePlanLocalPosition targetPosition;
	bool hasDeclarationIndex = false;
	std::size_t declarationIndex = 0;
};

struct RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand {
	bool hasFrameId = false;
	ResourceId frameId;
	RuntimeGameplayAsciiSourcePlanPlayerCommandKind command =
		RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Unknown;
	bool hasTargetTile = false;
	TileCoord targetTile;
	bool hasDeclarationIndex = false;
	std::size_t declarationIndex = 0;
};

struct RuntimeGameplayAsciiSourcePlanNoClaims {
	bool claimsRuntimeTruth = false;
	bool claimsGameplayExecution = false;
	bool claimsFileParsing = false;
	bool claimsProfileScenarioConversion = false;
};

struct RuntimeGameplayAsciiSourcePlanPromotionPolicy {
	bool promotionReady = false;
	bool allowsRuntimeExecution = false;
	bool allowsFileParsing = false;
	bool allowsProfileScenarioConversion = false;
};

struct RuntimeGameplayAsciiSourcePlan {
	ResourceId formatId;
	std::size_t version = 1;
	bool hasSourceId = false;
	ResourceId sourceId;
	bool hasSourceRef = false;
	ResourceId sourceRef;
	RuntimeGameplayAsciiSourcePlanGrid grid;
	std::vector<RuntimeGameplayAsciiSourcePlanGlyphLegendEntry> legend;
	std::vector<RuntimeGameplayAsciiSourcePlanAnnotatedCell> annotatedCells;
	std::vector<RuntimeGameplayAsciiSourcePlanRegion> regions;
	std::vector<RuntimeGameplayAsciiSourcePlanAuthoredControl> authoredControls;
	std::vector<RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand>
		authoredPlayerCommands;
	RuntimeGameplayAsciiSourcePlanNoClaims noClaims;
	RuntimeGameplayAsciiSourcePlanPromotionPolicy promotionPolicy;

	[[nodiscard]] bool hasRows() const;
	[[nodiscard]] std::size_t rowCount() const;
	[[nodiscard]] std::size_t legendCount() const;
	[[nodiscard]] std::size_t annotatedCellCount() const;
	[[nodiscard]] std::size_t regionCount() const;
	[[nodiscard]] std::size_t authoredControlCount() const;
	[[nodiscard]] std::size_t authoredPlayerCommandCount() const;
	[[nodiscard]] bool safeForAuthoring() const;
};

} // namespace iggy::runtime
