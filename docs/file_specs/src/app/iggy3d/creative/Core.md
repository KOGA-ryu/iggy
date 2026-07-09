# File Spec

File: `src/app/iggy3d/creative/Core.hpp`

Verified at: `cd6c9663`

## Owns

- Minimal creative core ids, refs, flags, and packet primitives.
- `Tool` enum used across creative tool/UI/input paths.
- `FrameRef`, `TargetRef`, `Flags`, `FramePacket`, and generic `Packet`.
- The shared invalid id sentinel for creative target refs.

## Does Not Own

- Creative document object ids.
- Tool input dispatch state.
- UI command catalog rows.
- Product window input routing.
- Save/load identity.

## Reads

- No runtime data; this is type and constant definition only.

## Writes / Mutates

- No mutation.

## Calls Out To / Wires Out To

- Included by facade, tool, UI, spatial, and bridge files as the lowest-level creative app packet vocabulary.
- `Tool` routes through input bridge, command catalog, facade, UI model, and draw list.
- `TargetRef` is the app-facing selection/pick target value used outside document object storage.

## Called By / Entry Points

- Types/constants are consumed by many creative headers.
- Grep proof: `rg -n "creative::Tool|cr::Tool|TargetRef|kInvalidId|FramePacket|PacketKind" src/app/iggy3d/creative tests/unit/creative_core_tests.cpp tests/unit/creative_tools_tests.cpp tests/unit/creative_facade_tests.cpp --glob '*.{hpp,cpp}'`.

## Invariants

- `kInvalidId` stays the invalid target sentinel for `TargetRef`.
- Default `Tool` consumers assume `Select` is the neutral/default tool.
- Core types must not include product window, renderer, save, or runtime dependencies.
- Keep this header low-level; do not add feature-specific behavior here.

## Tests / Proof Commands

- `creative_core_tests`.
- `creative_tools_tests`.
- `creative_facade_tests`.
- `rg -n "creative_core_tests|creative_tools_tests|creative_facade_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/State.hpp` unless legacy state mirrors change.
- `src/app/iggy3d/creative/tools/Tools.*` unless tool enum semantics change.
- `src/app/iggy3d/creative/ui/Ui.*` unless target refs or tool rows change.
- `src/app/iggy3d/creative/Facade.*` unless frame packet consumption changes.

## Update When

- Core ids, target refs, tool enum, flags, packet fields, or invalid-id contract changes.

## Do Not Update When

- Only higher-level tool, document, UI, or product window behavior changes while core packet vocabulary stays stable.
