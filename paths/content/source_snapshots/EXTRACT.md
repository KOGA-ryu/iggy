# Getting the exercises out of a book

Every book numbers its exercises differently, so there is no single command. There
is one script that works for all of them, and a table of what each book calls its
exercise sections.

Verified 2026-09-05 against the files on disk.

---

## The script

Save this once as `problems/work/exercises.py`. It prints every exercise section
in a book, chapter by chapter.

```python
import json, glob, re, sys

BASE = "/Users/kogaryu/dev/wiki/state/local_corpus/ml-letsgo/outputs/math"

book = sys.argv[1]
want = sys.argv[2] if len(sys.argv) > 2 else "exercis|problem"

for f in sorted(glob.glob(f"{BASE}/{book}/chapter_json/*/chapter.json")):
    ch = f.split("/")[-2]
    secs = json.load(open(f))["sections"]
    for i, s in enumerate(secs):
        if re.search(want, s.get("title", ""), re.I):
            body = s.get("content", "")
            # some books put the title in one section and the exercises in the next
            if len(body) < 40 and i + 1 < len(secs):
                body = secs[i + 1].get("content", "")
            if len(body) < 40:
                continue
            print(f"\n{'='*70}\n{ch}  {s['title']}\n{'='*70}\n{body}")
```

Run it with the venv Python:

```bash
~/devil/99-red-booleans/problems/.venv/bin/python problems/work/exercises.py trefethen_bau_numerical_linear_algebra
```

The second argument overrides what counts as an exercise heading. You need it
for two books; the table says which.

---

## What each book calls them

| # | book | section heading | an item looks like | note |
|---|---|---|---|---|
| 1 | `meckes_meckes_linear_algebra` | `**EXERCISES**` | `- 1.1.1 ` | three-level numbering, no bold. 36 sections across 6 chapters |
| 2 | `trefethen_bau_numerical_linear_algebra` | `Exercises` | `- **20.1.**` | one chapter file for the whole book; 25 sections |
| 3 | `boyd_convex_optimization` | **use `--` see below** | `**4.11**` | the heading trick fails here |
| 4 | `oksendal_stochastic_differential_equations` | `Exercises` | `**5.1.**` | 11 real sections in `ch_10`, plus a solutions section — see below |
| 5 | `resnick_heavy_tail_phenomena` | `N.N Problems` | `- 2.2.` | **the heading section is empty**; the script's look-ahead handles it |
| 6 | `billingsley_probability_measure` | `**Problems**` | `- 1.1.` | first block opens with a paragraph about the problems; skip it |
| 7 | `horn_johnson_analysis_matrix` | `**Problems**` | `**1.0.P1**` | `P` for problem, after the section number |
| 8 | `trefethen_spectral_methods_matlab` | `**Exercises**` | `- **3.1.**` | **read the warning in `ORDER.md` first** — this extraction is damaged |
| 9 | `golub_van_loan_matrix_computations` | `**Problems**` | `- **P1.1.1**` | 113 sections, the largest supply on the list |

### Boyd is the exception

Boyd's exercise blocks are not under a heading the script can find — the word
"problems" appears in ordinary section titles like *"1.1.2 Solving optimization
problems"*, so searching headings returns prose. The exercises are bold
`**N.N**` items sitting at the **end of each chapter file**.

Print them by item instead of by heading:

```python
import json, glob, re
BASE = "/Users/kogaryu/dev/wiki/state/local_corpus/ml-letsgo/outputs/math"
for f in sorted(glob.glob(f"{BASE}/boyd_convex_optimization/chapter_json/*/chapter.json")):
    secs = json.load(open(f))["sections"]
    for s in secs[len(secs)//2:]:                      # exercises live in the back half
        for m in re.finditer(r"\*\*(\d+\.\d+)\*\*(.{0,600})", s.get("content", ""), re.S):
            print(f"\n[{m.group(1)}] {m.group(2).strip()}")
```

Read what it prints before trusting it: the same bold pattern is used for
numbered *examples* earlier in a chapter, so an item printed from the front half
of a file may not be an exercise. Boyd's exercises restart their numbering at
each chapter, so `4.11` is chapter 4's eleventh.

### Øksendal has answers, and that is a hazard

`ch_10` carries a section titled *Solutions and Additional Hints to Some of the
Exercises*, about 12,700 characters covering roughly 30 items. It is the only
answer key anywhere in the nine books.

It is in the same file as the exercises, so it is easy to read by accident.

---

## Which exercises are computational

Some read as programs and some as proofs. Signals, from the books' own wording:

- **It says "generate your own instances", "experiment with", "compute", or
  "verify numerically".** These are written to be run. Boyd and both Trefethens
  use this language often.
- **It has a parameter you can vary** — a size, a bandwidth, a step, a count.
  Then the template's three-sizes rule applies and one exercise gives you a rule
  rather than a number.

And the other way:

- **"Prove", "show that", "let ... be arbitrary"** with nothing to compute.
- **It refers to a figure** you would have to regenerate first.
