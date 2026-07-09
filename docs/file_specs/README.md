# File Specs

File-specific cartography lives here.

Start with [INDEX.md](/Users/kogaryu/iggy3d/docs/file_specs/INDEX.md) for coverage status and the next batch queue.

Each spec mirrors one source path and records operational ownership:

- what the file or paired `.hpp`/`.cpp` surface owns
- what it must not own
- what it reads and mutates
- what it calls out to
- who calls it
- invariants future edits must preserve
- focused proof commands

Rules:

- Specs are grep-backed observations, not plans.
- Specs anchor by symbols and paths, not line numbers.
- Specs are updated only when the documented file contract changes.
- Paired `.hpp`/`.cpp` files normally share one spec named after the source stem.
- Folder-level cartography is not stored in `AGENTS.md` for this project.
