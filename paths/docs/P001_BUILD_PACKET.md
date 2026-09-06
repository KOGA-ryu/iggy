# P001 — standalone Paths with its title and existing game modes

Status: authorized by the user's Paths migration request. This supersedes
further FM002 work in the parent engine. Work only in
`/Users/kogaryu/iggy3d/paths/`, keep changes uncommitted, and send a final brief
to planner task `01a074ae-914f-7b23-bbc4-abaf7d14d533` when finished.

## Outcome and supplied seed

Deliver an independently buildable native `paths` executable that opens a
title screen and lets the player choose Guided Questions or Quick Hunt,
resume each session, and read session stats. Preserve the existing game
evidence. Apply the known Guided teaching/input/layout repairs to the copied
files. The source-card additions and configurable experiment rules follow in
separate capability packets.

The planner already copied the bounded source set. See
`MIGRATION_SEED_MANIFEST.json` for exact source paths and byte hashes. It
contains the four model files, four UI files, `app/main.cpp`, three tests,
seven allocator/capture/receipt helper files, the pinned ImGui subset, and
design/content references. The copied main is a **seed**, still using the old
engine host; it must be adapted before the project is independent.

Do not link `iggy3d`, configure the parent, copy its Creative source tree, or
fix parent engine files. The portable model include closure is standard C++
plus its own headers. The existing UI needs only those models, SDL events,
and ImGui. The seven helper files close over one another plus standard C++
and Vulkan; no engine world or scene model is needed.

## 1. Build graph

Create a standalone CMake root, C++20, with these exact product targets:

- `paths_model`: `HuntSession.cpp`, `LayeredQuestionSession.cpp`; include `src/`.
- `paths_imgui`: the four core ImGui `.cpp` files and the two SDL3/Vulkan
  backend `.cpp` files. Use SYSTEM includes and preserve the local license/pin.
- `paths_ui`: the two existing UI `.cpp` files; public dependency on
  `paths_model`, private dependencies on ImGui and SDL3. Its public include
  locations are this project's `src/` and `src/ui/`.
- `paths_native`: new `src/platform/NativeVulkanHost.hpp/.cpp`, plus the copied
  `RenderDiagnostics.cpp`, `VulkanMemoryAllocator.cpp`, and `FrameCapture.cpp`.
  Include `vendor/iggy3d/src/` privately and define `IGGY3D_HAS_VULKAN=1` for
  this borrowed code/host target. Link Vulkan, SDL3, and ImGui privately.
- `paths`: adapted `app/main.cpp`, composed from the model/UI/native targets.

Discover `SDL3` with CONFIG and Vulkan with `find_package(Vulkan REQUIRED)`.
The inspected machine resolves SDL3 at `/opt/homebrew/lib/cmake/SDL3` and
Vulkan headers/library under `/opt/homebrew`; treat these as current discovery
evidence, not paths to hardcode. Support normal CMake prefix/SDK overrides.
Derive RPATH from imported libraries when required. No source code or shader
input comes from a parent build directory. ImGui's embedded default shaders
are enough for this native UI pass; no GLSL compiler is needed by P001.

Register `paths_hunt_tests`, `paths_guided_tests`, and `paths_input_tests` from
the copied tests. Model tests link only `paths_model`. Input tests explicitly
link local ImGui/SDL alongside `paths_ui`. All Paths checks live in this CMake
project and do not join the parent CTest registry.

## 2. Native host: one rendering route

Own the SDL/Vulkan/ImGui lifecycle in `NativeVulkanHost`. Its public interface
uses a launch config, drawable extent, event callback, a frame result/error,
and capture paths; public headers must not expose ImGui objects. App/UI code
must not allocate Vulkan resources. Keep the already working semantic script
runner and JSON writer from main; replace its PackageRuntimeLookup,
SceneProjection, FrameInput, and VulkanBackend use with this host.

Use one owned RGBA8 color target for both offscreen and interactive rendering.
ImGui draws into that image. A screenshot reads that same image; interactive
mode additionally transfers it to the acquired swapchain image. This keeps
the UI render/capture path common across test and normal play.

Concrete host sequence:

1. Create an instance and a graphics-capable device. On macOS enable
   portability enumeration/its instance flag and portability subset when
   advertised. In interactive mode, obtain the SDL-required instance
   extensions and verify the chosen graphics queue also supports presentation
   to the SDL surface. In offscreen mode create no native window/surface and
   require no headless-surface extension. Return an explicit unsupported
   diagnostic if the required device/queue/capabilities are unavailable.
2. Create a render pass and framebuffer for an owned
   `VK_FORMAT_R8G8B8A8_UNORM` image, usage COLOR_ATTACHMENT | TRANSFER_SRC.
   Use clear/load, store, and transition to TRANSFER_SRC for readback/present.
   The copied allocator can create the image and capture readback buffer.
3. Initialize ImGui with the pinned 1.92.8 API. Use
   `ImGui_ImplVulkan_InitInfo::PipelineInfoMain.RenderPass`,
   `UseDynamicRendering=false`, and valid MinImageCount/ImageCount >= 2.
   Do not copy an older backend signature from memory. The current definitions
   are in `third_party/imgui/backends/imgui_impl_vulkan.h`.
4. Interactive mode initializes ImGui's SDL3 backend and feeds every SDL event
   to it as well as the application's action adapter. Offscreen mode sets
   DisplaySize, framebuffer scale, and a deterministic positive DeltaTime
   directly and does not call SDL backend functions that need a window.
5. The host owns exactly one ImGui NewFrame/Render pair per frame. Remove the
   old trailing `ImGui::Render()` in the copied UI frame function. The host
   calls UI drawing between those two operations and then records its draw
   data. It also owns shutdown; no second context or backend initialization.
6. Start with one GPU frame in flight and explicit fence completion before
   reusing the target/command resources. Correctness and deterministic
   capture matter more than extra pipelining in this UI application.
7. For interactive presentation, the pinned ImGui backend's window helpers
   may own swapchain/image resources. Their exact create signature includes
   the final `VkImageUsageFlags image_usage` argument. Request TRANSFER_DST
   usage, validate surface support, and choose a supported format. Transfer
   the RGBA target with the necessary conversion/blit when the swapchain is
   BGRA; do not raw-copy unlike channel orders. Transition the acquired image
   to transfer destination and then PRESENT. Respect acquire/submit/present
   semaphores, suboptimal/out-of-date results, and nonzero drawable extent.
8. Resize only after outstanding work completes; recreate target and capture
   resources. Minimized/zero-size windows skip drawing without losing state.
   On device/surface loss, return a clear failure; do not claim a capture or
   keep a half-initialized host. Release resources in reverse ownership order.
9. For capture, copy the completed target to the borrowed `FrameCapture`
   buffer with transfer-to-host synchronization, wait for completion, read
   normalized pixels, and write PNG/raw/meta/RGB-SHA artifacts with the copied
   helper. Record `product=paths` and whether UI draw data was recorded in the
   host's own receipt. Keep helper-origin diagnostics attributed to iggy3d.

Do not claim the swapchain route tested from an offscreen run. It must compile
and be implemented; a visible interactive test remains a separately reported
manual check because no visible launch was requested.

## 3. Title screen and shared action route

Extend the existing copied app dispatcher and `FirstMoveUiState` with
`PathsScreen {Title, Playing, Stats}`, title focus, and a quit-request flag.
Keep the existing active mode as its one authority. No new dispatcher wrapper
or duplicate selected-mode variable is needed. Register title actions in the
same action enum/table used by pointer, keyboard, and scripts.

Mode descriptors are a two-entry table using the existing mode enum:

| ID | Title | Description |
| --- | --- | --- |
| guided | Guided Questions | Name the symbols. Choose the steps. See why. |
| hunt | Quick Hunt | Spot the rule. Select the matches. Bank correct rows. |

Title layout: `PATHS`, then `One problem. More than one way through.`, two
large mode cards, `SESSION STATS`, text-size controls, and `QUIT`. Use one
column below the width needed for two readable cards. Keep existing palette
and local fonts for this slice. Draw path/line accents directly if useful;
do not require a new generated image or asset library.

Title keyboard: arrows select a mode, Enter opens it; G/H open their named
mode; V opens Session Stats; Q quits explicitly. Every card is a full hit
target. Playing has a persistent `MODES [F1]` action that returns to Title
without resetting a session. Escape preserves each mode's existing local
meaning. Stats has `BACK TO MODES [ESC]` and reads current + archived run
records without changing them. Preserve an open Hunt review and its cross-mode
guard even if the player visits Title or Stats in between.

Stats content has separate sections, no combined accuracy:

- Guided: first-try correct layers, corrected-after-retry layers, shown
  answers, incorrect checks, completed questions, and current step/total.
- Hunt: initially correct/incorrect rows, explained rows, current points,
  finished packs. Show denominators where presenting a rate.

Stats are for this process session. Do not imply durable study history.
P001 offers the carried fixed rules; the objective/scoring experimentation
architecture follows in P002. No fake future-mode buttons or mutable score
settings are needed to complete this migration.

## 4. Carry the reviewed learning fixes

Apply `docs/first-move/FM002_ARCHITECTURE.md` sections 2–6 to the Paths copies:
neutral wrong feedback, precise step-4 goal, neutral card subtitle, authored
working equations, text that actually scales, pinned context/footer, measured
option height, reading keys, accepted-action focus updates, and stale queued
input rejection. See `FM002_R1_BUILDER_BRIEF.md` for the exact evidence behind
those repairs. Its instruction to edit parent files is superseded here.

The inherited first-try/retry/reveal report evidence was correct. Preserve it;
do not replace the models or their records with UI-owned scoring. The eight
reference cases remain useful model tests. Make Paths' header identify the
product while keeping Guided/Quick Hunt recognizable mode names.

## 5. CLI and reports

Keep existing script commands and one command per frame. A normal unscripted
launch defaults to Title. Accept `--start-mode title|guided|hunt`; an explicit
choice wins. For existing scripts with no explicit mode, retain the legacy
Hunt default. Add `open_mode guided|hunt`, `back_title`, `open_stats`, and
`close_stats` through the shared dispatcher. Validate syntax before any native
resources are created. Unavailable but well-formed actions remain reported
runtime rejections.

Preserve FM002 report fields and their indexing: option indices zero-based,
step numbers one-based. Add `product: "paths"`, report schema version 1,
start/final screen, and mode ID. Reports serialize the actual models; there
is no independently reconstructed success state in main. Title/stats actions
cannot create attempts or restart a run. Error output and `--help` identify
the `paths` executable and the actual supported flags.

The source-card scripts in `content/reference_cases/*.planned.script` are
future specifications. P001 does not add their `guided_open_question` command
or claim those cards are playable.

## 6. Proof and finished brief

Required checks are bounded to this project:

1. Configure/build the independent targets from Paths' CMake root. Run the
   three copied/adapted model/input test targets. Add final observable cases
   for Title → each mode → Title → resume and review-guard preservation;
   reading Stats must leave the records equal.
2. Native offscreen captures of Title, Guided recovery, and explicit Hunt,
   with nonuniform pixels and UI-recorded receipts. Check 1440×900 / 100% and
   1024×768 / 150% across the title and Guided evidence. Visually inspect
   captures for actual readable option/explanation text, not only bounding
   rectangles. Include a Guided mechanics step showing current work.
3. Replay the carried all-correct/retry/reveal examples and verify the same
   mathematical/evidence results. The parent baseline was already tested;
   these runs establish the migrated route, not a repeated parent audit.
4. Copy `paths/` alone, excluding build/out/Git metadata, to a fresh temporary
   location outside iggy3d. Configure, build, and run a bounded offscreen title
   capture there. Inspect its compile inputs/dependency files for parent
   source/build references. A system SDK path is legitimate; a hidden parent
   source dependency fails isolation. Do not delete or rename the parent repo
   to perform this check.
5. Run `python3 content/verify_planning_artifacts.py` once from Paths to verify
   the moved content artifacts still resolve their own snapshots. Do not run
   the mutable source pipeline or import the new cards into runtime yet.

Use only the bounded GPU permissions needed for Metal/MoltenVK captures; do
not run broad parent CTest or launch a visible window. If a GPU/device boundary
is unavailable, report it explicitly and finish all independent build/model
work rather than substituting a fake screenshot.

Update Paths' README and WORKSTREAMS with verified commands and artifact
paths. Keep `MIGRATION_SEED_MANIFEST.json` as the source-origin record; local
edits after the seed are expected. Report implementation-owned production
files/LOC separately from third-party and borrowed seed volume. Stop after
P001 and push the completion brief; the planner will not poll your progress.
