# Spark Dispatch Watchdog

Purpose: keep the worker chain moving without letting Spark become the planner.
Spark watches packet state, reports idle or blocked gates, and points at the next
mechanical action. Planner Dex still owns roadmap order, packet boundaries, and
gate overrides.

## Files

- `spark_dispatch_ledger.tsv`: durable packet state.
- `../../../tools/iggy-spark-dispatch.sh`: local watchdog report.

## Run

```sh
engine/tools/iggy-spark-dispatch.sh
engine/tools/iggy-spark-dispatch.sh --all
engine/tools/iggy-spark-dispatch.sh --strict
```

Use `--all` for closed packet audit and `--strict` when a monitor should fail on
malformed or blocked rows.

## Ledger Columns

The TSV schema is fixed:

```text
packet status department owner source_commit source_review docs_commit docs_review blocked_reason next_action updated
```

Keep fields tab-separated and do not add tabs inside field text.

Recommended status values:

- `planned`: planner has identified a packet but no worker has been dispatched.
- `research`: researcher scout is active.
- `preflight_review`: reviewer is gating the boundary before build.
- `build_ready`: builder is allowed to start.
- `building`: builder owns source implementation.
- `source_review`: reviewer is gating the source commit.
- `docs_ready`: finisher may begin docs sync.
- `docs`: finisher owns docs sync.
- `docs_review`: reviewer is gating docs.
- `blocked`: planner action is required.
- `complete`: source/docs path is closed.
- `cancelled`: packet was intentionally abandoned.

## Spark Rules

- Spark may update ledger rows, run the watchdog, and nudge the next assigned
  worker with the existing order template.
- Spark may report a planner decision is required.
- Spark must not choose roadmap priority, widen packet boundaries, approve
  reviewer findings, merge branches, or edit source as a hidden side effect.
- Spark should treat dirty worktrees, missing commits, or mixed source/docs state
  as planner-visible blockers.

## Packet Flow

```text
planned
  -> research
  -> preflight_review
  -> build_ready
  -> building
  -> source_review
  -> docs_ready
  -> docs
  -> docs_review
  -> complete
```

Small packets can skip preflight when Planner Dex explicitly records the
override, but the ledger should then show why the builder was allowed to start.

## Why

The worker chain is useful only when every role has an obvious next action. The
ledger prevents silent idle states such as:

- builder waiting on a reviewer gate that planner intended to waive;
- finisher holding after reviewer PASS;
- reviewer waiting for a commit hash that builder already produced;
- docs sync happening before source review.

The watchdog does not replace judgment. It makes missing handoffs visible.
