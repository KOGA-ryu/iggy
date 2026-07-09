# File Spec

Files: `src/app/AppConfig.hpp`, `src/app/AppConfig.cpp`

Verified at: `9262eadf`

## Owns

- Core command-line app configuration packet and primary app modes.
- Default app configuration for headless demo startup.
- Validation rules for package, save, replay, expected summary, realtime camera, help/version, and primary-mode conflicts.
- Stable app config status code strings.

## Does Not Own

- Command-line token parsing.
- Product app/window options.
- Package runtime root lookup.
- Runtime session execution, replay execution, or validation package behavior.
- Smoke-test command construction.

## Reads

- `AppConfig` values.
- `RuntimeConfig` and requested `CameraMode`.
- Filesystem path shape, filename, replay extension, parent components, and legacy `/Users/kogaryu/iggy` path prefix.

## Writes / Mutates

- Returns default `AppConfig`.
- Returns `AppConfigStatus` and status code strings.
- Does not mutate filesystem, runtime state, app state, or environment.

## Calls Out To / Wires Out To

- `CliParser.cpp` creates defaults and calls validation after parsing CLI tokens.
- App entry code can use status codes for diagnostics and exit behavior.

## Called By / Entry Points

- `makeDefaultAppConfig()`.
- `validateAppConfig(...)`.
- `appConfigStatusCode(...)`.
- Focused proof: `rg -n "makeDefaultAppConfig|validateAppConfig|appConfigStatusCode|AppConfigStatus" src tests`.

## Invariants

- Help and version modes validate only with matching primary mode flags.
- First-person and third-person are the only accepted realtime camera modes.
- Package path must name `package.iggy3d.toml` when provided.
- Replay paths must carry the `.iggy3d.replay` suffix.
- Legacy `/Users/kogaryu/iggy` paths and parent-path components are rejected for guarded path fields.

## Tests / Proof Commands

- `rg -n "makeDefaultAppConfig|validateAppConfig|appConfigStatusCode" src tests`.
- Unknown from this slice: no focused unit target was found for `AppConfig.*`.

## Nearby Files Usually Not Touched

- `src/app/CliParser.*` unless CLI parsing or validation handoff changes.
- `src/config/RuntimeConfig.hpp` unless runtime config fields change.
- `src/runtime/camera/CameraState.hpp` unless app-facing camera mode names change.

## Update When

- App modes, config fields, validation rules, default package path, status enum, or status code strings change.

## Do Not Update When

- Only product-specific options, package lookup behavior, or smoke command wrappers change without changing `AppConfig`.
