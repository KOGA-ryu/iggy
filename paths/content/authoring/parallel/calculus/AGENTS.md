# Calculus authoring worker

This is a subject brief for an explicitly dispatched worker. No worker has
started merely because this file exists. Complete only [PILOT.md](PILOT.md),
using the fixed [sequence](sequence.json) and
[shared contract](../../../../docs/PARALLEL_QUESTION_AUTHORING.md).
Read the shared contract in full; it owns presentation and verification rules.

## Subject conventions

State that inputs are real. Distinguish f(a) from f'(a), average slope from
instantaneous slope, and h=0 from a limit as h approaches zero. Define the
increment and explain why cancellation uses h nonzero before taking the limit.

Use exact polynomials of degree at most two for this pilot. Derive the
difference quotient by polynomial expansion; check the derivative separately
using coefficient differentiation. A finite-difference sample is supplemental,
and an identity needs coefficient equality, not a few numerical evaluations.

There is no general runtime calculus checker. Supply this bounded authoring
certificate family. Do not add a CAS, a runtime symbolic parser, integrals,
chain-rule tasks or discontinuous functions. New families require their own
conditions and independent checks.

## Scope and return

Your source folder is `content/authoring/parallel/calculus/`.
Only author lesson.md.in, questions.paths.md.in, certificates.py,
certificate_tests.py, authoring.json and review.md there. The reserved sequence,
brief and pilot remain read-only. Write evidence only under
`build/parallel-authoring/calculus/`. Follow the shared exact gates.

Use multiple-choice controls and the existing textbook renderer. Supply one
complete lesson, six questions, seven decisions and fourteen diagnosed wrong
choices. The coordinator owns shared builds, registration and publication.
Do not modify other subjects, runtime code, personal progress or 3D assets.
No extra agents, windows, screenshots, captures, font probes, commits or pushes.
Stop after returning the candidate path, checks and any outstanding issue.
