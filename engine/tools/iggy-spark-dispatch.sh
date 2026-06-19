#!/usr/bin/env bash
set -euo pipefail

ROOT="${IGGY_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
LEDGER="${IGGY_SPARK_DISPATCH_LEDGER:-$ROOT/engine/research/departments/integration/spark_dispatch_ledger.tsv}"
SHOW_ALL=0
STRICT=0

usage() {
  cat <<'USAGE'
Usage: engine/tools/iggy-spark-dispatch.sh [--ledger PATH] [--all] [--strict]

Reads the Spark dispatch ledger and prints active packet state, blockers, and
next planner/worker actions.

Options:
  --ledger PATH  Read a non-default TSV ledger.
  --all          Include complete/cancelled packet rows.
  --strict       Exit nonzero when rows are malformed or blocked.
  --help, -h     Show this help text.

Default ledger:
  engine/research/departments/integration/spark_dispatch_ledger.tsv
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --ledger)
      if [[ $# -lt 2 ]]; then
        echo "--ledger requires a path" >&2
        exit 2
      fi
      LEDGER="$2"
      shift
      ;;
    --all)
      SHOW_ALL=1
      ;;
    --strict)
      STRICT=1
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

if [[ ! -f "$LEDGER" ]]; then
  echo "missing dispatch ledger: $LEDGER" >&2
  exit 1
fi

branch="$(git -C "$ROOT" branch --show-current 2>/dev/null || printf 'unknown')"
dirty_count="$(git -C "$ROOT" status --short 2>/dev/null | wc -l | tr -d ' ')"

printf 'Spark dispatch ledger\n'
printf 'repo:   %s\n' "$ROOT"
printf 'branch: %s\n' "$branch"
printf 'dirty:  %s changed path(s)\n' "$dirty_count"
printf 'ledger: %s\n\n' "$LEDGER"

awk -v show_all="$SHOW_ALL" -v strict="$STRICT" '
BEGIN {
  FS = "\t";
  expected = "packet\tstatus\tdepartment\towner\tsource_commit\tsource_review\tdocs_commit\tdocs_review\tblocked_reason\tnext_action\tupdated";
}
NR == 1 {
  if ($0 != expected) {
    print "invalid ledger header" > "/dev/stderr";
    print "expected: " expected > "/dev/stderr";
    print "actual:   " $0 > "/dev/stderr";
    invalid = 1;
    exit 2;
  }
  next;
}
NF != 11 {
  printf("invalid row %d: expected 11 TSV fields, got %d\n", NR, NF) > "/dev/stderr";
  issue_count++;
  invalid = 1;
  next;
}
{
  packet = $1;
  status = $2;
  department = $3;
  owner = $4;
  source_commit = $5;
  source_review = $6;
  docs_commit = $7;
  docs_review = $8;
  blocked_reason = $9;
  next_action = $10;
  updated = $11;

  done = status == "complete" || status == "cancelled";
  if (!show_all && done)
    next;

  row_issue = "";
  if (packet == "" || status == "" || department == "" || owner == "")
    row_issue = "missing required identity field";
  if (status == "blocked" && blocked_reason == "")
    row_issue = "blocked row needs blocked_reason";
  if ((status == "source_review" || status == "docs_ready" || status == "docs" || status == "docs_review" || status == "complete") && source_commit == "")
    row_issue = "status needs source_commit";
  if ((status == "docs_ready" || status == "docs" || status == "docs_review" || status == "complete") && source_review !~ /^PASS/)
    row_issue = "docs phase needs source_review PASS";
  if ((status == "docs_review" || status == "complete") && docs_commit == "")
    row_issue = "status needs docs_commit";

  if (row_issue != "") {
    issue_count++;
    printf("ledger issue: %s: %s\n", packet, row_issue) > "/dev/stderr";
  }
  if (status == "blocked")
    blocked_count++;

  if (!printed) {
    printf("%-46s %-16s %-13s %-12s %-10s %-12s %s\n", "packet", "status", "department", "owner", "source", "docs", "next");
    printf("%-46s %-16s %-13s %-12s %-10s %-12s %s\n", "------", "------", "----------", "-----", "------", "----", "----");
    printed = 1;
  }

  source_short = source_commit == "" ? "-" : source_commit;
  docs_short = docs_commit == "" ? "-" : docs_commit;
  action = next_action == "" ? default_action(status, owner, blocked_reason) : next_action;
  printf("%-46s %-16s %-13s %-12s %-10s %-12s %s\n", packet, status, department, owner, source_short, docs_short, action);
}
END {
  if (!printed)
    print "No active packets.";
  if (issue_count > 0)
    printf("\nissues: %d ledger issue(s)\n", issue_count) > "/dev/stderr";
  if (blocked_count > 0)
    printf("\nblocked: %d packet(s) require planner action\n", blocked_count) > "/dev/stderr";
  if (strict && issue_count > 0)
    exit 3;
  if (strict && invalid)
    exit 3;
  if (strict && blocked_count > 0)
    exit 4;
}
function default_action(status, owner, blocked_reason) {
  if (status == "planned") return "planner chooses and dispatches first worker";
  if (status == "research") return "researcher returns PASS/NO-GO scout";
  if (status == "preflight_review") return "reviewer returns PASS/NO-GO before build";
  if (status == "build_ready") return "builder can start";
  if (status == "building") return "wait for builder commit or blocker";
  if (status == "source_review") return "reviewer gates source commit";
  if (status == "docs_ready") return "release finisher docs sync";
  if (status == "docs") return "wait for finisher docs commit";
  if (status == "docs_review") return "reviewer gates docs commit";
  if (status == "blocked") return "planner resolves blocker: " blocked_reason;
  if (status == "complete") return "closed";
  if (status == "cancelled") return "closed";
  return "unknown status: planner inspect";
}
' "$LEDGER"
