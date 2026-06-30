# Dead-Code Cleanup Plan v0.1

Repo-wide dead/scaffolding sweep (grep-verified, 16-cluster scan). Root cause:
the slice-by-slice build added a builder + helper + unit test per slice, later
slices superseded them, and the old code + its passing test were never deleted.
~70+ confirmed dead symbols. Execute in build-gated batches: build + full test
suite after each; do not proceed until green. The compiler/linker is the safety
net for compile/link-time refs — a truly zero-ref removal cannot break the build.

## Status
- **Batch 1 DONE** (commit 3636790): whole dead files — FrontendReceipt, MenuRow.hpp,
  AutomationInput, GamepadSystemControls, room/VisualProof, room_editor/ObjectPalette,
  apps/iggy3d_validate_package + their tests + CMake regs. Build clean; failures 13→12
  (dropped the dead product_room_visual_proof_smoke).

## Batch 2 — dead functions/types inside live files (zero refs, no coverage drop)
Safe: no test references; host files stay in CMake. Build with Vulkan enabled (some
items are under IGGY3D_BUILD_VULKAN_BACKEND / #if IGGY3D_HAS_VULKAN).
- core: `stableHashBytes`, `addEnumByte` (StableHash); `entityIdFromUint64` (EntityId.hpp);
  `hasLocation` (Diagnostic.hpp). (Defer `operator<(EntityId)` — medium.)
- runtime: `CommandDebugLabel`, `isSessionControlCommand`, `isValidCommandAbility`,
  `isAccepted`, `isRejected` (Command.hpp); `CommandLog::findRejectedById`/`latestRejected`;
  `defaultSaveFileRoot` (SaveFileStore); `RuntimeMetricsResetPolicy`; `hasValidEntityIdentity`
  (EntityState); `PlayerRoster::upsertSlot`/`rebindActor`.
- render (Vulkan): `rendererRequirementName`; `createMaterialUnlitTexturedDescriptorLayout`
  (+orphaned `DescriptorLayoutCreateInfo`/`DescriptorLayoutResult`); `destroyDescriptorSetLayout`;
  `shaderStageFlag`; `vulkanCallContextName`; `VulkanMemoryAllocator::budgetSnapshot`
  (keep `AllocationBudgetSnapshot` until Batch 3 drops makeMemoryBudgetReceipt);
  `DebugValidation::counters`; `InstanceDeviceSurface::selectedDevice`;
  `BufferImageResources::allocator`; `FrameCapture::extent`; `DescriptorPoolRecord`,
  `MaterialDescriptorRecord` structs. (Keep the underlying `_` fields — internally live.)
- content/projection: `setFloorPositionCommand`, `roomEditCommandKindName` (EditableRoomDocument).
- frontend/app: `starterChildScreenForAction`; `buildSaveBrowserModelFromCatalog` (CatalogProjector);
  `canSoftDeleteProductSave` (Catalog); `productPauseSaveFlowKindName` (Flow);
  `productSaveMutationStatusName` (SaveBridge; keep the enum value); `recordProductVulkanMenuUnsupported`.
- world/ascii/input: `productBuiltinDungeonWorldTitle`; `productCustomDungeonSourceName`
  (keep `kCustomSourceName`); `asciiRoomDiagnosticSeverityError`; `asciiRoomCellKindName`;
  `actionForDeviceEvent`, `inputDeviceKindName`, `neutralInputFromName` (after Batch 1 AutomationInput);
  `pollGamepadControllerModeChordSample` (+maybe orphan struct), `pollGamepadRoomEditorActions`
  (keep `recordGamepadRoomEditorActions` — test-referenced).
- top app (safe subset): `AppConfig::strict` + `--strict`; `ProductSaveFlowOperation::Load`/`Recover`
  enum values + their name cases.
Follow-on hazards to grep right before removing: the orphaned support structs, the
`neutralInputFromName` ordering dep.

## Batch 2b (own reviewed PR, entry-point hazard)
`AppMode::ValidatePackage`/`Replay` + loadPath/replayPath/requestedRealtimeCamera/runtimeConfig
+ their CLI/validation/status-enum code. Zero reads but spans CLI + AppConfigStatus exhaustiveness.

## Batch 3 — test-only code + its tests (DROPS COVERAGE — needs owner sign-off)
Each has no production caller but a live test; removal edits/deletes that test. Some are
plausibly-intended API. Split into per-cluster PRs.
- **3a physics island (biggest win, own PR):** delete `PhysicsAabbStep`, `PhysicsAabbCollisionBatch`,
  `PhysicsBodyDeltaAccumulator`, `PhysicsBodyStore`, `PhysicsStep`, `PhysicsColliderBake`,
  `PhysicsShapeStore` (.cpp/.hpp) + their unit tests + CMake. First remove test-only consumers:
  `appendPhysicsCollisionBatchDebugProjection` (DebugProjection), the 4 island `PhysicsFrameStats`
  accumulators, `raycastPhysicsAabbs` (in live PhysicsCollisionQueries). EXCLUDE `PhysicsAabbContact`
  + `PhysicsAabbContactSolver` (tool-retained by kernel-bench) and `PhysicsSpatialSurfaceColliderBake`
  (live). Keep `accumulatePlayerPhysicsMovePlannerStats`.
- 3b math: `Plane`/`Ray3` (.{hpp,cpp}, coupled) + `translationMat4`/`scaleMat4`/`rotationEulerRadiansMat4`
  (keep Mat4 file); rework math_tests.
- 3c save/clock/command: `writeSessionSaveFile` (migrate tests to durable writer), `deleteSaveFile`,
  `acceptCommand`, `makeDefaultClockState`, `shouldRunAutomaticTick`,
  `requiresEntityTarget`/`requiresPointTarget`/`requiresAbilityPayload` (KEEP `requiresActor` — live).
- 3d world/camera/catalog: `WorldState::upsertEntity`, `makeDefaultCameraState`, `clearCameraInputRequest`,
  `canRecoverProductSave`, `productUiPrimitiveKindName`/`productUiToneName`.
- 3e content/frontend/render: `PackageValidator` (whole module, big test surgery — own PR),
  `appendRuntimeDebugSnapshot`, `setObjectPositionCommand`, `routeStarterBackFromChild`,
  `nextSettingsTab`/`previousSettingsTab`, `restoreFrontendSettingsDefaults`/`applyFrontendSettingsDraft`,
  BuiltinDungeon legacy getters, `makeMemoryBudgetReceipt`+`AllocationBudgetSnapshot`, `defaultBeanModelSize`.
- 3f ascii/mapmaker: `asciiRoomSourceOffset`, `asciiRoomCellAt`, `next`/`previousProductMapMakerGridPitch`,
  `inputAction{Feature,Owner,HandledBeforeMenu,KeepsGameplayActive}Name` (keep `inputActionDescriptor`).

## Batch 4 — none. tests/unit `expect`/fixture duplication is intentional; consolidating is a
separate "introduce tests/unit/TestSupport.hpp" refactor, not a deletion.

## DO NOT TOUCH (looked dead, verified live)
PhysicsAabbContact/Solver (kernel-bench); PhysicsSpatialSurfaceColliderBake;
accumulatePlayerPhysicsMovePlannerStats; queryPhysicsAabbOverlaps/sweepPhysicsAabb/checkPhysicsGround
(only raycast is dead); requiresActor; rejectCommand; writeSessionSaveFileDurably/softDeleteProductSave;
toUint64/operator==/!= (EntityId); Mat4 identity/at/transformPoint; StableHasher/addVec3Quantized;
defaultFrontendSettings; ProductFrontendSurface::ConfirmDialog + dispatchConfirmDialog (LIVE save-delete
confirm); FrontendSaveBrowserMode::Delete/SaveSlotCommand::Delete/saveSlotCommandName; AutomationSaveBrowser
deleted-save enum values; BeanModelKind::CodexProbe; recordGamepadRoomEditorActions; iggy3d_replay_tool
(shipped+installed — owner confirm); tests/unit harness duplication; all kept Vulkan/SDL #ifdef items.
