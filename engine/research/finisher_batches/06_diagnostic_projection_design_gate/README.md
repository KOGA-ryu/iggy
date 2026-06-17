# 06 Diagnostic Projection Design Gate

Status: complete.

Goal: design flattened diagnostic entries over nested file, TOML, source,
converter, profile, and run issues without changing validation semantics.

Slices:
- Inventory issue types.
- Propose compact fields.
- List first tests.
- Keep the packet docs-only unless implementation is approved and low conflict.

Verification:
- Docs diff review.
- `git diff --check`

Inventory source files:
- `engine/src/runtime/RuntimeGameplayAsciiSourcePlanTomlFileReader.hpp`
- `engine/src/runtime/RuntimeGameplayAsciiSourcePlanTomlReader.hpp`
- `engine/src/runtime/RuntimeGameplayAsciiSourcePlanValidator.hpp`
- `engine/src/runtime/RuntimeGameplayScenarioAuthoringAdapter.hpp`
- `engine/src/runtime/RuntimeGameplayAsciiSourcePlanProfileScenarioConverter.hpp`
- `engine/src/runtime/RuntimeGameplayProfileScenarioValidator.hpp`
- `engine/src/runtime/RuntimeGameplayScenarioValidator.hpp`
- `engine/src/runtime/RuntimeGameplayProfileScenarioRunner.hpp`
- `engine/apps/scenario_toml_runner/IggyScenarioTomlRunner.cpp`

Current diagnostic layers:
- File read: `RuntimeGameplayAsciiSourcePlanTomlFileReadIssue` with `code`,
  `path`, and `detail`.
- TOML read: `RuntimeGameplayAsciiSourcePlanTomlReadIssue` with `code`, source
  `line`/`column`, `table`, optional `tableIndex`, `key`, `detail`, and nested
  `RuntimeGameplayAsciiSourcePlanIssue` for source-plan validation failures.
- Source-plan validation: `RuntimeGameplayAsciiSourcePlanIssue` with `code`,
  `index`, `firstIndex`, `row`, `column`, optional `glyph`, and optional `id`.
- Authoring adapter: `RuntimeGameplayScenarioAuthoringAdapterIssue` with
  adapter `code`, source kind, payload/config presence flags, and nested
  source-plan conversion status.
- Source-plan conversion:
  `RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue` with `code`,
  source row/column/glyph, nested source issue, authored control, authored
  player command, actor/control/profile/interaction/item/AI-map registry issues,
  and nested profile scenario issue.
- Profile validation: `RuntimeGameplayProfileScenarioIssue` with `code`,
  `frameIndex`, `actorIndex`, `npcId`, `profileId`, nested profile trait issue,
  and nested scenario issue.
- Scenario validation: `RuntimeGameplayScenarioIssue` with `code`,
  `frameIndex`, `actorIndex`, `controlIndex`, `subjectIndex`,
  `firstSubjectIndex`, `npcId`, and nested frame/subject/map context.
- Run failure: no separate run issue type. `RuntimeGameplayProfileScenarioRunStatus::ValidationFailed`
  reports the runner's `RuntimeGameplayProfileScenarioValidationResult`.

Proposed entry fields:
- `stage`: one of `file`, `toml`, `source_plan`, `adapter`, `conversion`,
  `profile`, `scenario`, `run`.
- `code`: stable snake-case issue code text matching current CLI `ToString`
  names.
- `status`: optional status text for the owning result, such as
  `missing_file`, `source_plan_invalid`, or `profile_scenario_invalid`.
- `path`: file path when available.
- `line` and `column`: TOML source position when available.
- `table`, `tableIndex`, and `key`: TOML location when available.
- `row` and `columnInGrid`: source-plan grid position when available.
- `index` and `firstIndex`: source-plan or scenario list indices when
  available.
- `frameIndex`, `actorIndex`, `controlIndex`, `subjectIndex`: scenario/profile
  indices when available.
- `glyph`: source glyph when available.
- `id`, `npcId`, `profileId`, `targetId`: resource identifiers when available.
- `detail`: existing detail text only; do not invent new prose.
- `nestedStage` and `nestedCode`: optional breadcrumb for projected nested
  causes when the top-level entry is an adapter/conversion/profile wrapper.

API sketch:

```cpp
namespace iggy::runtime {

enum class RuntimeGameplayAuthoringDiagnosticStage {
	File,
	Toml,
	SourcePlan,
	Adapter,
	Conversion,
	Profile,
	Scenario,
	Run,
};

struct RuntimeGameplayAuthoringDiagnosticEntry {
	RuntimeGameplayAuthoringDiagnosticStage stage =
		RuntimeGameplayAuthoringDiagnosticStage::File;
	std::string code;
	std::string status;
	std::filesystem::path path;
	std::size_t line = 0;
	std::size_t column = 0;
	std::string table;
	bool hasTableIndex = false;
	std::size_t tableIndex = 0;
	std::string key;
	std::size_t row = 0;
	std::size_t columnInGrid = 0;
	std::size_t index = 0;
	std::size_t firstIndex = 0;
	std::size_t frameIndex = 0;
	std::size_t actorIndex = 0;
	std::size_t controlIndex = 0;
	std::size_t subjectIndex = 0;
	char glyph = '\0';
	ResourceId id;
	ResourceId npcId;
	ResourceId profileId;
	ResourceId targetId;
	std::string detail;
	std::string nestedStage;
	std::string nestedCode;
};

[[nodiscard]] std::vector<RuntimeGameplayAuthoringDiagnosticEntry>
projectTomlScenarioDiagnostics(
	const RuntimeGameplayTomlScenarioFacadeResult &result);

} // namespace iggy::runtime
```

Projection rules:
- Preserve existing failure semantics. The helper only reads result structs.
- Preserve existing ordering: file issues, TOML issues, nested source issues,
  adapter issues, conversion issues, profile issues, scenario issues.
- Do not drop wrapper issues. For example, keep the adapter
  `ascii_source_plan_conversion_failed` entry and add the nested conversion
  entry after it.
- Do not synthesize errors for absent optional data.
- Do not translate, redact, or reword `detail`.
- Do not add warning severity until a warning-channel packet opens that policy.
- Do not make diagnostics depend on directory scanning or package metadata.

First tests to add when implementation is safe:
- Missing file projects one `file` entry with `missing_file`, path, and detail.
- Corrupt TOML projects `file` `toml_read_failed` followed by `toml`
  `syntax_error` with line/column/table/key facts.
- Source-plan semantic invalid projects the TOML source-plan wrapper and nested
  `source_plan` issue facts.
- Unknown authored control projects adapter and conversion entries while
  preserving `npcId`.
- Profile validation failure projects conversion/profile entries while
  preserving frame, actor, NPC, and profile identifiers.
- Existing CLI failure diagnostics remain unchanged until an explicit
  output-contract packet updates and locks text.

Result:
- Design gate completed as docs only.
- No validation semantics, CLI output, or exit codes changed.
