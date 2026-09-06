# problems

Math and CS practice from your own books.

## Setup, once

NumPy and SciPy are not installed anywhere on this Mac. `/usr/bin/python3`
(3.9.6) and Homebrew's 3.14.3 both fail on `import numpy`, and Homebrew's
refuses `pip install` outright with `error: externally-managed-environment`.

```bash
python3 -m venv ~/devil/99-red-booleans/problems/.venv
~/devil/99-red-booleans/problems/.venv/bin/pip install numpy scipy
```

Run the programs you write with that Python:

```bash
~/devil/99-red-booleans/problems/.venv/bin/python problems/work/002.py
```

Or add an alias once:

```bash
echo "alias venvpy='~/devil/99-red-booleans/problems/.venv/bin/python'" >> ~/.zshrc
```

`work/exercises.py` and `work/pp.py` need none of this — both are standard
library only, so plain `python3` runs them. The venv is for the programs you
write.

## The files

| file | what it is |
|---|---|
| `ORDER.md` | the books, math and CS |
| `EXTRACT.md` | how to get exercises out of each math book |
| `CLAIMS.md` | claims worth testing, per CS book |
| `TEMPLATE-math.md`, `TEMPLATE-cs.md` | copy one to start a page |
| `work/exercises.py` | prints a book's exercises |
| `work/pp.py` | reads the pages: `fetch`, `check`, `lint`, `status` |
| `math/`, `cs/` | the pages |
| `work/` | your programs |

## Where the books are

Math, 59 books, extracted:

```
~/dev/wiki/state/local_corpus/ml-letsgo/outputs/math/<book>/chapter_json/ch_NN/chapter.json
```

`reader_plain.txt` sits beside it and is damaged for mathematics — the build
strips `<...>` as if it were HTML, which across 400 chapters deletes about 4,900
spans that are not tags. A Cauchy condition loses `< \epsilon$ holds whenever
m, n >` and says nothing about it. `chapter_json` is clean.

```bash
python3 problems/work/exercises.py trefethen_bau_numerical_linear_algebra
python3 problems/work/exercises.py --list
```

The script warns about the three books it cannot read straight — Boyd, Øksendal,
and Trefethen's *Spectral Methods*. `EXTRACT.md` says what to do about each.

CS, 21 books, on your shelf, not extracted. The notes under
`~/dev/wiki/state/wiki_mirror/sources/computer/` say what each is for, and
`CLAIMS.md` has claims already shaped for the template.

## Making a page

```bash
cp problems/TEMPLATE-math.md problems/math/004_short_name.md   # edit the frontmatter
python3 problems/work/pp.py fetch 004    # prints the exercise, verbatim, quoted
#   ... write the rest of the page, then the program in work/004.py
python3 problems/work/pp.py check 004    # runs it, writes Result, marks it done
python3 problems/work/pp.py lint         # rule violations it can see
python3 problems/work/pp.py status       # every page and where it stands
```

`pp.py` reads the page the way the mentor app's `parse_lesson` reads a card:
`+++` frontmatter, then `## ` sections. It does the mechanical half.

- **`fetch`** finds the exercise named in the frontmatter — `book`, `chapter`,
  `exercise` — inside `chapter_json` and prints it quoted, LaTeX intact. Nothing
  is copied by hand, so rule 1 holds without anyone remembering it.
- **`check`** runs `work/NNN.py`, writes whatever it printed into **Result**, and
  sets `status = "done"` when the program exits 0. **Exit 0 means the check
  passed** — that is the whole protocol, and it is the `command_exits_zero` idea
  the card format has needed in three separate places.
- **`lint`** catches what a page can be checked for mechanically: a check with no
  tolerance, `==` on floats, a missing Answer, an exercise that was paraphrased
  instead of quoted, a CS locator that looks like a page number.
- **`status`** prints the frontmatter's `status` for every page. It does not
  guess.

---

# Rules — math

The books have exercises and no answers. So every rule here is about how you
know you are right.

**1. The exercise goes on the page verbatim.** Copied out of `chapter_json`,
LaTeX and hints included. A paraphrase loses the constraint that made it a
problem.

**2. The answer goes on the page, and it is the assistant's.** Worked out
without running anything. It is there so you are never stuck with nowhere to go.

**3. The check computes, and it outranks the answer.** A library call, an
invariant, two methods compared, or a planted solution. When the program and the
written answer disagree, the program is the one that ran.

**4. Every check states a tolerance.** Floating point does not give equality —
`np.sin(np.pi)` is `1.22e-16`, not zero. `==` on a float is a bug in the page.

**5. If the exercise has a parameter, run it at three values.** A size, a
bandwidth, a step count. One run gives a number; three give a rule, and the rule
is usually what was being asked for.

**6. A wrong answer gets said out loud and fixed once.** Yours, mine, or the
check's. The page is better afterwards than one that was never tested.

---

# Rules — cs

The books have claims and no exercises. So every rule here is about turning an
assertion into a measurement.

**1. One claim per page, cited by chapter or item name.** Never a page number —
editions differ, and a page number nobody checked is how a page becomes
unusable.

**2. Two versions, one difference.** The way the book advises and the way it
warns against. Same inputs, same output, everything else held equal, and the
page says what was held equal.

**3. Write the prediction down before running.** Including the size of
difference that would count as real. A 2% gap on one run is noise, and deciding
that afterwards is how you talk yourself into a result.

**4. Four verdicts, all of them results.** Confirmed. Confirmed but too small to
matter. Not observable here. The opposite. The second is the most common and the
fourth is worth more than the other three together.

**5. Report the boundary, not the verdict.** At what size, count or frequency
does the claim start to be true? That is the number the book cannot give you,
because it does not know your machine or your data.

**6. A book states a rule for the general case; your program is a particular
case.** The claim failing here is not the book being wrong.

---

## The check

Four ways to know an answer is right:

| kind | what it means | example |
|---|---|---|
| **library** | a function already computes this, or a book prints the output of its own program | `numpy.linalg.lstsq` gives the same solution |
| **invariant** | a property the right answer must have | `P @ A == L @ U`; the residual is orthogonal to the columns; the error falls fourfold when you halve the step for a second-order method; a two-class logistic loss at zero weights is exactly `ln 2` |
| **two methods** | compute it two ways and compare | normal equations against QR; a derived gradient against a centred finite difference, `(f(x+h) - f(x-h)) / 2h`, `h = 1e-5`, at a random interior point, to a relative error of `1e-7` |
| **planted** | choose the answer first | pick a seeded random `x`, form `b = A @ x`, solve, measure the distance back |

Two things about the planted check. Plant a seeded random `x`, not a vector of
ones — a trivial plant passes buggy code. And scale the tolerance by the
condition number, roughly `cond(A) * 1e-16`, because an ill-conditioned matrix
genuinely cannot return your `x`.

That second one is the difference between the residual being small — the
equation holds, backward error — and the answer being right, forward error. On a
badly conditioned problem the first can be tiny while the second is huge.

Always state a tolerance. `1e-10` for well-conditioned linear algebra, `1e-6`
when an iteration is involved, a wide band for anything with sampling in it.
`==` on floating point is a bug.

## When the check disagrees

Usually the program. Print the intermediate values.

Sometimes the check — wrong library function, tolerance too tight, or a
convention mismatch: degrees against radians, population against sample
variance, 252 against 251 trading days, a library's defaults (a regulariser
that is on, a stopping tolerance looser than yours).

Sometimes the Answer on the page, which the assistant wrote without running
anything. Say so and it gets fixed.

## The finance lane

Separate, and already built. `~/dev/tapelawl` opens the 820 sealed puzzles in
`~/dev/Arc/data/puzzle_packs/` — `anomaly_v1` 199, `attention_v1` 475,
`execution_v1` 146 — grades them against the sealed keys, and
`~/dev/Arc/python/dojo/scorecard.py` prints the calibration.
