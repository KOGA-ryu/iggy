# The books

## Math

Extracted, with exercises on disk. Counts are exercise sections found by
`work/exercises.py`.

| book | sections | what is in it |
|---|---:|---|
| `meckes_meckes_linear_algebra` | 36 | vectors, matrices, bases, eigenvalues, SVD, at undergraduate level |
| `trefethen_bau_numerical_linear_algebra` | 25 | conditioning, least squares, QR, SVD, stability |
| `boyd_convex_optimization` | — | convexity, duality, gradient and Newton methods. Serves AI and finance at once. Needs the separate snippet in `EXTRACT.md` |
| `oksendal_stochastic_differential_equations` | 41 | Brownian motion, Itô's formula, Black–Scholes. **Contains the only answer key in these nine** — `ch_10`, *Solutions and Additional Hints to Some of the Exercises*, about 12,700 characters |
| `resnick_heavy_tail_phenomena` | 8 | tail estimation, value at risk |
| `billingsley_probability_measure` | 40 | measure-theoretic probability |
| `horn_johnson_analysis_matrix` | 53 | matrix analysis, norms, perturbation |
| `trefethen_spectral_methods_matlab` | 15 | spectral methods. **The extraction is damaged**: figures missing, 70 dead image references, 11 of 29 program listings carry OCR errors (`pi3.m` for `p13.m`). A wrong answer here may be the source's fault. The author's M-files are on his page |
| `golub_van_loan_matrix_computations` | 113 | the reference. Largest supply on the list |

`work/exercises.py --list` prints the other fifty. Munkres, Hatcher, Mac Lane,
Lee, Jacobson, the functional analysis shelf, the differential geometry shelf —
all extracted, all available.

## CS

On your shelf, not extracted. `CLAIMS.md` has two or three claims per book, with
the chapter to look in.

| book | what it gives you |
|---|---|
| Agner Fog, *Optimizing Software in C++* | branch prediction, cache, where the time actually goes. Every claim is a measurement |
| Meyers, *Effective Modern C++* | 42 items, each a claim. `design/books/effective-modern-cpp.md` maps item → card |
| Cormen, *Algorithms Unlocked* | growth rates, sorting, searching, shortest paths |
| Roughgarden, *Algorithms Illuminated: Graph Algorithms and Data Structures* | graphs, heaps, hash tables — the structures no project here forces |
| Higham, *Accuracy and Stability of Numerical Algorithms* | floating point, summation, condition numbers. The bridge to the math lane |
| Kleppmann, *Designing Data-Intensive Applications* | storage, indexing, replication. Measurable against SQLite, which macOS has |
| Williams, *C++ Concurrency in Action* | mutexes, the memory model, false sharing. Nothing you have written is threaded yet |
| Kurose & Ross, *Computer Networking* | layering, transport, congestion. The only lane that needs the network |

Notes for all of them: `~/dev/wiki/state/wiki_mirror/sources/computer/`.

## Finance

`~/dev/tapelawl` — 820 sealed puzzles from `~/dev/Arc/data/puzzle_packs/`,
graded against their keys, calibration printed by
`~/dev/Arc/python/dojo/scorecard.py`.
