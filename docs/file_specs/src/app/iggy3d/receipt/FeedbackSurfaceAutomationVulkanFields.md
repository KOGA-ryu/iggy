# File Spec

Files: `src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.cpp`

Verified at: `0ba40cb9`

## Owns

- Receipt field emission for product gameplay feedback, active/input surface ownership, last input action, automation-control state, product Vulkan renderer readiness, and Vulkan menu UI proof.
- Table-driven mapping from feedback, active surface, automation, and Vulkan readiness packets to receipt keys.

## Does Not Own

- Gameplay feedback construction.
- Active surface resolution.
- Automation command execution.
- Vulkan renderer lifecycle or menu drawing.
- Mouse capture policy decisions.

## Reads

- `ProductAppWindowState`, `GameplayFeedback`, `ProductActiveSurfaceFrame`, and `ProductVulkanGameplayReadiness`.

## Writes / Mutates

- Appends fields to `RenderReceipt`.
- Does not mutate feedback, active surface, automation, input, or Vulkan state.

## Calls Out To / Wires Out To

- `appendReceiptField(...)`.
- Surface/input/mouse capture/menu owner/input action name helpers.

## Called By / Entry Points

- `buildProductAppReceipt(...)` calls `appendProductFeedbackSurfaceAutomationVulkanFields(...)`.
- Focused proof: `rg -n "appendProductFeedbackSurfaceAutomationVulkanFields|product_feedback_visible|active_surface|product_vulkan_renderer_ready" src/app tests`.

## Invariants

- Active surface fields are supplied by the receipt builder and must not be recomputed differently here.
- Feedback and automation fields are observational proof only.
- Vulkan readiness fields must not initialize or touch the renderer.
- Input owner and mouse capture policy fields remain separate facts.

## Tests / Proof Commands

- `rg -n "product_feedback_visible|active_surface|product_vulkan_renderer_ready|automation_control_status" tests/unit tests/smoke src/app/iggy3d/receipt`.
- `rg -n "product_menu_transitions_tests|product_vulkan_room_frame_tests|product_ascii_package_smoke" cmake/iggy3d_tests.cmake tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/menu/FrontendRouter.*` unless active surface packets change.
- `src/app/iggy3d/automation/*` unless automation proof fields change.
- `src/app/iggy3d/window/RendererLifecycle.*` unless Vulkan readiness fields change.

## Update When

- Feedback, active surface, automation, Vulkan readiness, or Vulkan menu receipt fields change.

## Do Not Update When

- Only gameplay feedback logic, automation execution internals, or Vulkan backend behavior changes without changing emitted receipt fields.
