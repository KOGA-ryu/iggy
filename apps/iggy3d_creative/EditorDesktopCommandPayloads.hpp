#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "app/iggy3d/creative/assets/AuthoredAsset.hpp"
#include "app/iggy3d/creative/document/LogicLink.hpp"

namespace iggy3d_creative_app {

// Typed payloads for the desktop command families (Step 3 / plan DD-7). Each
// command id reads exactly one payload alternative; a mismatched id/payload is a
// no-op failure in the dispatcher, never a reinterpretation. This replaces the
// former single std::string arg so widgets (Step 4+) depend on a typed contract.
// The payload lives inside a fixed-capacity, copyable command frame; the vector/
// string members keep it copyable and default-constructible.

// SaveDocumentAs: the target save id (the former bare std::string arg).
struct CreativeDesktopSaveAsPayload {
  std::string saveId;
};

// SelectObjects: replace the persistent selection with these ids. primaryObjectId
// picks the primary; kInvalidObjectId falls back to the last surviving id.
// ClearSelection ignores its payload (an empty list selects nothing).
struct CreativeDesktopSelectPayload {
  std::vector<iggy3d::creative::CreativeObjectId> objectIds;
  iggy3d::creative::CreativeObjectId primaryObjectId =
      iggy3d::creative::kInvalidObjectId;
};

// SetLogicSource reads sourceObjectId. SetLogicLink and RemoveLogicLink read
// both endpoints; RemoveLogicLink ignores action. One typed payload keeps the
// desktop dispatcher aligned with the canonical CreativeLogicLink contract.
struct CreativeDesktopLogicLinkPayload {
  iggy3d::creative::CreativeObjectId sourceObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectId targetObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeLogicLinkAction action =
      iggy3d::creative::CreativeLogicLinkAction::Toggle;
};

// DeleteObjects: an explicit id list, or the current selection when empty.
struct CreativeDesktopDeletePayload {
  std::vector<iggy3d::creative::CreativeObjectId> objectIds;
};

// RenameObject: absolute rename of a single object.
struct CreativeDesktopRenamePayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::string name;
};

// SetObjectsVisible / SetObjectsLocked: an absolute bool over an id list (or the
// current selection when empty).
struct CreativeDesktopObjectFlagPayload {
  std::vector<iggy3d::creative::CreativeObjectId> objectIds;
  bool value = true;
};

// SetObjectTransform: absolute transform of a single object. The component flags
// select which of position/rotation/scale to write.
struct CreativeDesktopTransformPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeTransform transform;
  bool setPosition = false;
  bool setRotation = false;
  bool setScale = false;
};

// EditAssetSource lifecycle phase (unused by the other asset ops).
enum class CreativeDesktopAssetEditPhase : std::uint8_t {
  None,
  Begin,
  Save,
  Cancel,
};

// EquipAsset / EditAssetSource / RenameAsset / DuplicateAsset / DeleteAsset.
// name is used only by RenameAsset; editPhase only by EditAssetSource.
struct CreativeDesktopAssetOpPayload {
  std::string assetId;
  std::string name;
  CreativeDesktopAssetEditPhase editPhase = CreativeDesktopAssetEditPhase::None;
};

// RefreshInstances (uses mode) / UpdateAssetFromInstance (ignores mode). Both
// key off the in-document instance-root/group object id.
struct CreativeDesktopInstanceRefreshPayload {
  iggy3d::creative::CreativeObjectId instanceRootObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeAuthoredAssetRefreshMode mode =
      iggy3d::creative::CreativeAuthoredAssetRefreshMode::ForceAll;
};

// The discriminated payload carried by every command (monostate = no payload).
using CreativeDesktopCommandPayload = std::variant<
    std::monostate,
    CreativeDesktopSaveAsPayload,
    CreativeDesktopSelectPayload,
    CreativeDesktopLogicLinkPayload,
    CreativeDesktopDeletePayload,
    CreativeDesktopRenamePayload,
    CreativeDesktopObjectFlagPayload,
    CreativeDesktopTransformPayload,
    CreativeDesktopAssetOpPayload,
    CreativeDesktopInstanceRefreshPayload>;

}  // namespace iggy3d_creative_app
