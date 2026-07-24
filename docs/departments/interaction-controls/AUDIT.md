# Interaction and Controls Audit

Scheduled after Persistence and Validation. The audit will establish one
semantic route per action and identify raw-input or duplicate dispatch paths.

| ID | Surface | Classification | Evidence | Disposition | Priority |
| --- | --- | --- | --- | --- | --- |
| INT-A1-001 | `InputRouter` to Controls repeat/navigation | Canonical Owner | Controls no longer rescans physical bindings after routing; `InputRouter` is canonical for context, priority, and consumption | Keep | P0 |
