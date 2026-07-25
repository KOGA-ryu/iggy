#pragma once

#include <cstddef>
#include <string>

#include "EditorWorldLayoutState.hpp"

namespace iggy3d_creative_app {

struct CreativeDesktopWorldLayoutToolPayload {
  CreativeEditorWorldLayoutTool tool =
      CreativeEditorWorldLayoutTool::Select;
};

struct CreativeDesktopWorldLayoutCatalogAssetPayload {
  std::string assetId;
};

struct CreativeDesktopWorldLayoutSourcePayload {
  iggy3d::creative::CreativeWorldLayoutTable table =
      iggy3d::creative::CreativeWorldLayoutTable::None;
  std::size_t index =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  std::size_t preferredLevelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeDesktopWorldLayoutObjectSourcePayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
};

struct CreativeDesktopWorldLayoutSourceRenamePayload {
  iggy3d::creative::CreativeWorldLayoutTable table =
      iggy3d::creative::CreativeWorldLayoutTable::None;
  std::size_t index =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  std::string name;
};

struct CreativeDesktopWorldLayoutPointPayload {
  CreativeEditorWorldLayoutPoint point;
};

struct CreativeDesktopWorldLayoutGesturePayload {
  CreativeEditorWorldLayoutGesturePhase phase =
      CreativeEditorWorldLayoutGesturePhase::Begin;
  CreativeEditorWorldLayoutPoint point;
};

}  // namespace iggy3d_creative_app
