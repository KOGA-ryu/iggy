#include "app/iggy3d/AppShell.hpp"

#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "app/PackageRuntimeLookup.hpp"
#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/ProductAppOperations.hpp"
#include "app/iggy3d/ProductCameraController.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductAsciiRoomActivation.hpp"
#include "app/iggy3d/product/AutomationControl.hpp"
#include "app/iggy3d/product/AutomationDispatch.hpp"
#include "app/iggy3d/product/AutomationRoomEditing.hpp"
#include "app/iggy3d/product/ProductMenuInputRouter.hpp"
#include "app/iggy3d/ProductBuiltinDungeon.hpp"
#include "app/iggy3d/ProductGameplayController.hpp"
#include "app/iggy3d/ProductGameplayFeedback.hpp"
#include "app/iggy3d/ProductGameplayProjectionRefresh.hpp"
#include "app/iggy3d/ProductGameplayTape.hpp"
#include "app/iggy3d/ProductGameplayTapeRunner.hpp"
#include "app/iggy3d/ProductMenuTransitions.hpp"
#include "app/iggy3d/ProductMovementDebugHud.hpp"
#include "app/iggy3d/ProductNpcBehaviorDebugHud.hpp"
#include "app/iggy3d/ProductPrimitiveDrawList.hpp"
#include "app/iggy3d/ProductRenderBridge.hpp"
#include "app/iggy3d/ProductRoomEditorActionController.hpp"
#include "app/iggy3d/ProductRoomEditorOverlay.hpp"
#include "app/iggy3d/ProductScriptedGameplayDriver.hpp"
#include "app/iggy3d/ProductViewportFraming.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/SaveBridge.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "app/input/GamepadInput.hpp"
#include "app/input/KeyboardInput.hpp"
#include "app/input/MouseInput.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/RenderDiagnostics.hpp"
#include "render/RendererApi.hpp"
#include "runtime/session/Session.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <chrono>
#include <thread>

#include <SDL3/SDL.h>

#include "app/iggy3d/OpeningMenuView.hpp"
#include "app/platform/SdlWindow.hpp"
#endif

#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_APP_VULKAN_BACKEND)
#include "app/platform/SdlVulkanSurface.hpp"
#include "render/vulkan/VulkanBackend.hpp"
#endif

namespace iggy3d {

namespace {

FrontendInputBackend settingsInputBackendFromOptions(ProductInputBackend backend) {
  switch (backend) {
    case ProductInputBackend::Keyboard:
      return FrontendInputBackend::Keyboard;
    case ProductInputBackend::Gamepad:
      return FrontendInputBackend::Gamepad;
    case ProductInputBackend::Auto:
      return FrontendInputBackend::Auto;
  }
  return FrontendInputBackend::Auto;
}

FrontendRendererChoice settingsRendererFromOptions(ProductRendererRequest renderer) {
  switch (renderer) {
    case ProductRendererRequest::Null:
      return FrontendRendererChoice::Null;
    case ProductRendererRequest::Vulkan:
      return FrontendRendererChoice::Vulkan;
  }
  return FrontendRendererChoice::Null;
}

FrontendWindowMode settingsWindowModeFromOptions(ProductWindowMode mode) {
  switch (mode) {
    case ProductWindowMode::NoWindow:
      return FrontendWindowMode::NoWindow;
    case ProductWindowMode::Window:
      return FrontendWindowMode::Window;
  }
  return FrontendWindowMode::NoWindow;
}

FrontendSettings productFrontendSettingsFromOptions(const ProductAppOptions& options) {
  FrontendSettings settings = defaultFrontendSettings();
  settings.inputBackend = settingsInputBackendFromOptions(options.inputBackend);
  settings.renderer = settingsRendererFromOptions(options.renderer);
  settings.windowMode = settingsWindowModeFromOptions(options.windowMode);
  settings.cameraMode = FrontendCameraMode::FirstPerson;
  settings.devToolsEnabled = true;
  settings.debugOverlayEnabled = true;
  return settings;
}

std::string receiptFieldValueOr(const RenderReceipt& receipt,
                                std::string_view key,
                                std::string_view fallback) {
  for (const RenderReceiptField& field : receipt.fields) {
    if (field.key == key) {
      return field.value;
    }
  }
  return std::string(fallback);
}

void recordProductVulkanRendererUnavailable(ProductAppWindowState& window,
                                            std::string_view reasonCode) {
  window.productVulkanRendererCreated = false;
  window.productVulkanRendererReady = false;
  window.productVulkanSurfaceCreated = false;
  window.productVulkanSwapchainReady = false;
  window.productVulkanStatus = "renderer_unavailable";
  window.productVulkanReasonCode = std::string(reasonCode);
}

void recordProductVulkanRendererReady(ProductAppWindowState& window,
                                      const RendererApi& renderer) {
  const RenderReceipt diagnostics = renderer.diagnostics();
  window.productVulkanRendererCreated = renderer.hasBackend();
  window.productVulkanRendererReady =
      renderer.lifecycleState() == RendererLifecycleState::Ready;
  window.productVulkanSurfaceCreated =
      window.productVulkanRendererReady ||
      hasReceiptField(diagnostics, "surface_ready", "true");
  window.productVulkanSwapchainReady =
      window.productVulkanRendererReady ||
      receiptFieldValueOr(diagnostics, "swapchain_state", "none") == "ready";
  window.productVulkanStatus = window.productVulkanRendererReady
                                   ? "renderer_ready"
                                   : "renderer_unavailable";
  window.productVulkanReasonCode =
      receiptFieldValueOr(diagnostics, "reason_code", "renderer_unavailable");
}

void recordProductVulkanSubmit(ProductAppWindowState& window,
                               const RenderSubmitResult& submit) {
  window.productVulkanRenderingPath =
      receiptFieldValueOr(submit.receipt, "rendering_path", "none");
  window.productVulkanRecordMode =
      receiptFieldValueOr(submit.receipt, "record_mode", "none");
  window.productVulkanReasonCode = std::string(submit.reason.code);
  if (submit.outcome == RenderOutcome::Ok) {
    window.productVulkanSurfaceCreated = true;
    window.productVulkanSwapchainReady = true;
    window.productVulkanFrameSubmitted = true;
    ++window.productVulkanFrameSubmittedCount;
    window.productVulkanStatus = "frame_submitted";
    if (window.productVulkanRenderingPath == "package_room_meshes" &&
        window.productVulkanRecordMode == "room_mesh_draws") {
      window.viewport.productVulkanRoomMeshBackendPresented = true;
    }
  } else {
    window.productVulkanStatus = "frame_not_submitted";
  }
}

Vec3 crossProduct(Vec3 lhs, Vec3 rhs) {
  return {lhs.y * rhs.z - lhs.z * rhs.y,
          lhs.z * rhs.x - lhs.x * rhs.z,
          lhs.x * rhs.y - lhs.y * rhs.x};
}

Vec3 normalizedOr(Vec3 value, Vec3 fallback) {
  const float len2 = lengthSquared(value);
  if (!std::isfinite(len2) || len2 <= 0.000001F) {
    return fallback;
  }
  return value / std::sqrt(len2);
}

Mat4 productPerspectiveMat4(float verticalFovRadians,
                            float aspect,
                            float nearPlane,
                            float farPlane) {
  const float f = 1.0F / std::tan(verticalFovRadians * 0.5F);
  Mat4 result{{{}}};
  result.m[0] = f / aspect;
  result.m[5] = -f;
  result.m[10] = farPlane / (nearPlane - farPlane);
  result.m[11] = -(farPlane * nearPlane) / (farPlane - nearPlane);
  result.m[14] = -1.0F;
  return result;
}

Mat4 productViewFromCamera(Vec3 eye, Vec3 forward, Vec3 up) {
  const Vec3 f = normalizedOr(forward, {0.0F, 0.0F, -1.0F});
  const Vec3 r = normalizedOr(crossProduct(f, up), {1.0F, 0.0F, 0.0F});
  const Vec3 u = crossProduct(r, f);
  Mat4 result = identityMat4();
  result.m[0] = r.x;
  result.m[1] = r.y;
  result.m[2] = r.z;
  result.m[3] = -dot(r, eye);
  result.m[4] = u.x;
  result.m[5] = u.y;
  result.m[6] = u.z;
  result.m[7] = -dot(u, eye);
  result.m[8] = -f.x;
  result.m[9] = -f.y;
  result.m[10] = -f.z;
  result.m[11] = dot(f, eye);
  return result;
}

FrameInput makeProductVulkanFrame(const SceneProjectionResult& scene,
                                  const DebugProjectionResult& debug,
                                  std::uint64_t frameIndex,
                                  std::uint32_t viewportWidth,
                                  std::uint32_t viewportHeight,
                                  float cameraYawDegrees,
                                  float cameraPitchDegrees) {
  constexpr float kPi = 3.14159265358979323846F;
  constexpr float kEyeHeightMeters = 1.7F;
  FrameInput frame;
  frame.viewport = {viewportWidth, viewportHeight,
                    static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight)};
  frame.clock = {scene.sourceTick, frameIndex, 0.0F, 1.0F / 60.0F};
  frame.camera.mode = RenderCameraMode::FirstPerson;
  Vec3 eye{0.0F, kEyeHeightMeters, 0.0F};
  for (const SceneItem& item : scene.items) {
    if (item.kind == SceneItemKind::Player || item.stableName == "player") {
      eye = item.transform.position + Vec3{0.0F, kEyeHeightMeters, 0.0F};
      break;
    }
  }
  const float yaw = cameraYawDegrees * kPi / 180.0F;
  const float pitch = cameraPitchDegrees * kPi / 180.0F;
  const float cosPitch = std::cos(pitch);
  frame.camera.worldEye = eye;
  frame.camera.worldForward = {std::sin(yaw) * cosPitch, std::sin(pitch),
                               -std::cos(yaw) * cosPitch};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  frame.camera.viewFromWorld =
      productViewFromCamera(frame.camera.worldEye, frame.camera.worldForward,
                            frame.camera.worldUp);
  frame.camera.clipFromView =
      productPerspectiveMat4(68.0F * kPi / 180.0F, frame.viewport.aspectRatio,
                             frame.camera.nearPlane, frame.camera.farPlane);
  frame.camera.clipFromWorld = frame.camera.clipFromView * frame.camera.viewFromWorld;
  frame.projections.scene = &scene;
  frame.projections.debug = &debug;
  return frame;
}

RendererConfig makeProductVulkanRendererConfig() {
  PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = PackageMode::BuildTreeProduct;
  lookupConfig.requireShaderRoot = true;
  lookupConfig.requireGraphicsRuntime = true;
  const PackageLookupResult lookup = resolvePackageRuntimeLookup(lookupConfig);

  RendererConfig config;
  config.renderer = RendererMode::Vulkan;
  config.rendererRequirement = RendererRequirement::Optional;
  config.allowSoftwareVulkan = true;
  if (lookup.outcome == RenderOutcome::Ok) {
    config.shaderRoot = lookup.lookup.shaderRoot;
    config.diagnosticsDir = lookup.lookup.diagnosticsDir;
  }
  return config;
}

std::string failedTapeStepReceiptValue(std::uint64_t stepIndex) {
  return stepIndex == 0U ? "none" : std::to_string(stepIndex);
}

void recordProductGameplayTapeParse(const ProductGameplayTapeParseResult& parsed,
                                    ProductAppWindowState& window) {
  window.gameplayTapeLoaded = parsed.ok;
  window.gameplayTapeStatus = parsed.status;
  window.gameplayTapeReasonCode = parsed.reasonCode;
  window.gameplayTapeLineCount = parsed.lineCount;
  window.gameplayTapeStepCount =
      static_cast<std::uint64_t>(parsed.tape.steps.size());
  window.gameplayTapeFailedStep = failedTapeStepReceiptValue(parsed.failedLine);
  window.gameplayTapeFailedSourceLine = parsed.failedLine;
  window.gameplayTapeFailedAction = "none";
  window.gameplayTapeFailedTarget = parsed.failedToken;
  window.gameplayTapeFailedRejection = "none";
}

void recordProductGameplayTapeRun(const ProductGameplayTapeRunResult& run,
                                  ProductAppWindowState& window) {
  window.gameplayTapeStatus = run.status;
  window.gameplayTapeReasonCode = run.reasonCode;
  window.gameplayTapeStepCount = run.stepCount;
  window.gameplayTapeExecutedStepCount = run.executedStepCount;
  window.gameplayTapeExpectedRejectedStepCount = run.expectedRejectedStepCount;
  window.gameplayTapeExpectedBlockedStepCount = run.expectedBlockedStepCount;
  window.gameplayTapeFailedStep = failedTapeStepReceiptValue(run.failedStepIndex);
  window.gameplayTapeFailedSourceLine = run.failedSourceLine;
  window.gameplayTapeFailedAction = run.failedAction;
  window.gameplayTapeFailedTarget = run.failedTarget;
  window.gameplayTapeFailedRejection = run.failedRejection;
  window.gameplayTapeFailedMovementBlock = run.failedMovementBlock;
  window.gameplayTapeLastAction = run.lastAction;
  window.gameplayTapeLastTarget = run.lastTarget;
  window.gameplayTapeLastMovementBlock = run.lastMovementBlock;
  window.gameplayTapeKeyCollected = run.keyCollected;
  window.gameplayTapeSecretDoorOpened = run.secretDoorOpened;
  window.gameplayTapeTreasureCollected = run.treasureCollected;
  window.gameplayTapeNpcTargetable = run.npcTargetable;
  window.gameplayTapeNpcDefeated = run.npcDefeated;
  window.gameplayTapeExitObjectiveComplete = run.exitObjectiveComplete;
  window.gameplayTapeLoopComplete = run.loopComplete;
  window.gameplayTapeAiCommandLogged = run.aiCommandLogged;
  window.gameplayTapeAiAttackLogged = run.aiAttackLogged;
  window.gameplayTapeAiWaitLogged = run.aiWaitLogged;
  window.gameplayTapeAiPlayerDamaged = run.aiPlayerDamaged;
  window.gameplayTapeAiPlayerHpBefore = run.aiPlayerHpBefore;
  window.gameplayTapeAiPlayerHpAfter = run.aiPlayerHpAfter;
  window.gameplayTapeAiActorId = run.aiActorId;
  window.gameplayTapeAiTargetId = run.aiTargetId;
  window.gameplayTapeAiBehavior = run.aiBehavior;
  window.gameplayTapeAiIntent = run.aiIntent;
  window.sessionOutcome = run.sessionOutcome;
  window.runtimeStateHash = run.runtimeStateHash;
  if (!run.ok) {
    window.status = "gameplay_tape_failed";
  }
}

void runProductGameplayTapeFromOptions(const ProductAppOptions& options,
                                       std::optional<Session>& activeSession,
                                       ProductAppWindowState& window) {
  if (options.gameplayTapePath.empty()) {
    return;
  }

  window.gameplayTapeRequested = true;
  window.gameplayTapePath = options.gameplayTapePath.generic_string();
  const ProductGameplayTapeParseResult parsed =
      loadProductGameplayTapeFile(options.gameplayTapePath);
  recordProductGameplayTapeParse(parsed, window);
  if (!parsed.ok) {
    window.status = "gameplay_tape_parse_failed";
    return;
  }

  const ProductGameplayTapeRunResult run = runProductGameplayTape(
      ProductGameplayTapeRunRequest{activeSession.has_value() ? &*activeSession : nullptr,
                                    &parsed.tape,
                                    productActiveRoomCollisionSurfaces(
                                        window.activeRoomCollision),
                                    &window.activeRoom,
                                    &window.activeRoomCollision});
  recordProductGameplayTapeRun(run, window);
}

bool routeAutomationInput(FrontendState& frontend,
                          const ProductSaveBridgeResult& saves,
                          const ProductAppOptions& options,
                          FrontendSettingsTab& settingsTab,
                          std::optional<Session>& activeSession,
                          WorldSetupDraft& worldSetupDraft,
                          ProductAppWindowState& window,
                          InputAction action,
                          bool& closeRequested) {
  ActionState actionState;
  ProductOpeningMenuInputContext menuContext{
      frontend, saves, options, settingsTab, activeSession, worldSetupDraft,
      window, closeRequested};
  routeProductOpeningMenuInput(action, actionState, menuContext);
  window.automationControlLastOwner = productInputOwnerFor(frontend, window);
  return window.lastInputAccepted || action == InputAction::None;
}

bool applyProductAutomationCommand(const ProductAutomationCommand& command,
                                   FrontendState& frontend,
                                   const ProductSaveBridgeResult& saves,
                                   const ProductAppOptions& options,
                                   FrontendSettingsTab& settingsTab,
                                   std::optional<Session>& activeSession,
                                   WorldSetupDraft& worldSetupDraft,
                                   ProductAppWindowState& window,
                                   bool& closeRequested) {
  ProductAutomationDispatchContext dispatchContext{
      frontend, saves, options, settingsTab,
      // branch-gate: BG-1016
      activeSession.has_value() ? &*activeSession : nullptr,
      worldSetupDraft, window,
      [&frontend, &window]() { return productInputOwnerFor(frontend, window); },
      [&frontend, &saves, &options, &settingsTab, &activeSession, &worldSetupDraft,
       &window, &closeRequested](InputAction action) {
        return routeAutomationInput(frontend, saves, options, settingsTab,
                                    activeSession, worldSetupDraft, window, action,
                                    closeRequested);
      },
      [&activeSession, &window, &frontend]() {
        const ProductAsciiRoomActivationResult activated =
            activateProductAsciiRoomPreview(activeSession, window);
        // branch-gate: BG-1016
        if (activated.ok) {
          enterProductGameplayTransition(frontend, window,
                                         FrontendAction::CreateAndEnter);
        }
        return activated.ok;
      },
      [&window](InputAction action) {
        InputRoutingContext routingContext;
        routingContext.owners.editor = window.roomEditing.ready;
        routingContext.owners.gameplay = true;
        return routeInputAction(routingContext, action);
      },
      [&frontend, &window, &activeSession]() {
        returnProductToTitleTransition(frontend, window);
        activeSession.reset();
      },
      [&closeRequested]() { closeRequested = true; },
  };
  return applyProductAutomationCommand(command, dispatchContext);
}

ProductAppWindowState runOpeningMenuWindow(const ProductAppOptions& options,
                                           const ProductWorldTemplate& world,
                                           FrontendState& frontend,
                                           std::optional<Session>& activeSession,
                                           WorldSetupDraft& worldSetupDraft,
                                           ProductAppWindowState window,
                                           const FrontendSettings& settings,
                                           const ProductSaveBridgeResult& saves) {
  window.requested = options.windowMode == ProductWindowMode::Window;
  const bool useVulkanRenderer = options.renderer == ProductRendererRequest::Vulkan;
  window.productVulkanRendererRequested = useVulkanRenderer;
  window.inputOwner = productInputOwnerFor(frontend, window);
  window.gameplayInputSuppressed =
      frontendBlocksGameplayInput(frontend) ||
      menuOwnerBlocksGameplay(window.inputOwner);
  if (!window.requested) {
    return window;
  }

#if defined(IGGY3D_HAS_SDL3)
  window.sdlAvailable = true;

  SdlWindowCreateInfo createInfo;
  createInfo.title = "iggy3d - Opening Menu";
  createInfo.width = 1280;
  createInfo.height = 720;
  createInfo.resizable = true;
  createInfo.highDpi = true;
  createInfo.vulkan = useVulkanRenderer;

  SdlWindow sdlWindow(createInfo);
  window.created = sdlWindow.nativeWindow() != nullptr;
  window.drawable = sdlWindow.isDrawable();
  window.openingMenuVisible = window.created && frontend.screen == FrontendScreen::Starter;
  if (!window.created) {
    window.status = "window_create_failed";
    return window;
  }

  SDL_Renderer* renderer = nullptr;
  RendererApi vulkanRenderer;
  if (useVulkanRenderer) {
#if defined(IGGY3D_APP_VULKAN_BACKEND)
    SdlVulkanSurfaceProvider sdlVulkanProvider;
    const SdlVulkanExtensionList extensions =
        sdlVulkanProvider.requiredInstanceExtensions(sdlWindow);
    if (extensions.outcome != RenderOutcome::Ok) {
      recordProductVulkanRendererUnavailable(window, extensions.reason.code);
      window.status = "product_vulkan_renderer_unavailable";
      return window;
    }
    VulkanBackendCreateInfo backendInfo;
    backendInfo.config = makeProductVulkanRendererConfig();
    const SdlDrawableExtent drawableExtent = sdlWindow.drawableExtent();
    backendInfo.drawableWidth =
        drawableExtent.width == 0U ? createInfo.width : drawableExtent.width;
    backendInfo.drawableHeight =
        drawableExtent.height == 0U ? createInfo.height : drawableExtent.height;
    backendInfo.surfaceProvider.requiredInstanceExtensions = extensions.names;
    backendInfo.surfaceProvider.createSurface =
        [&sdlVulkanProvider, &sdlWindow](VkInstance instance, VkSurfaceKHR* surface) {
          const SdlVulkanSurfaceCreateResult created =
              sdlVulkanProvider.createSurface(sdlWindow, instance);
          if (surface != nullptr) {
            *surface = created.surface;
          }
          RenderReceipt receipt;
          appendReceiptField(receipt, "surface_provider", "sdl3");
          appendReceiptField(receipt, "surface_created",
                             created.outcome == RenderOutcome::Ok &&
                                 created.surface != VK_NULL_HANDLE);
          appendReceiptField(receipt, "result",
                             created.outcome == RenderOutcome::Ok ? "pass" : "fail");
          appendReceiptField(receipt, "reason_code", created.reason.code);
          return receipt;
        };
    vulkanRenderer =
        RendererApi(std::make_unique<VulkanBackend>(std::move(backendInfo)));
    recordProductVulkanRendererReady(window, vulkanRenderer);
    if (!window.productVulkanRendererReady) {
      window.status = "product_vulkan_renderer_unavailable";
      return window;
    }
#else
    recordProductVulkanRendererUnavailable(window, "product_vulkan_backend_unavailable");
    window.status = "product_vulkan_renderer_unavailable";
    return window;
#endif
  } else {
    renderer = SDL_CreateRenderer(sdlWindow.nativeWindow(), nullptr);
    if (renderer == nullptr) {
      window.status = "renderer_create_failed";
      return window;
    }
  }

  sdlWindow.setTitle(window.gameplayActive ? "iggy3d - Gameplay" : "iggy3d - Opening Menu");
  const auto start = std::chrono::steady_clock::now();
  KeyboardInputState keyboard;
  MouseInputState mouse;
  GamepadMenuState gamepad;
  initializeGamepadMenuState(gamepad);
  window.gamepadAvailable = gamepad.gamepadAvailable;
  window.gamepadName = gamepad.gamepadName;
  window.gamepadMapping = gamepad.gamepadAvailable ? "sdl_gamepad" : "unavailable";
  bool closeRequested = false;
  FrontendSettingsTab settingsTab = FrontendSettingsTab::Input;
  while (sdlWindow.isOpen()) {
    ActionState actionState;
    sdlWindow.pollEvents();
    ++window.eventPollCount;
    window.drawable = sdlWindow.isDrawable();
    sdlWindow.setTitle(window.gameplayActive ? "iggy3d - Gameplay" : "iggy3d - Opening Menu");

    ProductOpeningMenuInputContext menuContext{
        frontend, saves, options, settingsTab, activeSession, worldSetupDraft,
        window, closeRequested};
    routeProductOpeningMenuInput(pollKeyboardMenuAction(keyboard), actionState,
                                 menuContext);
    if (frontend.childScreen == FrontendScreen::NewWorld) {
      const char paintGlyph = pollKeyboardAsciiRoomPaintGlyph(keyboard);
      if (paintGlyph != '\0') {
        applyDungeonDraftPaintGlyph(worldSetupDraft, window, paintGlyph);
      }
    }

    const InputAction gamepadAction = pollGamepadMenuAction(gamepad);
    if (gamepadAction != InputAction::None) {
      window.gamepadMenuSelectUsed = true;
      routeProductOpeningMenuInput(gamepadAction, actionState, menuContext);
    }

    const MouseClick click = pollMouseClick(mouse);
    if (click.clicked) {
      const OpeningMenuHitTestResult hit = openingMenuActionAt(frontend, click.x, click.y);
      if (hit.hit) {
        window.mouseMenuSelectUsed = true;
        if (hit.area == OpeningMenuHitArea::StarterAction) {
          frontend.selectedAction = hit.action;
          routeProductOpeningMenuInput(mouseClickAction(click), actionState,
                                       menuContext);
        } else if (hit.area == OpeningMenuHitArea::DevToolsCategory) {
          frontend.devToolsCategory = hit.devToolsCategory;
          frontend.status = "dev_tools_category_selected";
        } else if (hit.area == OpeningMenuHitArea::SettingsTab) {
          settingsTab = hit.settingsTab;
          frontend.status = "settings_tab_selected";
        }
      }
    }

    if (window.gameplayActive && activeSession.has_value() &&
        !frontendBlocksGameplayInput(frontend)) {
      ActionState gameplayActions;
      if (window.roomEditing.ready) {
        pollKeyboardRoomEditorActions(keyboard, gameplayActions);
        pollGamepadRoomEditorActions(gamepad, gameplayActions);
      } else {
        pollKeyboardGameplayActions(keyboard, gameplayActions);
        pollGamepadGameplayActions(gamepad, gameplayActions);
        pollMouseGameplayActions(mouse, gameplayActions);
      }

      ActionState acceptedGameplayActions;
      ActionState acceptedEditorActions;
      InputRoutingContext routingContext;
      routingContext.owners.editor = window.roomEditing.ready;
      routingContext.owners.gameplay = true;
      for (const ActionStateEntry& entry : gameplayActions.entries) {
        const InputRoutingResult routed = routeInputAction(routingContext, entry.action);
        window.inputOwner = routed.owner;
        window.lastInputAction = routed.action;
        window.lastInputAccepted = routed.accepted;
        window.gameplayInputSuppressed = routed.gameplaySuppressed;
        if (routed.accepted && routed.owner == MenuOwner::Editor &&
            inputActionGroup(entry.action) == InputActionGroup::Editor) {
          recordAction(acceptedEditorActions, entry.action, entry.down, entry.pressed,
                       entry.released, entry.value);
        } else if (routed.accepted && routed.owner == MenuOwner::Gameplay) {
          recordAction(acceptedGameplayActions, entry.action, entry.down, entry.pressed,
                       entry.released, entry.value);
        }
      }
      if (!acceptedEditorActions.entries.empty()) {
        const ProductRoomEditorActionResult result =
            applyProductRoomEditorActions(window.roomEditing,
                                          window.roomEditorCursor,
                                          acceptedEditorActions,
                                          ProductRoomAuthoringInputSource::Hotkey);
        recordProductRoomEditorActionResult(window, result);
      } else {
        applyProductCameraActions(acceptedGameplayActions, window.viewport, settings,
                                  "action_map");
        const SpatialSurfaceSet* collisionSurfaces =
            productActiveRoomCollisionSurfaces(window.activeRoomCollision);
        applyProductGameplayActions(*activeSession, acceptedGameplayActions, window,
                                    "action_map", collisionSurfaces);
      }
    }

    SceneProjectionResult scene;
    DebugProjectionResult debug;
    ProductPrimitiveDrawList drawList;
    ProductViewportFrame frame;
    std::size_t sceneItemCount = 0;
    const SceneProjectionResult* scenePtr = nullptr;
    const DebugProjectionResult* debugPtr = nullptr;
    const ProductPrimitiveDrawList* drawListPtr = nullptr;
    const ProductViewportFrame* framePtr = nullptr;
    const ProductRenderBridgeFrame* bridgePtr = nullptr;
    ProductGameplayFeedback feedback = buildProductGameplayFeedback(window);
    ProductMovementDebugHud movementHud = buildProductMovementDebugHud(
        window, settings.devToolsEnabled, settings.debugOverlayEnabled);
    ProductNpcBehaviorDebugHud npcBehaviorHud = buildProductNpcBehaviorDebugHud(
        nullptr, window.gameplayActive, settings.devToolsEnabled,
        settings.debugOverlayEnabled);
    copyNpcBehaviorDebugHud(window, npcBehaviorHud);
    ProductRoomEditorOverlay roomEditorOverlay =
        buildProductRoomEditorOverlay(window.roomEditorCursor, false);
    copyProductRoomEditorOverlay(window, roomEditorOverlay);
    ProductRenderBridgeFrame bridge;
    if (window.gameplayActive && activeSession.has_value()) {
      const RoomAsset* activeRoom =
          window.activeRoom.loaded ? &window.activeRoom.room : nullptr;
      scene = buildSceneProjection(activeSession->state(), activeRoom);
      debug = buildProductDebugProjectionWithNpcBehavior(activeSession->state());
      roomEditorOverlay = buildProductRoomEditorOverlay(window.roomEditorCursor,
                                                        window.roomEditing.ready);
      copyProductRoomEditorOverlay(window, roomEditorOverlay);
      drawList = buildProductPrimitiveDrawList(&scene, &debug, activeRoom,
                                               &window.activeRoomCollision,
                                               &roomEditorOverlay);
      frame = buildProductViewportFrame(
          drawList, ProductViewportFrameConfig{window.viewport.cameraYawDegrees,
                                               window.viewport.cameraPitchDegrees});
      scenePtr = &scene;
      debugPtr = &debug;
      drawListPtr = &drawList;
      framePtr = &frame;
      sceneItemCount = scene.items.size();
      window.runtimeStateHash = activeSession->stateHash();
      feedback = buildProductGameplayFeedback(window);
      movementHud = buildProductMovementDebugHud(window, settings.devToolsEnabled,
                                                 settings.debugOverlayEnabled);
      npcBehaviorHud = buildProductNpcBehaviorDebugHud(&debug,
                                                       window.gameplayActive,
                                                       settings.devToolsEnabled,
                                                       settings.debugOverlayEnabled);
      copyNpcBehaviorDebugHud(window, npcBehaviorHud);
      bridge = buildProductRenderBridgeFrame(&drawList, &frame, &feedback);
      bridgePtr = &bridge;
    }

    if (window.drawable) {
      applyGameplayProjectionMetrics(window, scenePtr, debugPtr, drawListPtr, framePtr,
                                     bridgePtr,
                                     window.gameplayActive && scenePtr != nullptr);
      if (useVulkanRenderer) {
        if (scenePtr != nullptr && debugPtr != nullptr && scenePtr->room.loaded) {
          const SdlDrawableExtent drawableExtent = sdlWindow.drawableExtent();
          if (drawableExtent.width > 0U && drawableExtent.height > 0U) {
            const FrameInput renderFrame = makeProductVulkanFrame(
                *scenePtr, *debugPtr, window.framesPresented + 1U, drawableExtent.width,
                drawableExtent.height, window.viewport.cameraYawDegrees,
                window.viewport.cameraPitchDegrees);
            const RenderSubmitResult submit = vulkanRenderer.submitFrame(renderFrame);
            recordProductVulkanSubmit(window, submit);
          } else {
            window.productVulkanStatus = "frame_not_submitted";
            window.productVulkanReasonCode = "frame_not_drawable";
          }
        } else {
          window.productVulkanStatus = "waiting_for_gameplay_room";
          window.productVulkanReasonCode = "product_vulkan_waiting_for_gameplay_room";
        }
      } else {
        const OpeningMenuViewState view =
            drawOpeningMenuView(*renderer, options, world, frontend, settingsTab,
                                worldSetupDraft,
                                window.worldSetupDungeonDraftEditMode,
                                window.worldSetupDungeonDraftModified,
                                window.worldSetupDungeonDraftCursorRow,
                                window.worldSetupDungeonDraftCursorColumn,
                                window.gameplayActive, window.runtimeStateHash, framePtr,
                                &feedback, &movementHud, &npcBehaviorHud,
                                sceneItemCount, debugPtr,
                                window.viewport.cameraYawDegrees,
                                window.viewport.cameraPitchDegrees, saves);
        window.viewport.cameraHeadingVisible =
            window.viewport.cameraHeadingVisible || view.cameraHeadingDrawn;
        window.menuTextDrawn = window.menuTextDrawn || view.textDrawn;
        window.selectedRowDrawn = window.selectedRowDrawn || view.selectedRowDrawn;
        window.menuRowCount = view.rowCount;
      }
    } else {
      applyGameplayProjectionMetrics(window, scenePtr, debugPtr, drawListPtr, framePtr,
                                     bridgePtr, false);
    }
    ++window.framesPresented;

    if (options.frames > 0 && window.framesPresented >= options.frames) {
      break;
    }
    if (closeRequested) {
      break;
    }
    if (options.holdSeconds > 0) {
      const auto elapsed = std::chrono::steady_clock::now() - start;
      if (elapsed >= std::chrono::seconds(options.holdSeconds)) {
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  shutdownGamepadMenuState(gamepad);
  if (useVulkanRenderer) {
    (void)vulkanRenderer.waitIdle();
    vulkanRenderer.shutdown();
  } else {
    SDL_DestroyRenderer(renderer);
  }
  window.selectedSettingsTab = settingsTab;
  if (useVulkanRenderer) {
    window.status = window.productVulkanFrameSubmitted
                        ? "product_vulkan_frame_presented"
                        : window.productVulkanStatus;
  } else {
    window.status = window.menuTextDrawn ? "opening_menu_text_ready"
                                         : "opening_menu_window_ready";
  }
  return window;
#else
  (void)world;
  (void)frontend;
  (void)worldSetupDraft;
  (void)saves;
  window.sdlAvailable = false;
  window.created = false;
  window.drawable = false;
  window.openingMenuVisible = false;
  window.status = "sdl3_unavailable";
  return window;
#endif
}

}  // namespace

int runProductApp(int argc, char** argv) {
  const ProductAppOptionsParseResult parsed = parseProductAppOptions(argc, argv);
  if (parsed.status == ProductAppOptionStatus::Help) {
    std::cout << productAppHelpText();
    return 0;
  }
  if (parsed.status != ProductAppOptionStatus::Ok) {
    RenderReceipt receipt;
    appendReceiptField(receipt, "app", "iggy3d");
    appendReceiptField(receipt, "result", "fail");
    appendReceiptField(receipt, "reason_code", productAppOptionStatusReason(parsed.status));
    appendReceiptField(receipt, "option", parsed.option);
    std::cout << formatRenderReceipt(receipt);
    return 2;
  }

  const ProductAppOptions& options = parsed.options;
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  ProductSaveBridgeResult saves =
      scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  const FrontendSettings settings = productFrontendSettingsFromOptions(options);
  std::optional<Session> activeSession;
  // branch-gate: BG-1026
  WorldSetupDraft worldSetupDraft = options.devPackageOverride.empty()
                                        ? makeProductDefaultWorldSetupDraft()
                                        : makeDefaultWorldSetupDraft();
  ProductAppWindowState window;
  recordWorldSetupDraftState(worldSetupDraft, window);

  FrontendState frontend;
  initializeProductStarterTransition(frontend, window, saves.slots.compatibleCount > 0);

  if (options.autoNewWorld) {
    launchProductNewWorld(options, worldSetupDraft, frontend, activeSession, window);
  }
  if (options.scriptedGameplaySmoke) {
    runScriptedProductGameplaySmoke(activeSession, window);
    ActionState scriptedLook;
    recordAction(scriptedLook, InputAction::PlayerLookX, true, false, false, 1.0F);
    recordAction(scriptedLook, InputAction::PlayerLookY, true, false, false, 0.5F);
    applyProductCameraActions(scriptedLook, window.viewport, settings, "scripted");
  }

  FrontendSettingsTab automationSettingsTab = FrontendSettingsTab::None;
  bool automationCloseRequested = false;
  ProductAutomationControlContext automationControlContext{
      options.automationControlPath, window, automationSettingsTab,
      [&frontend, &saves, &options, &automationSettingsTab, &activeSession,
       &worldSetupDraft, &window, &automationCloseRequested](
          const ProductAutomationCommand& command) {
        return applyProductAutomationCommand(command, frontend, saves, options,
                                             automationSettingsTab, activeSession,
                                             worldSetupDraft, window,
                                             automationCloseRequested);
      },
      [&frontend, &window]() { return productInputOwnerFor(frontend, window); },
      [&frontend](MenuOwner owner) {
        return frontendBlocksGameplayInput(frontend) || menuOwnerBlocksGameplay(owner);
      },
  };
  applyProductAutomationControl(automationControlContext);
  if (!automationCloseRequested) {
    runProductGameplayTapeFromOptions(options, activeSession, window);
  }
  saves = scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  if (automationCloseRequested) {
    window.status = "automation_close_requested";
  }

  window =
      runOpeningMenuWindow(options, world, frontend, activeSession, worldSetupDraft,
                           window, settings, saves);
  refreshProductGameplayProjectionMetrics(
      ProductGameplayProjectionRefreshRequest{activeSession, window,
                                              settings.devToolsEnabled,
                                              settings.debugOverlayEnabled});

  if (options.printRenderReceipt) {
    std::cout << formatRenderReceipt(
        buildProductAppReceipt(options, world, frontend, settings, window, saves));
  }

  return window.requested && !window.created ? 77 : 0;
}

}  // namespace iggy3d
