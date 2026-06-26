#pragma once

#include <filesystem>
#include <functional>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/frontend/WorldSetupModel.hpp"
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
  SaveSelect,
  SaveDelete,
  SaveShowDeleted,
  SaveDeletedSelect,
  SaveRecover,
  GameplayMoveX,
  GameplayMoveY,
  GameplayAttack,
  GameplayInteract,
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

struct ProductAutomationCommand {
  std::string key;
  std::string value;
};

struct FrontendState;
struct ProductAppWindowState;
enum class FrontendSettingsTab : int;
enum class MenuOwner : std::uint8_t;

struct ProductAutomationExecutionContext {
  FrontendState& frontend;
  FrontendSettingsTab& settingsTab;
  ProductAppWindowState& window;
  std::function<MenuOwner()> currentOwner;
  std::function<bool(InputAction)> routeInput;
};

struct ProductAutomationExecutionResult {
  bool handled = false;
  bool accepted = false;
};

struct ProductAutomationWorldSetupContext {
  FrontendState& frontend;
  WorldSetupDraft& worldSetupDraft;
  ProductAppWindowState& window;
  std::function<MenuOwner()> currentOwner;
  std::function<bool()> activateAsciiRoom;
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

enum class ProductDungeonDraftDirection : std::uint8_t;

struct ProductDungeonDraftDirectionAutomationResult {
  bool valid = false;
  ProductDungeonDraftDirection direction{};
};

struct ProductDungeonDraftPaintAutomationResult {
  bool valid = false;
  std::string_view glyph = "";
};

struct ProductDungeonDraftCellAutomationResult {
  bool valid = false;
  std::size_t row = 0;
  std::size_t column = 0;
};

struct ProductDungeonDraftCursor;
struct ProductDungeonDraftOperationResult;

ProductBoolAutomationResult resolveProductAutomationBool(
    std::string_view value);

bool resolveProductAutomationBool(std::string_view value, bool& out);

ProductGameplayAxisAutomationResult resolveProductGameplayAxisAutomation(
    std::string_view value);

ProductNonEmptyStringAutomationResult resolveProductNonEmptyStringAutomation(
    std::string_view value);

ProductDungeonDraftDirectionAutomationResult resolveProductDungeonDraftDirectionAutomation(
    std::string_view value);

ProductDungeonDraftPaintAutomationResult resolveProductDungeonDraftPaintAutomation(
    std::string_view value);

ProductDungeonDraftCellAutomationResult resolveProductDungeonDraftCellAutomation(
    std::string_view rowValue,
    std::string_view columnValue,
    std::string_view glyphValue);

struct ProductAppWindowState;
struct RoomEditCommand;
enum class ProductRoomEditorDirection : std::uint8_t;
enum class ProductRoomEditorTool : std::uint8_t;

std::vector<std::string_view> splitProductAutomationCsv(
    std::string_view value);

bool parseProductAutomationOwner(std::string_view value, MenuOwner& out);

bool parseProductAutomationFloorCommand(std::string_view value,
                                        RoomEditCommand& command);

bool parseProductAutomationWallCommand(std::string_view value,
                                       RoomEditCommand& command);

bool parseProductRoomEditorDirection(std::string_view value,
                                     ProductRoomEditorDirection& out);

bool parseProductRoomEditorTool(std::string_view value,
                                ProductRoomEditorTool& out);

bool parseProductRoomEditorInputAction(std::string_view value,
                                       InputAction& out,
                                       float& actionValue);

bool readProductAutomationCommands(const std::filesystem::path& path,
                                   ProductAppWindowState& window,
                                   std::vector<ProductAutomationCommand>& commands);

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

void markAutomationApplied(ProductAppWindowState& window,
                           const ProductAutomationCommand& command,
                           std::string_view action,
                           MenuOwner owner,
                           std::string_view result);

ProductAutomationExecutionResult applyProductCommonAutomationCommand(
    const ProductAutomationCommand& command,
    const ProductAutomationCommandDispatchSpec& automationSpec,
    ProductAutomationExecutionContext& context);

void recordWorldSetupDraftState(const WorldSetupDraft& draft,
                                ProductAppWindowState& window);

ProductDungeonDraftCursor dungeonDraftCursorFromWindow(
    const ProductAppWindowState& window);

void recordDungeonDraftOperation(ProductAppWindowState& window,
                                 const ProductDungeonDraftOperationResult& result);

void resetDungeonDraftWindowCursor(const WorldSetupDraft& draft,
                                   ProductAppWindowState& window);

bool applyDungeonDraftPaintGlyph(WorldSetupDraft& worldSetupDraft,
                                 ProductAppWindowState& window,
                                 char glyph);

ProductAutomationExecutionResult applyProductWorldSetupAutomationCommand(
    const ProductAutomationCommand& command,
    std::string_view canonicalKey,
    ProductAutomationWorldSetupContext& context);

enum class ProductRoomEditorDirection : std::uint8_t;
enum class ProductRoomEditorTool : std::uint8_t;

}  // namespace iggy3d
