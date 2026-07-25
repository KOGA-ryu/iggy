#pragma once

#include <variant>

#include "EditorDesktopCommandPayloadCommon.hpp"
#include "EditorDesktopCommandPayloadObject.hpp"
#include "EditorDesktopCommandPayloadTerrain.hpp"
#include "EditorDesktopCommandPayloadWorldLayoutBuilding.hpp"
#include "EditorDesktopCommandPayloadWorldLayoutPlan.hpp"
#include "EditorDesktopCommandPayloadWorldLayoutReview.hpp"
#include "EditorDesktopCommandPayloadWorldLayoutSource.hpp"

namespace iggy3d_creative_app {

// The sole discriminated payload envelope. Alternative order is a compatibility
// contract because commands are fixed-layout, copyable frame values.
using CreativeDesktopCommandPayload = std::variant<
    std::monostate,
    CreativeDesktopSaveAsPayload,
    CreativeDesktopMapTemplatePayload,
    CreativeDesktopMeasurementAnnotationPayload,
    CreativeDesktopSelectPayload,
    CreativeDesktopLogicLinkPayload,
    CreativeDesktopRenamePayload,
    CreativeDesktopObjectFlagPayload,
    CreativeDesktopTransformPayload,
    CreativeDesktopGroupPivotPayload,
    CreativeDesktopMovingPlatformPayload,
    CreativeDesktopPlayerSpawnPayload,
    CreativeDesktopNpcSpawnPayload,
    CreativeDesktopLootPointPayload,
    CreativeDesktopExitPointPayload,
    CreativeDesktopMovingPlatformPreviewPayload,
    CreativeDesktopMovingPlatformWaypointPayload,
    CreativeDesktopTerrainOperationPayload,
    CreativeDesktopTerrainStampPayload,
    CreativeDesktopWorldLayoutToolPayload,
    CreativeDesktopWorldLayoutCatalogAssetPayload,
    CreativeDesktopWorldLayoutBuildingBlockoutPayload,
    CreativeDesktopWorldLayoutBuildingBlockoutUpdatePayload,
    CreativeDesktopWorldLayoutBuildingSelectionPayload,
    CreativeDesktopWorldLayoutSourcePayload,
    CreativeDesktopWorldLayoutObjectSourcePayload,
    CreativeDesktopWorldLayoutSourceRenamePayload,
    CreativeDesktopWorldLayoutLevelOperationPayload,
    CreativeDesktopWorldLayoutLevelSettingsPayload,
    CreativeDesktopWorldLayoutLevelDatumPayload,
    CreativeDesktopWorldLayoutRoofApertureCreatePayload,
    CreativeDesktopWorldLayoutRoofApertureManipulationPayload,
    CreativeDesktopWorldLayoutRoofManipulationPayload,
    CreativeDesktopGeneratedLevelSettingsPayload,
    CreativeDesktopWorldLayoutObjectSettingsPayload,
    CreativeDesktopWorldLayoutBuildingManipulationPayload,
    CreativeDesktopWorldLayoutBuildingDuplicatePayload,
    CreativeDesktopWorldLayoutBuildingTransformPayload,
    CreativeDesktopGeneratedBuildingOperationPayload,
    CreativeDesktopWorldLayoutBuildingGroundingPayload,
    CreativeDesktopWorldLayoutBuildingArchitecturePayload,
    CreativeDesktopWorldLayoutBuildingTemplateCapturePayload,
    CreativeDesktopWorldLayoutBuildingTemplateSyncPayload,
    CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload,
    CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload,
    CreativeDesktopWorldLayoutPointPayload,
    CreativeDesktopWorldLayoutGesturePayload,
    CreativeDesktopGeneratedRoomSettingsPayload,
    CreativeDesktopWorldLayoutRoomSplitPayload,
    CreativeDesktopWorldLayoutRoomMergePayload,
    CreativeDesktopWorldLayoutWallSplitPayload,
    CreativeDesktopWorldLayoutWallMergePayload,
    CreativeDesktopWorldLayoutRoomManipulationPayload,
    CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload,
    CreativeDesktopWorldLayoutRoomCornerManipulationPayload,
    CreativeDesktopGeneratedVerticalConnectorSettingsPayload,
    CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload,
    CreativeDesktopWorldLayoutBoxSettingsPayload,
    CreativeDesktopWorldLayoutBoxManipulationPayload,
    CreativeDesktopWorldLayoutWallSettingsPayload,
    CreativeDesktopGeneratedWallSettingsPayload,
    CreativeDesktopWorldLayoutWallManipulationPayload,
    CreativeDesktopWorldLayoutOpeningSettingsPayload,
    CreativeDesktopGeneratedOpeningSettingsPayload,
    CreativeDesktopWorldLayoutPropertyEditPayload,
    CreativeDesktopWorldLayoutOpeningInsertPayload,
    CreativeDesktopWorldLayoutAssetRepairPayload,
    CreativeDesktopWorldLayoutBuildingRepairPayload,
    CreativeDesktopWorldLayoutOpeningManipulationPayload,
    CreativeDesktopWorldLayoutConfirmPayload>;

}  // namespace iggy3d_creative_app
