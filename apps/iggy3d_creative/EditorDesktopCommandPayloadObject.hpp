#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "app/iggy3d/creative/document/LogicLink.hpp"
#include "app/iggy3d/creative/document/Object.hpp"

namespace iggy3d_creative_app {

struct CreativeDesktopSelectPayload {
  std::vector<iggy3d::creative::CreativeObjectId> objectIds;
  iggy3d::creative::CreativeObjectId primaryObjectId =
      iggy3d::creative::kInvalidObjectId;
};

struct CreativeDesktopLogicLinkPayload {
  iggy3d::creative::CreativeObjectId sourceObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectId targetObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeLogicLinkAction action =
      iggy3d::creative::CreativeLogicLinkAction::Toggle;
};

struct CreativeDesktopRenamePayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::string name;
};

struct CreativeDesktopObjectFlagPayload {
  std::vector<iggy3d::creative::CreativeObjectId> objectIds;
  bool value = true;
};

struct CreativeDesktopTransformPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeTransform transform;
  bool setPosition = false;
  bool setRotation = false;
  bool setScale = false;
};

struct CreativeDesktopGroupPivotPayload {
  iggy3d::creative::CreativeObjectId groupObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeVec3 pivot;
};

struct CreativeDesktopMovingPlatformPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeMovingPlatformSettings settings;
};

struct CreativeDesktopPlayerSpawnPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativePlayerSpawnSettings settings;
};

struct CreativeDesktopNpcSpawnPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeNpcSpawnSettings settings;
};

struct CreativeDesktopLootPointPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeLootPointSettings settings;
};

struct CreativeDesktopExitPointPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeExitPointSettings settings;
};

struct CreativeDesktopMovingPlatformPreviewPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  double normalizedProgress = 0.0;
};

struct CreativeDesktopMovingPlatformWaypointPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t pointIndex = 0U;
  double dwellSeconds = 0.0;
};

}  // namespace iggy3d_creative_app
