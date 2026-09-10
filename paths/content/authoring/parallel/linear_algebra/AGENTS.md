# Linear Algebra authoring worker

This is a subject brief for an explicitly dispatched worker. No worker has
started merely because this file exists. Complete only [PILOT.md](PILOT.md),
using the fixed [sequence](sequence.json) and
[shared contract](../../../../docs/PARALLEL_QUESTION_AUTHORING.md).
Read the shared contract in full; it owns presentation and verification rules.

## Subject conventions

Keep the column order x, y, constant and explain the augmented bar. A prime
labels a changed row; it is not a derivative here. Evaluate row replacement
from the old rows and operate on the constant column too. Name the destination
and the row that remains unchanged.

Use exact rational arithmetic on two-row, three-column augmented matrices.
Prove reversibility through the inverse operation and check the same solution
in both original equations. Distinguish a valid row operation from one that
eliminates the requested coefficient.

Reuse build_question_batch.MATRIX_ROLE_CHECKERS for these fixed cases. Do not
copy/reimplement that table or add another eliminator to the app. Singular,
inconsistent, free-variable and larger systems are later distinct families;
the unique-system certificates cannot be relabelled to cover them.

## Scope and return

Your source folder is `content/authoring/parallel/linear_algebra/`.
Only author lesson.md.in, questions.paths.md.in, certificates.py,
certificate_tests.py, authoring.json and review.md there. The reserved sequence,
brief and pilot remain read-only. Write evidence only under
`build/parallel-authoring/linear_algebra/`. Follow the shared exact gates.

Use multiple-choice controls and the existing textbook renderer. Supply one
complete lesson, six questions, seven decisions and fourteen diagnosed wrong
choices. The coordinator owns shared builds, registration and publication.
Do not modify other subjects, runtime code, personal progress or 3D assets.
No extra agents, windows, screenshots, captures, font probes, commits or pushes.
Stop after returning the candidate path, checks and any outstanding issue.
