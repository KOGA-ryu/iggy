# Vulkan Validation Message Policy

This document defines which Vulkan validation messages fail strict lanes, how message IDs are logged, how allowlists are represented, where allowlists may live, and how `iggy3d` avoids hiding real renderer errors.

Validation messages are renderer diagnostics. They do not own runtime truth, gameplay command legality, camera truth, save/load state, replay determinism, package validation, or projection semantics.

## Purpose

Define strict validation behavior before implementation:

```text
validation_error_in_strict_lane=fail
sync_validation_hazard_in_strict_lane=fail
validation_warning_in_first_strict_lane=log_and_count
warning_clean_lane=deferred
allowlist_default=empty
allowlist_requires_owner_issue_expiry=true
substring_suppression_default=forbidden
runtime_mutation_from_validation=false
```

This document narrows:

- [debug_validation.md](debug_validation.md)
- [vulkan_result_and_error_policy.md](vulkan_result_and_error_policy.md)
- [vulkan_ci_and_smoke_lanes.md](vulkan_ci_and_smoke_lanes.md)
- [vulkan_debug_labels_and_capture.md](vulkan_debug_labels_and_capture.md)
- [vulkan_threading_and_frame_ownership.md](vulkan_threading_and_frame_ownership.md)
- [diagnostics_and_tests.md](diagnostics_and_tests.md)
- [sync_contract.md](sync_contract.md)
- [vulkan_image_layouts_and_barriers.md](vulkan_image_layouts_and_barriers.md)
- [boundaries.md](boundaries.md)

## Source Priority

Use these sources before implementation:

| Source | Use for |
| --- | --- |
| Vulkan Validation Overview: https://docs.vulkan.org/guide/latest/validation_overview.html | validation purpose, VUID meaning, undefined behavior risk, `VK_LAYER_KHRONOS_validation` role |
| Vulkan Specification debugging chapter: https://docs.vulkan.org/spec/latest/chapters/debugging.html | `VK_EXT_debug_utils` severities, message types, callback data, callback threading behavior |
| LunarG Synchronization Validation docs: https://vulkan.lunarg.com/doc/view/latest/windows/synchronization_usage.html | sync hazard message behavior, sync validation enablement, extra properties for filtering |
| Vulkan Specification/Registry: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html | exact VUID references and API valid usage truth |
| Project validation/CI/result docs | local strict/optional lane behavior, reason codes, receipts, runtime firewall |

Priority rule:

```text
valid_usage_truth=Vulkan Specification
message_callback_truth=Vulkan debug utils specification
validation_layer_behavior=Khronos and LunarG validation docs
project_failure_policy=iggy3d Vulkan docs
runtime_truth=iggy3d runtime docs, never validation messages
```

## Scope

In scope:

- validation message severity policy;
- message type policy;
- strict lane failure rules;
- optional lane behavior;
- sync validation hazard behavior;
- message ID/VUID capture;
- validation log format;
- allowlist shape and storage;
- allowlist expiry and review;
- suppression restrictions;
- diagnostics receipt fields;
- tests and review gates.

Out of scope:

- implementing validation callback code now;
- changing Vulkan validation layer behavior;
- replacing validation with RenderDoc;
- shipping validation layers in release builds by default;
- general logging system design;
- user-facing error UI;
- runtime/gameplay error taxonomy;
- suppressing driver crashes or device loss.

## Local File Surface

Likely future files:

```text
src/render/vulkan/DebugValidation.hpp
src/render/vulkan/DebugValidation.cpp
src/render/vulkan/VulkanResult.hpp
src/render/vulkan/VulkanResult.cpp
src/render/RenderDiagnostics.hpp
src/render/RenderDiagnostics.cpp
cmake/iggy3d_vulkan_smoke.cmake
tests/unit/render_validation_message_policy_tests.cpp
tests/unit/render_validation_allowlist_tests.cpp
tests/smoke/vulkan_validation_smoke.cpp
tests/smoke/vulkan_sync_smoke.cpp
tests/smoke/vulkan_diagnostics_smoke.cpp
docs/vulkan/validation_allowlist.md
tests/data/vulkan/validation_allowlist.kv
build/artifacts/render_diagnostics/validation/
```

This document does not implement those files.

## Ownership

| Item | Owner | Must never own |
| --- | --- | --- |
| debug messenger callback | `DebugValidation` | runtime mutation |
| message counters | `DebugValidation` / diagnostics | gameplay state |
| first message summaries | diagnostics | save/replay truth |
| validation log artifact | diagnostics/test harness | source assets |
| allowlist source | docs/test data with review owner | code path hiding errors |
| strict fail decision | smoke app/test wrapper | runtime command legality |
| sync hazard classification | validation policy | resource barrier implementation |

Rules:

- validation messages never mutate runtime;
- validation callback must not throw across Vulkan C callback boundary;
- validation callback must not submit Vulkan work;
- validation callback must not call runtime/projection/save APIs;
- strict lanes fail after the attempted Vulkan operation emits a receipt;
- allowlists are test/diagnostic policy, not renderer correctness.

## Message Severity Policy

Track all debug utils severities:

```text
verbose
info
warning
error
```

First policy:

| Severity | Optional Vulkan smoke | Strict Vulkan smoke | Notes |
| --- | --- | --- | --- |
| verbose | log only when verbose diagnostics requested | log only when verbose diagnostics requested | never a failure by itself |
| info | count/log summary | count/log summary | never a failure by itself |
| warning | count/log summary | count/log summary initially | warning-clean lane deferred |
| error | fail attempted test | fail attempted test | cannot be allowlisted without explicit exception |

Rules:

- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT` fails strict lanes;
- error severity also fails optional lanes when Vulkan work was attempted;
- warnings are not ignored; they are counted, logged, and visible in receipts;
- a future `warning_clean` lane may fail warnings after first renderer stability;
- verbose/info logs must be bounded.

## Message Type Policy

Track debug utils message types:

```text
general
validation
performance
device_address_binding
```

First policy:

| Type | Default handling |
| --- | --- |
| validation | fail on error, count warnings |
| performance | count/log; fail only in future performance-clean lane |
| general | count/log; fail if severity is error |
| device_address_binding | out of first path unless feature adopted |

Rules:

- validation errors always matter;
- performance warnings are useful but must not block first-room strict smoke unless promoted;
- message type does not override severity;
- sync validation hazard messages are treated as validation errors even when they do not carry stable VUIDs.

## Strict Failure Rules

Strict Vulkan smoke fails when:

- validation layer is required but unavailable;
- debug messenger is required but cannot be created;
- any unallowlisted error-severity validation message occurs;
- sync validation reports a hazard;
- validation callback records a message with unknown severity/type that cannot be classified;
- validation allowlist file is malformed when allowlist support is enabled;
- runtime hash changes during a validation-enabled render test.

Strict Vulkan smoke does not fail merely because:

- validation is disabled by a non-validation lane;
- optional validation layer is unavailable in an optional lane;
- warning messages exist in first-room strict lane;
- verbose/info messages exist;
- platform automation is unavailable in a test that is explicitly diagnostic-only.

Hard rule:

```text
strict_validation_error_count_must_be_zero_after_allowlist=true
```

## Optional Lane Rules

Optional Vulkan smoke may skip only when:

- Vulkan loader/driver/display is unavailable before renderer work;
- validation layer is unavailable and the test is specifically validation-optional;
- sync validation is unavailable and the test is specifically sync-validation-optional.

Optional Vulkan smoke must fail when:

- validation error occurs during attempted renderer work;
- sync validation hazard occurs during attempted renderer work;
- malformed allowlist is loaded;
- renderer hides a validation error as a skip;
- runtime hash changes.

Reason:

```text
optional_means_environment_may_be_missing
optional_does_not_mean_renderer_bugs_are_allowed
```

## Message Identity

Every validation callback record should capture:

```text
message_severity
message_type
message_id_name
message_id_number
vuid
api_command
object_type
object_name
queue_label
command_label
frame_index
swapchain_generation
message_text_hash
message_text_excerpt
```

Rules:

- `message_id_name` is the primary identity when present;
- VUID is captured when present in `message_id_name` or message text;
- sync validation may not provide a normal VUID; use extra properties where available;
- full message text goes to artifact logs, not stable receipt parsing;
- receipts include first message summaries and counts, not unbounded logs.

For core validation messages:

```text
preferred_key=VUID_or_message_id_name
fallback_key=message_id_number_plus_text_hash
```

For sync validation messages:

```text
preferred_key=sync_extra_properties_if_available
fallback_key=hazard_type_plus_api_command_plus_resource_class_plus_text_hash
```

## VUID Policy

VUIDs are useful because they point to exact valid usage rules.

Rules:

- if a message has a `VUID-*` identifier, log it exactly;
- do not suppress a VUID by broad substring;
- a VUID allowlist entry must include the Vulkan API call or subsystem context;
- VUID allowlist entries must expire;
- a VUID that appears on macOS/MoltenVK, Linux, and Windows is presumed to be an `iggy3d` bug until proven otherwise.

Receipt fields:

```text
first_validation_vuid=
validation_vuid_count=
validation_unique_vuids=
```

## Sync Validation Hazard Policy

Sync validation reports hazards such as read-after-write, write-after-read, write-after-write, and racing accesses.

Strict policy:

```text
sync_hazard_in_strict_lane=fail
sync_hazard_allowlist_default=forbidden
```

Rules:

- sync hazards usually mean image layout, access mask, stage mask, semaphore, fence, or queue-ordering bugs;
- sync hazard messages should include command label and resource name when labels are available;
- allowlisting sync hazards requires a separate reviewer note explaining why the access is safe;
- suppressing sync hazards by raw prose text is forbidden unless no stable extra property exists and the entry is temporary;
- a sync hazard in first-room rendering blocks the renderer from being considered clean.

Receipt fields:

```text
sync_validation_error_count=
sync_validation_warning_count=
first_sync_hazard_type=
first_sync_hazard_command=
first_sync_hazard_resource=
sync_validation_clean=true|false
```

## Allowlist Storage

Allowed locations:

```text
docs/vulkan/validation_allowlist.md
tests/data/vulkan/validation_allowlist.kv
```

Roles:

- markdown file explains human rationale and review status;
- key-value test-data file is the machine-readable allowlist for smoke tests;
- both must be reviewed together if machine-readable suppression changes;
- no allowlist entries belong in runtime/content/projection/save code;
- no allowlist entries belong hardcoded inside `DebugValidation.cpp` except bootstrap tests before external file support, and those must be removed before strict lanes.

Forbidden locations:

```text
src/runtime/**
src/content/**
src/projection/**
src/runtime/save/**
shaders/**
apps/iggy3d_headless_demo/**
```

Reason:

```text
validation_allowlist_is_renderer_test_policy_not_game_truth
```

## Allowlist Entry Shape

Machine-readable entry shape should include:

```text
id=temporary-human-readable-id
enabled=true
severity=warning
message_type=validation
message_id_name=VUID-example
message_id_number=0
vuid=VUID-example
sync_hazard_type=
api_command=vkExample
subsystem=swapchain|sync|pipeline|memory|debug
platforms=macos_moltenvk
lanes=optional_vulkan_smoke
expires=YYYY-MM-DD
owner=name-or-team
issue=issue-or-doc-link
rationale=short reason
replacement_work=what removes this entry
```

Rules:

- `enabled=false` entries are allowed only as history in markdown, not active key-value data;
- `expires` is required for every active entry;
- `owner` is required;
- `issue` or local file-plan reference is required;
- `severity=error` allowlist entries are forbidden by default;
- platform and lane scope must be as narrow as possible;
- no wildcard platform for active suppressions unless reviewer explicitly accepts it.

## Allowed Allowlist Uses

Allowlist may be used for:

- known validation-layer false positive with linked upstream issue;
- platform-specific MoltenVK/loader message that is not an invalid usage error;
- temporary warning while a narrow renderer packet is in flight;
- performance warning in a non-performance lane;
- diagnostic-only test where the warning is the expected output.

Allowlist must not be used for:

- first-room validation errors;
- sync hazards in normal render path;
- missing image layout transitions;
- command buffer lifetime errors;
- descriptor lifetime errors;
- swapchain acquire/present misuse;
- fence reset deadlocks;
- invalid shader interface;
- out-of-memory or device-loss behavior;
- hiding bugs to make CI green.

## Suppression Rules

Default:

```text
suppression_by_substring=false
broad_wildcards=false
allowlist_matches_must_be_exact_or_structured=true
```

Allowed match keys, in preference order:

1. `vuid`
2. `message_id_name`
3. sync validation extra properties
4. `message_id_number` plus `api_command`
5. `message_text_hash` plus narrow platform/lane expiry

Rules:

- substring matching on prose message text is forbidden unless no stable structured key exists;
- message text hash suppressions must expire within 14 days by default;
- allowlist cannot reduce the logged count to zero; it must report `allowlisted_count`;
- allowlisted messages still appear in artifacts;
- allowlist hit rate must be visible in receipts.

## Logging Policy

Validation artifact files:

```text
build/artifacts/render_diagnostics/validation/<test_name>.validation.log
build/artifacts/render_diagnostics/validation/<test_name>.validation.summary
```

Each full log entry should include:

```text
timestamp_or_frame
thread_model
severity
type
message_id_name
message_id_number
vuid
api_command
objects
object_names
queue_label
command_label
frame_index
swapchain_generation
allowlisted=true|false
allowlist_entry_id=
message_text
```

Rules:

- artifact logs may contain full validation message text;
- receipt text should contain bounded excerpts only;
- callback should not perform slow file IO directly;
- callback can enqueue/copy bounded data to renderer-owned diagnostics state;
- logs must not expose raw Vulkan handles through public renderer API, though artifact logs may include handles for debugging.

## Receipt Fields

Validation receipts should include:

```text
validation_requested=off|optional|required
validation=enabled|disabled|unavailable
sync_validation_requested=off|optional|required
sync_validation=enabled|disabled|unavailable
debug_messenger=enabled|disabled|unavailable
validation_message_policy_version=1
validation_error_count=
validation_warning_count=
validation_info_count=
validation_verbose_count=
validation_allowlist_enabled=true|false
validation_allowlist_path=
validation_allowlist_entry_count=
validation_allowlisted_count=
validation_unallowlisted_error_count=
validation_unallowlisted_warning_count=
first_validation_message_id_name=
first_validation_message_id_number=
first_validation_vuid=
first_validation_severity=
first_validation_type=
first_validation_command=
first_validation_object_name=
first_validation_excerpt=
sync_validation_clean=true|false|unavailable
sync_validation_error_count=
first_sync_hazard_type=
strict_vulkan=true|false
result=pass|fail|skip
reason_code=
```

Rules:

- strict pass requires `validation_unallowlisted_error_count=0`;
- strict sync pass requires `sync_validation_clean=true` when sync validation is required;
- unavailable validation is not the same as clean validation;
- allowlisted counts must be visible even when result is pass.

## Release Build Policy

Validation layers are development/smoke infrastructure.

Rules:

- release builds do not enable validation by default;
- release packages must not require validation layers to run;
- strict smoke can require validation even in a release-like build tree if the test explicitly enables it;
- validation allowlists are not shipped gameplay data;
- absence of validation layers in a user install is not gameplay failure.

## Platform Policy

macOS/MoltenVK:

- validation availability depends on Vulkan SDK/MoltenVK installation;
- MoltenVK portability messages are diagnosed, not cross-platform truth;
- platform-specific warnings can be allowlisted only with `platforms=["macos_moltenvk"]`.

Linux native Vulkan:

- strict Linux lane should treat validation errors as native Vulkan proof failures;
- software Vulkan lane is optional aid and cannot replace native GPU proof.

Windows native Vulkan:

- strict Windows lane should treat validation errors as native Vulkan proof failures;
- Visual Studio debugger/vkconfig convenience does not replace receipt artifacts.

## Runtime Firewall

Forbidden:

```text
src/runtime/** reads validation messages
src/content/** reads validation messages
src/projection/** reads validation messages
src/runtime/save/** reads validation messages
runtime hash includes validation message state
runtime command legality depends on validation result
save/replay serializes validation allowlist state
```

Allowed:

```text
src/render/vulkan/** handles validation callback
src/render/** stores backend-neutral diagnostics summaries
tests/smoke/vulkan_* evaluates strict/optional result
docs/vulkan/validation_allowlist.md documents temporary exceptions
tests/data/vulkan/validation_allowlist.kv feeds smoke tests
```

Firewall scan:

```sh
rg -n "validation_allowlist|validation_message|VUID-|VK_DEBUG_UTILS_MESSAGE" \
  src/runtime src/content src/projection src/runtime/save \
  apps/iggy3d_headless_demo apps/iggy3d_replay_tool apps/iggy3d_validate_package
```

Expected result:

```text
no runtime/content/projection/save validation ownership leak
```

## Failure Reason Codes

Validation reason codes:

```text
validation_layer_missing
debug_messenger_missing
validation_error
validation_warning_clean_failure
validation_unknown_message_class
validation_allowlist_malformed
validation_allowlist_expired
validation_allowlist_missing_owner
validation_allowlist_missing_issue
validation_allowlist_too_broad
validation_allowlist_error_forbidden
validation_message_suppressed_by_policy
sync_validation_required_unavailable
sync_validation_hazard
sync_validation_extra_properties_missing
runtime_hash_changed
runtime_validation_leak
```

Rules:

- `validation_message_suppressed_by_policy` should be rare and reviewed;
- expired or malformed allowlist is a strict failure;
- error-severity allowlist entry is a strict failure unless a specific future packet explicitly permits one.

## Test Gates

Unit tests should prove:

- severity mapping is correct;
- error severity fails strict lanes;
- warning severity logs/counts without failing first strict lane;
- malformed allowlist fails;
- expired allowlist entry fails;
- missing owner/issue fails;
- broad wildcard allowlist fails;
- error-severity allowlist entry fails by default;
- allowlisted messages still increment `allowlisted_count`;
- runtime firewall scan is clean.

Smoke tests should prove:

- validation-required lane fails when layer missing;
- validation error during attempted render fails;
- sync validation hazard fails;
- validation warning is logged and counted;
- allowlist hit appears in receipt and artifact;
- strict pass has zero unallowlisted errors;
- runtime hash is unchanged by validation callback activity.

## Review Checklist

Reviewer should reject a validation-message packet if:

- it suppresses messages by broad substring;
- it ignores `ERROR` severity in strict lanes;
- it treats sync validation hazards as warnings;
- it hides allowlisted counts from receipts;
- it stores allowlists in runtime/content/projection/save;
- it hardcodes a permanent suppression in C++;
- it lacks expiry/owner/issue for an allowlist entry;
- it removes validation messages without proving the underlying bug was fixed;
- it claims validation is clean when validation was unavailable.

## Acceptance Criteria

This policy is ready for implementation when:

- [ ] validation callback captures severity, type, message ID/name, VUID, command, object names, labels, and frame context;
- [ ] strict lanes fail on unallowlisted validation errors;
- [ ] strict sync lanes fail on sync hazards;
- [ ] allowlist storage and schema are implemented only in renderer test/diagnostic surfaces;
- [ ] expired/malformed/broad allowlist entries fail tests;
- [ ] receipts include validation counts and allowlist counts;
- [ ] validation artifacts include full message detail;
- [ ] runtime firewall scan is clean;
- [ ] release/default app path does not require validation layers.

## Non-Goals For First Renderer Pass

Do not add:

- permanent validation suppression;
- broad text substring filters;
- release-build validation dependency;
- runtime-visible validation messages;
- user-facing validation UI;
- performance-warning-clean lane;
- GPU-assisted validation requirement;
- automated upstream issue filing;
- RenderDoc replacement for validation.
