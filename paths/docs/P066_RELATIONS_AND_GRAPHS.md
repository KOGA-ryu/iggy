# P066 — Relations and Graphs Lab

`graph` is the 37th Math Lab asset. It owns a directed binary relation on six
labelled nodes, including self-loops, with 42 controls and nine complete presets.
The compact inspector exposes only the selected node's outgoing row.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object graph --level 3 --object-preset 6
```

Press Play to build the matching. Violet marks current pairs; a pending
alternating path uses gold for additions and coral for removals. The bead
traverses that path, then the pairs flip together at the next integer stage.
This example's last augmentation reroutes an existing pair. Scrub to about
2.5 to inspect it. Matching playback lasts three seconds.

| Layer | Construction |
| --- | --- |
| 0 — Connections and adjacency | Select a node, edit its outgoing switches, and read the synchronized binary matrix |
| 1 — Paths and reachability | Reveal BFS distance layers; highlight the selected destination's shortest path |
| 2 — Relations and property witnesses | Check reflexivity, symmetry and transitivity; inspect missing edges; group valid equivalence classes |
| 3 — Bipartite matching | Restrict to links 0..2 -> 3..5; animate augmentations and inspect Hall obstructions |

Click a node or choose Edit row / search source. Camera dragging still orbits.
Matching permits selecting only left nodes 0..2. Its switches edit destinations
3..5; other stored edges are retained but ignored in that layer. All edits pause
playback. Reset restores the branching network.

Preset indices: 0 Branching network; 1 Directed cycle; 2 Broken transitivity;
3 Three interleaved classes; 4 All connected; 5 Empty relation;
6 Matching needs a reroute; 7 Hall obstruction; 8 All bipartite links.

To inspect a missing transitivity edge:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object graph --level 2 --object-preset 2
```

The required N0 -> N2 connection is dashed coral; the two existing witness
connections are gold. The missing connection remains zero in the matrix until
edited. Selecting another property changes the witness. If that property holds,
no counterexample is fabricated. The grouping control is available only for
valid equivalence relations; grouping changes placement, never the relation.

## Reusable mathematical and presentation contract

`FiniteGraph.hpp/.cpp` own the fixed 36-bit relation, deterministic BFS distances
and parents, first property witnesses, equivalence-class IDs, bipartite extraction,
augmenting paths, intermediate matchings and a deficient Hall subset. No renderer,
randomness or learner persistence participates in those calculations.

Matrix entry (i,j) and the N_i -> N_j arrow read the same bit. The matrix is a
bounded triangle mesh, leaving primitive capacity for dense graph arrows and
witnesses. Lit cells mean one; dark cells mean zero. Opposite arrows bend in
opposite directions; self-loops have arrowheads. Positions do not imply weights.
All graph paths use unit edge costs. Reachability includes the source by a
zero-length path even if its self-loop is absent. BFS ties use ascending node IDs.
The table shows complete distances/parents; the animation reveals distance layers
one per second over six seconds. -1 denotes an unreachable distance or no parent.

Reflexivity is checked on all six nodes. The empty relation is symmetric and
transitive but not reflexive. Witnesses can repeat nodes (for example, a two-cycle
without its required self-loop). Non-equivalence relations have no class IDs.

Matching starts empty and uses deterministic BFS augmentations. Each successful
path increases cardinality by one; integer stages record valid matchings. After
the maximum is reached, remaining stages retain it. Hall witnesses enumerate all
seven nonempty left subsets, choosing the first greatest deficiency. A perfect
matching exists precisely when maximum size is three and deficiency is zero.
The highlighted neighbor set contains all eligible neighbors of the chosen left
subset, independent of the current matching stage.

This supplies foundations for Discrete 120/122/127/130/147 and Algebra 003.
Chapter integration, arbitrary graph sizes, weighted paths, coloring, automata,
spanning trees, and general relation/category constructions remain separate work.
The existing cube-route model and probability-network model remain available.

## Verification

Both Release applications (`math_lab`, `sorter`) build. All 14 selected CTest
checks pass: five CPU suites and nine text-only CLI cases. The graph suite covers
2,634 relations from all six sources, including every embedded three-node
relation, dense/sparse boundaries and deterministic six-node samples. Independent
walk enumeration checks BFS; relational composition checks transitivity; every
reported witness is verified against the original relation.

All 512 three-by-three bipartite graphs are checked against exhaustive matching
enumeration, including intermediate validity, alternating paths, symmetric-
difference updates, maximum cardinality and Hall deficiency. The suite also
constructs 2,560 fresh CPU scenes so equal revisions on independent model copies
cannot reuse stale geometry. Dense cases, property witnesses, all presets,
control limits, grouping, playback and atomic rejection pass. Observed maxima:
182 parts, 6,488 vertices, 19,416 indices. Existing compact-layout checks cover
6,480 layouts and 136 object/layer states.

The linker retains its duplicate `libpaths_imgui.a` warning. Verification uses
only source, CPU geometry and text: no native windows, images, screenshots,
captures or font probes. Visual review remains with the user. Work is uncommitted;
source cards, learner state and textbook integration are not modified.
