# File Spec

Files: `src/app/iggy3d/ascii_room/Preview.hpp`, `src/app/iggy3d/ascii_room/Preview.cpp`

Verified at: `b49a78ae`

## Owns

- Product ASCII room preview request construction from draft window state.
- Automation text escape decoding for ASCII room source text.
- Recording preview authoring status, counts, and asset-text proof into `window.creativeAuthoring.asciiRoomPreview`.

## Does Not Own

- Source parsing, authoring pipeline internals, active-room rebuild, gameplay/session activation, automation command registry, or receipt field emission.

## Reads

- `window.creativeAuthoring.asciiRoomDraft`.
- `ProductAsciiRoomAuthoringResult` fields returned by authoring.
- Escaped automation string values for source text updates.

## Writes / Mutates

- Writes `window.creativeAuthoring.asciiRoomPreview` fields.
- `buildProductAsciiRoomPreviewResult(...)` and `buildProductAsciiRoomPreview(...)` build authoring results from the current draft and record them.

## Calls Out To / Wires Out To

- `buildProductAsciiRoomAuthoring(...)`.
- Called by automation, world launch, and activation flows before active room/session changes.

## Called By / Entry Points

- `decodeProductAsciiRoomAutomationText(...)`.
- `productAsciiRoomAuthoringRequestFromDraft(...)`.
- `recordProductAsciiRoomPreview(...)`.
- `buildProductAsciiRoomPreviewResult(...)`.
- `buildProductAsciiRoomPreview(...)`.
- Grep proof: `rg -n "buildProductAsciiRoomPreview|recordProductAsciiRoomPreview|decodeProductAsciiRoomAutomationText|asciiRoomPreview|asciiRoomDraft" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Draft room id/source name pass through to the authoring request.
- Empty recorded source/room ids become `"none"` in preview state.
- Preview `ready` mirrors authoring result success.
- Asset text proof is derived from the asset text result and generated text byte size.
- Escape decoding handles newline, tab, and backslash, while preserving unknown escapes with a leading backslash.

## Tests / Proof Commands

- `rg -n "product_ascii_room_activation_tests|product_ascii_room_authoring_tests|product_ascii_authoring_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "asciiRoomPreview|asciiRoomDraft|buildProductAsciiRoomPreview|decodeProductAsciiRoomAutomationText" src/app/iggy3d/automation tests/unit tests/smoke`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/Authoring.*` unless preview authoring contract changes.
- `src/app/iggy3d/automation/Automation.*` unless command routing changes.
- `src/app/iggy3d/receipt/WorldAuthoringFields.*` unless preview receipt field projection changes.

## Update When

- Draft-to-request mapping, preview recording fields, automation escape decoding, or preview build entry points change.

## Do Not Update When

- Only source/grid/asset conversion internals or activation/session flow changes without preview state contract changes.
