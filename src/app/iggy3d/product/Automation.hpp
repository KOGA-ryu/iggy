#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/input/ActionState.hpp"
#include "app/input/InputAction.hpp"

namespace iggy3d {

enum class ProductAutomationCommandCategory : std::uint8_t {
  Owner,
  MenuInput,
  MenuShortcut,
  FrontendSelect,
  FrontendExecute,
  WorldSetup,
  DungeonDraft,
  AsciiRoom,
  SaveBrowser,
  DeletedSaveBrowser,
  RoomEdit,
  RoomEditor,
  GameplayInput,
  GameplayTape,
  Settings,
  DevTools,
  System,
  Unknown,
};

enum class ProductAutomationValueKind : std::uint8_t {
  Bool,
  String,
  Float,
  Size,
  Csv,
  Action,
  Direction,
  Tool,
  Raw,
  Unknown,
};

enum class ProductAutomationCommandId : std::uint8_t {
  MenuShortcut,
  RoomEditStart,
  RoomEditStartActive,
  RoomEditAddFloor,
  RoomEditAddWall,
  RoomEditDeleteFloor,
  RoomEditDeleteWall,
  RoomEditUndo,
  RoomEditRedo,
  RoomEditorMove,
  RoomEditorTool,
  RoomEditorCycleTool,
  RoomEditorWallDirection,
  RoomEditorPlace,
  Unknown,
};

struct ProductAutomationCommandSpec {
  std::string canonicalKey;
  std::vector<std::string> aliases;
  ProductAutomationCommandCategory category =
      ProductAutomationCommandCategory::Unknown;
  ProductAutomationValueKind valueKind = ProductAutomationValueKind::Unknown;
  bool requiresValue = true;
  bool ignoresFalseBool = false;
  std::string ownerHint;
};

// Owns the full set of known automation keys, aliases, and metadata.
struct ProductAutomationCommandRegistry {
  std::vector<ProductAutomationCommandSpec> specs;
};

// Owns the subset of registry rows this runtime dispatch path handles directly.
struct ProductAutomationCommandDispatchSpec {
  bool handled = false;
  std::string_view canonicalKey = "unknown";
  ProductAutomationCommandId commandId = ProductAutomationCommandId::Unknown;
  ProductAutomationCommandCategory category =
      ProductAutomationCommandCategory::Unknown;
  ProductAutomationValueKind valueKind = ProductAutomationValueKind::Unknown;
  InputAction inputAction = InputAction::None;
};

struct ProductAutomationCommandDispatchRequest {
  const ProductAutomationCommandRegistry* registry = nullptr;
  std::string_view key;
};

struct ProductAutomationCommandDispatchResult {
  bool handled = false;
  bool accepted = false;
  std::string_view status = "unknown";
  std::string_view reasonCode = "unknown";
  std::string_view canonicalActionLabel = "unknown";
  std::string_view ownerResult = "none";
  bool closeRequested = false;
  ProductAutomationCommandDispatchSpec spec;
};

struct ProductMenuShortcutAutomationResult {
  bool valid = false;
  bool routeRequested = false;
  InputAction inputAction = InputAction::None;
};

struct ProductMenuInputAutomationResult {
  bool valid = false;
  InputAction inputAction = InputAction::None;
};

enum class FrontendAction : std::uint8_t;

struct ProductFrontendSelectAutomationResult {
  bool valid = false;
  FrontendAction action{};
};

enum class FrontendSettingsTab : int;
enum class FrontendDevToolsCategory : std::uint8_t;

struct ProductSettingsTabAutomationResult {
  bool valid = false;
  FrontendSettingsTab settingsTab{};
};

struct ProductDevToolsCategoryAutomationResult {
  bool valid = false;
  FrontendDevToolsCategory category{};
};

struct ProductSaveSelectionAutomationResult {
  bool valid = false;
  std::string_view saveId = "";
};

struct ProductBoolAutomationResult {
  bool valid = false;
  bool requested = false;
};

struct ProductGameplayAxisAutomationResult {
  bool valid = false;
  float value = 0.0F;
};

struct ProductNonEmptyStringAutomationResult {
  bool valid = false;
  std::string_view value = "";
};

ProductBoolAutomationResult resolveProductAutomationBool(
    std::string_view value);

bool resolveProductAutomationBool(std::string_view value, bool& out);

ProductGameplayAxisAutomationResult resolveProductGameplayAxisAutomation(
    std::string_view value);

ProductNonEmptyStringAutomationResult resolveProductNonEmptyStringAutomation(
    std::string_view value);

bool resolveProductSaveBrowserBoolAutomation(std::string_view value, bool& out);

std::string_view productAutomationCommandCategoryName(
    ProductAutomationCommandCategory category);

std::string_view productAutomationValueKindName(
    ProductAutomationValueKind valueKind);

std::string_view productAutomationCommandIdName(
    ProductAutomationCommandId commandId);

ProductAutomationCommandRegistry makeProductAutomationCommandRegistry();

const ProductAutomationCommandSpec& findProductAutomationCommandSpec(
    const ProductAutomationCommandRegistry& registry,
    std::string_view key);

std::string_view productAutomationCanonicalKey(
    const ProductAutomationCommandRegistry& registry,
    std::string_view key);

const ProductAutomationCommandDispatchSpec& findProductAutomationCommandDispatchSpec(
    std::string_view canonicalKey);

ProductAutomationCommandDispatchResult resolveProductAutomationCommandDispatch(
    ProductAutomationCommandDispatchRequest request);

ProductMenuShortcutAutomationResult resolveProductMenuShortcutAutomation(
    const ProductAutomationCommandDispatchSpec& spec,
    std::string_view value);

ProductMenuInputAutomationResult resolveProductMenuInputAutomation(
    std::string_view value);

ProductFrontendSelectAutomationResult resolveProductFrontendSelectAutomation(
    std::string_view value);

ProductSettingsTabAutomationResult resolveProductSettingsTabAutomation(
    std::string_view value);

ProductDevToolsCategoryAutomationResult resolveProductDevToolsCategoryAutomation(
    std::string_view value);

ProductSaveSelectionAutomationResult resolveProductSaveSelectionAutomation(
    std::string_view value);

struct ProductRoomEditorCursorResult;
struct ProductRoomEditorActionResult;
struct ProductRoomEditorCursorState;
struct ProductRoomEditingStartResult;
struct ProductRoomEditingOperationResult;
struct ProductAsciiRoomAuthoringRequest;
struct ProductActiveRoomState;
struct ProductRoomEditingState;
struct ProductRoomAuthoringStartFromAsciiRequest;
struct ProductRoomAuthoringStartFromActiveRoomRequest;
struct ProductRoomAuthoringEditCommandRequest;
struct ProductRoomAuthoringUndoRedoRequest;
enum class ProductRoomEditorDirection : std::uint8_t;
enum class ProductRoomEditorTool : std::uint8_t;
enum class ProductRoomAuthoringInputSource : std::uint8_t;

ProductRoomEditorCursorResult applyProductRoomEditorMoveAutomation(
    ProductRoomEditorCursorState state,
    ProductRoomEditorDirection direction);

ProductRoomEditorCursorResult applyProductRoomEditorToolAutomation(
    ProductRoomEditorCursorState state,
    ProductRoomEditorTool tool);

ProductRoomEditorCursorResult applyProductRoomEditorCycleToolAutomation(
    ProductRoomEditorCursorState state);

ProductRoomEditorCursorResult applyProductRoomEditorWallDirectionAutomation(
    ProductRoomEditorCursorState state,
    ProductRoomEditorDirection direction);

ProductRoomEditorActionResult applyProductRoomEditorPlaceAutomation(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    ProductRoomAuthoringInputSource inputSource);

ProductRoomEditorActionResult applyProductEditorInputAutomation(
    const ProductRoomEditingState& editing,
    ProductRoomEditorCursorState cursor,
    const ActionState& actions,
    ProductRoomAuthoringInputSource inputSource);

ProductRoomEditingStartResult startProductRoomEditAutomationFromAsciiDraft(
    ProductRoomAuthoringStartFromAsciiRequest request);

ProductRoomEditingStartResult startProductRoomEditAutomationFromActiveRoom(
    ProductRoomAuthoringStartFromActiveRoomRequest request);

ProductRoomEditingOperationResult applyProductRoomEditAutomation(
    ProductRoomAuthoringEditCommandRequest request);

ProductRoomEditingOperationResult undoProductRoomEditAutomation(
    ProductRoomAuthoringUndoRedoRequest request);

ProductRoomEditingOperationResult redoProductRoomEditAutomation(
    ProductRoomAuthoringUndoRedoRequest request);

}  // namespace iggy3d
