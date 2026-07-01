#!/usr/bin/env python3
"""Validate + compile/activate an iggy3d ASCII room grid, printing the render receipt.

This is the closed-loop AI-authoring harness: draft an ASCII grid, and this
validates it locally (rectangular rows, exactly one 'P', legal glyphs) BEFORE
the engine sees it, then compiles (preview) or activates (live session) it
headless and prints the ascii_room_* receipt keys that prove what was built.

Usage:
    tools/ascii_room.py <grid-file>                 # preview compile
    tools/ascii_room.py <grid-file> --activate      # boot a live session
    tools/ascii_room.py <grid-file> --room-id vault --activate

The grid file is a plain ASCII room (one glyph per cell, rectangular rows;
a leading '*<scale>' or 'floorN' layer directive is allowed). The full glyph
vocabulary and authoring rules live in docs/ascii_dungeon_authoring_reference.md.
Requires build/iggy3d (make -C build -j8 iggy3d_app).
"""
import argparse
import os
import subprocess
import sys
import tempfile

# The complete closed glyph vocabulary (AsciiRoomGrid.cpp kGlyphs) + space.
LEGAL = set("#J. 0123^v<>!+sPNMKTRCLE?$")

# Directive lines that are not part of the glyph grid (skipped when validating
# widths/spawn): a leading '*<scale>' and 'floorN' layer markers.
def _is_directive(line: str) -> bool:
    return line.startswith("*") or (line.startswith("floor") and line[5:].isdigit())


def validate(rows):
    grid = [r for r in rows if not _is_directive(r)]
    errs = []
    if not grid:
        errs.append("no glyph rows found")
        return errs, 0, 0
    width = len(grid[0])
    for i, r in enumerate(grid):
        if len(r) != width:
            errs.append(f"ragged rows: grid row {i} width {len(r)} != {width}")
        for j, c in enumerate(r):
            if c not in LEGAL:
                errs.append(f"illegal glyph {c!r} at grid row {i} col {j}")
    spawns = sum(r.count("P") for r in grid)
    if spawns != 1:
        errs.append(f"spawn count: found {spawns} 'P', need exactly 1")
    return errs, width, len(grid)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("grid", help="path to the ASCII room grid file")
    ap.add_argument("--activate", action="store_true",
                    help="boot a live gameplay session (default: preview compile only)")
    ap.add_argument("--room-id", default="scratch_room")
    args = ap.parse_args()

    repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    app = os.path.join(repo, "build", "iggy3d")
    if not os.access(app, os.X_OK):
        sys.exit("error: build/iggy3d missing — run: make -C build -j8 iggy3d_app")

    rows = open(args.grid).read().splitlines()
    errs, width, height = validate(rows)
    if errs:
        print("VALIDATION FAILED (fix the grid before the engine sees it):", file=sys.stderr)
        for e in errs:
            print("  -", e, file=sys.stderr)
        sys.exit(1)
    print(f"validate: OK ({height} grid rows x {width} cols, exactly 1 spawn, legal glyphs)")

    key = "activate" if args.activate else "build"
    # Rows are joined by the two-char escape backslash-n (NOT a real newline);
    # a real newline ends the automation command. See reference doc section 5.1.
    text = "\\n".join(rows) + "\\n"
    with tempfile.NamedTemporaryFile("w", suffix=".ctl", delete=False) as f:
        f.write(f"ascii_room.room_id={args.room_id}\n")
        f.write("ascii_room.text=" + text + "\n")
        f.write(f"ascii_room.{key}=true\n")
        ctl = f.name
    try:
        proc = subprocess.run(
            [app, "--no-window", "--automation-control", ctl, "--print-render-receipt"],
            capture_output=True, text=True)
    finally:
        os.unlink(ctl)

    prefix = "ascii_room_activation_" if args.activate else "ascii_room_preview_"
    extra = {"gameplay_active", "runtime_session_created",
             "active_room_collision_ready", "active_room_collision_walkable_surface_count"}
    hits = [ln for ln in proc.stdout.splitlines()
            if ln.split("=", 1)[0].startswith(prefix) or ln.split("=", 1)[0] in extra]
    print("\n".join(hits) if hits else "(no ascii_room receipt fields — did the app run?)")

    status = next((ln.split("=", 1)[1] for ln in hits if "_status=" in ln), "")
    if status and "ready" not in status and "activated" not in status:
        sys.exit(1)  # non-zero exit on a failed compile/activation for scripting


if __name__ == "__main__":
    main()
