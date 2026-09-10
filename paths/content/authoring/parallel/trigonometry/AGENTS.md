# Trigonometry authoring worker

This is a subject brief for an explicitly dispatched worker. No worker has
started merely because this file exists. Complete only [PILOT.md](PILOT.md),
using the fixed [sequence](sequence.json) and
[shared contract](../../../../docs/PARALLEL_QUESTION_AUTHORING.md).
Read the shared contract in full; it owns presentation and verification rules.

## Subject conventions

State radians explicitly and preserve every interval endpoint. Sine is the
vertical coordinate on the unit circle. Distinguish an angle, its sine value,
a principal inverse value and the full set of angles solving an equation.

Use exact rational multiples of pi. For this pilot use only sine values
0, 1/2, -1/2 and the separate worked value 1. Enumerate both periodic branches,
intersect with the stated interval and deduplicate coincident endpoints.
A principal inverse value or sampled plot is not a complete solution set.

There is no general runtime trigonometry checker. Supply bounded authoring
certificates with exact angle arithmetic and a completeness argument. Do not
add inverse-trig UI, arbitrary trigonometric identities, a solver dependency or
new 3D controls. Record other topics as later families.

## Scope and return

Your source folder is `content/authoring/parallel/trigonometry/`.
Only author lesson.md.in, questions.paths.md.in, certificates.py,
certificate_tests.py, authoring.json and review.md there. The reserved sequence,
brief and pilot remain read-only. Write evidence only under
`build/parallel-authoring/trigonometry/`. Follow the shared exact gates.

Use multiple-choice controls and the existing textbook renderer. Supply one
complete lesson, six questions, seven decisions and fourteen diagnosed wrong
choices. The coordinator owns shared builds, registration and publication.
Do not modify other subjects, runtime code, personal progress or 3D assets.
No extra agents, windows, screenshots, captures, font probes, commits or pushes.
Stop after returning the candidate path, checks and any outstanding issue.
