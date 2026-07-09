# File Spec

Files: `src/app/CliParser.hpp`, `src/app/CliParser.cpp`

Verified at: `9262eadf`

## Owns

- Minimal command-line parser for core app modes and core app config fields.
- CLI parse result packet with `AppConfig`, status, and diagnostics.
- Help and version text for the core CLI.
- Diagnostics for conflicting primary modes, unknown options, missing option values, invalid camera mode, and invalid app config.

## Does Not Own

- App config validation rules after parsing.
- Product app-specific option parsing.
- Runtime execution, package validation, replay execution, or smoke harness commands.
- Diagnostic rendering or process exit behavior.

## Reads

- `argc` and `argv`, or a vector of string-view arguments.
- Core options for demo, validate-package, replay, help, version, package/fixture, save, replay path, summary, camera, and verbose.
- `AppConfig` defaults and validation status.

## Writes / Mutates

- Returns `CliParseResult`.
- Mutates only the local parse result while parsing.
- Does not mutate environment, filesystem, runtime state, or app state.

## Calls Out To / Wires Out To

- Calls `makeDefaultAppConfig()`.
- Calls `validateAppConfig(...)` and `appConfigStatusCode(...)`.
- Builds `Diagnostic` records through core diagnostics helpers.

## Called By / Entry Points

- `parseCommandLine(int, const char* const*)`.
- `parseCommandLine(std::vector<std::string_view>)`.
- `cliHelpText()`.
- `cliVersionText()`.
- Focused proof: `rg -n "parseCommandLine|cliHelpText|cliVersionText|CliParseResult" src tests`.

## Invariants

- The program name is skipped when the first argument is not an option.
- Only one primary mode may be selected.
- Options that require values reject missing values or a following option token.
- `--package` and `--fixture` are aliases for the same package path.
- Camera values are case-sensitive and limited to `FirstPerson` and `ThirdPerson`.
- Final validation is delegated to `AppConfig.*`.

## Tests / Proof Commands

- `rg -n "parseCommandLine|cliHelpText|cliVersionText" src tests`.
- Unknown from this slice: no focused unit target was found for `CliParser.*`.

## Nearby Files Usually Not Touched

- `src/app/AppConfig.*` unless core app config validation or fields change.
- `src/core/diagnostics/Diagnostic.*` unless diagnostic construction contracts change.
- Product option parsers unless product-specific CLI flags move into core CLI.

## Update When

- Core CLI options, parse result fields, diagnostic codes, help/version text, or parser-to-config handoff changes.

## Do Not Update When

- Only product app options, smoke command strings, or runtime execution behavior changes without changing this parser.
